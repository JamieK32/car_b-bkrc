import sensor, image, time, lcd
from Maix import GPIO
from machine import UART, Timer, PWM
from fpioa_manager import fm
import re
import math

# ============================================================================
#                                  1. 调试与常量
# ============================================================================

DEBUG_MODE_SWITCH = True
DEBUG_TRAFFIC = True
DEBUG_SERVO = False
DEBUG_SENSOR = False
DEBUG_LINE = False

class Protocol:
    """协议常量定义"""
    PKT_HEAD = 0x55
    PKT_VER = 0x02
    PKT_TAIL = 0xBB

    CMD_SERVO = 0x91
    CMD_TRAFFIC = 0x92
    CMD_TRACE = 0x93
    CMD_VAR   = 0x95

    SUB_SERVO_SET = 0x01

    SUB_TRACE_START = 0x06
    SUB_TRACE_STOP = 0x07
    SUB_TRACE_DATA = 0x01

    SUB_TRAFFIC_START = 0x03
    SUB_TRAFFIC_STOP = 0x04
    SUB_TRAFFIC_RESULT = 0x01

    SUB_QR_START = 0x01

    SERVO_POSITIVE_FLAG = 0x2B

class VarID:
    """变量 ID 映射表"""
    WXD  = 0x10
    FHT  = 0x11
    CKCS = 0x12
    LDC  = 0x13
    LDZ  = 0x14
    CP   = 0x15
    SJ   = 0x16
    WD   = 0x17
    LS1  = 0x18
    XY   = 0x19
    AF   = 0x20
    MY   = 0x21
    IQ   = 0x22
    WXD2 = 0x23
    WXD3 = 0x24

class Mode:
    """模式常量"""
    IDLE = 0
    LINE = 1
    TRAFFIC = 2
    QRCODE  = 3
    NAMES = ("IDLE", "LINE", "TRAFFIC", "QRCODE")

def debug_print(tag, msg):
    """调试打印"""
    print("[{}] {}".format(tag, msg))

# ============================================================================
#                                  2. 辅助工具
# ============================================================================

class StrictValidator:
    def hex_str_to_bytes(s: str) -> bytes:
        # 去空格
        s = s.replace(" ", "")
        # 按逗号拆分
        parts = s.split(",")
        # 直接转 bytes
        return bytes(int(p, 16) for p in parts)

    def is_hex(s):
        if not s: return False
        for c in s:
            if not ((c >= '0' and c <= '9') or (c >= 'A' and c <= 'Z')):
                return False
        return True

    def is_num(s):
        if not s: return False
        for c in s:
            if not (c >= '0' and c <= '9'):
                return False
        return True
    def is_letter(s):
         if not s: return False
         for c in s:
            if not (c >= 'A' and c <= 'Z'):
                return False
         return True

    def parse_bracket_content(text, bracket_types, keep_type=False):
        if not text or not bracket_types:
            return []
        open_to_close = {b[0]: b[1] for b in bracket_types}
        opens = set(open_to_close.keys())
        closes = set(open_to_close.values())

        results = []
        stack = []  # 栈元素: [start_char, end_char, buffer_list]

        for ch in text:
            # 1) 如果遇到起始符：开启一个新的捕获（允许嵌套）
            if ch in opens:
                stack.append([ch, open_to_close[ch], []])
                continue

            # 2) 如果当前处于某层括号内部
            if stack:
                start_char, end_char, buf = stack[-1]
                # 2.1) 当前字符是这一层的结束符：收束这一层
                if ch == end_char:
                    content = ''.join(buf)
                    if keep_type:
                        results.append((start_char + end_char, content))
                    else:
                        results.append(content)
                    stack.pop()
                    continue
                # 2.2) 当前字符是某个“结束符”，但不是当前层需要的结束符：
                #      忽略它（防止串扰），仍继续在当前层收集字符
                #      你也可以选择把它当普通字符加入 buf，这里默认忽略更稳。
                if ch in closes and ch != end_char:
                    continue
                # 2.3) 普通字符：加入当前层 buffer
                buf.append(ch)
        return results


# ============================================================================
#                                  3. 传感器管理
# ============================================================================

class SensorManager:
    """传感器配置管理"""
    def __init__(self, debug=False):
        self.debug = debug
    def init_gray_sensor(self):
        """初始化灰度传感器（循迹模式）"""
        sensor.reset()
        sensor.set_vflip(1)
        sensor.set_framesize(sensor.QVGA)
        sensor.set_pixformat(sensor.GRAYSCALE)
        sensor.set_brightness(-30)
        sensor.set_contrast(2)
        sensor.skip_frames(time=300)

    def init_rgb_sensor(self):
        """初始化RGB传感器（交通灯模式）"""
        sensor.set_pixformat(sensor.RGB565)
        sensor.set_brightness(-10)
        sensor.set_auto_gain(False)
        sensor.set_auto_whitebal(False)

    def init_qr_sensor(self):
        """初始化传感器（二维码模式）"""
        sensor.reset()
        sensor.set_vflip(1)
        sensor.set_framesize(sensor.QVGA)
        sensor.set_pixformat(sensor.GRAYSCALE)
        # === 正常亮度设置 ===
        sensor.set_auto_gain(False,gain_db = 12)# gain_db = 12
        sensor.set_auto_exposure(True)      # 允许自动曝光
        sensor.set_brightness(2)      # 标准亮度
        sensor.set_contrast(4)        # 标准对比度
        sensor.skip_frames(time = 500)

# ============================================================================
#                                  4. 串口通信 (已修复延时)
# ============================================================================

class UARTComm:
    """串口通信封装"""
    def __init__(self, uart_obj, debug=False):
        self.uart = uart_obj
        self.debug = debug

    def send_packet(self, cmd, sub, d1=0, d2=0):
        chk = (cmd + sub + d1 + d2) & 0xFF
        self.uart.write(bytes([
            Protocol.PKT_HEAD, Protocol.PKT_VER, cmd, sub, d1, d2, chk, Protocol.PKT_TAIL
        ]))
        if self.debug:
            debug_print("UART", "Send: cmd=0x{:02X}, sub=0x{:02X}, d1={}, d2={}".format(cmd, sub, d1, d2))

    def send_trace_packet(self, value):
        self.send_packet(Protocol.CMD_TRACE, Protocol.SUB_TRACE_DATA, value, 0)

    def send_var(self, var_id, data_bytes):
        pkt_len = len(data_bytes) + 2  # CMD_VAR + VarID + DATA
        pkt = bytearray([Protocol.PKT_HEAD, pkt_len, Protocol.CMD_VAR, var_id])
        pkt.extend(data_bytes)
        chk = (Protocol.CMD_VAR + var_id + sum(data_bytes)) & 0xFF
        pkt.append(chk)
        pkt.append(Protocol.PKT_TAIL)
        print("[变量发送] VarID: 0x{:02X} Len: {} Data: {}".format(var_id, len(data_bytes), data_bytes))
        self.uart.write(pkt)

        # 【修改点】增加延时到 200ms，确保 STM32 有足够时间处理上一包数据并打印
        time.sleep_ms(200)

    def read_packet(self, length=8):
        return self.uart.read(length) if self.uart.any() else None

    def has_data(self):
        return self.uart.any()

# ============================================================================
#                                  5. 模式管理
# ============================================================================

class ModeManager:
    """模式状态管理"""
    def __init__(self, debug=False):
        self.current_mode = Mode.IDLE
        self.debug = debug

    def set_mode(self, new_mode):
        if self.debug:
            debug_print("MODE", "{} -> {}".format(
                Mode.NAMES[self.current_mode], Mode.NAMES[new_mode]))
        self.current_mode = new_mode

    def is_mode(self, mode):
        return self.current_mode == mode

# ============================================================================
#                                  6. 舵机控制
# ============================================================================

class ServoController:
    """舵机控制器"""
    def __init__(self, pwm_obj, min_angle=-80, max_angle=40, debug=False):
        self.pwm = pwm_obj
        self.min_angle = min_angle
        self.max_angle = max_angle
        self.debug = debug

    def set_angle(self, angle):
        angle = max(self.min_angle, min(self.max_angle, angle))
        self.pwm.duty((angle + 90) / 18 + 2.5)
        if self.debug:
            debug_print("SERVO", "Angle {}".format(angle))
        return angle

# ============================================================================
#                                  7. 交通灯检测
# ============================================================================

class TrafficLightDetector:
    """交通灯检测器"""
    COLOR_UNKNOWN = 0
    COLOR_RED = 1
    COLOR_GREEN = 2
    COLOR_YELLOW = 3

    TRAFFIC_RED = [(75, 26, 29, 114, 1, 127)]
    TRAFFIC_GREEN = [(100, 0, -128, -20, -128, 127)]
    TRAFFIC_YELLOW = [(80, 60, -10, 127, 10, 127)]
    TRAFFIC_ROI = (0, 0, 320, 120)

    TIMEOUT_MS = 1000

    def __init__(self, sensor_mgr, uart_comm, debug=False):
        self.sensor_mgr = sensor_mgr
        self.uart_comm = uart_comm
        self.debug = debug

    def detect(self):
        self.sensor_mgr.init_rgb_sensor()
        votes = [0, 0, 0, 0]
        start_tick = time.ticks_ms()

        for _ in range(5):
            if time.ticks_diff(time.ticks_ms(), start_tick) > self.TIMEOUT_MS:
                result = self.COLOR_YELLOW
                self.uart_comm.send_packet(Protocol.CMD_TRAFFIC, Protocol.SUB_TRAFFIC_RESULT, result, 0)
                return result

            img = sensor.snapshot()
            r = img.find_blobs(self.TRAFFIC_RED, roi=self.TRAFFIC_ROI, pixels_threshold=150)
            g = img.find_blobs(self.TRAFFIC_GREEN, roi=self.TRAFFIC_ROI, pixels_threshold=150)
            y = img.find_blobs(self.TRAFFIC_YELLOW, roi=self.TRAFFIC_ROI, pixels_threshold=150)

            rp = max((b.pixels() for b in r), default=0)
            gp = max((b.pixels() for b in g), default=0)
            yp = max((b.pixels() for b in y), default=0) * 1.5

            if yp > rp and yp > gp and yp > 200:
                votes[self.COLOR_YELLOW] += 1
            elif gp > rp and gp > 200:
                votes[self.COLOR_GREEN] += 1
            elif rp > 300:
                votes[self.COLOR_RED] += 1
            else:
                votes[self.COLOR_UNKNOWN] += 1

        result = votes.index(max(votes)) if max(votes) >= 2 else self.COLOR_UNKNOWN
        self.uart_comm.send_packet(Protocol.CMD_TRAFFIC, Protocol.SUB_TRAFFIC_RESULT, result, 0)
        return result

# ============================================================================
#                                  8. 循迹模块
# ============================================================================

class LineTracker:
    """循迹控制器"""

    LINE_THRESHOLDS = [
        [(53, 0)], [(53, 0)],
        [(90, 0, 127, -128, -128, 127)],
        [(95, 0, 127, -128, -128, 127)],
        [(90, 0, 127, -128, -128, 127)],
        [(90, 0, 127, -128, -128, 127)],
        [(80, 0, 127, -128, -128, 127)],
        [(53, 0)]
    ]
    # 8个 ROI 区域
    LINE_ROI_LIST = [(i * 40, 170, 40, 70) for i in range(8)]

    def __init__(self, uart_comm, debug=False):
        self.uart_comm = uart_comm
        self.debug = debug

    def draw_rois(self, img, active_bitmask=0):
        """在图像上绘制 ROI 框和编号"""
        for i, r in enumerate(self.LINE_ROI_LIST):
            is_active = (active_bitmask >> (7 - i)) & 1
            color = 255 if is_active else 127
            thickness = 2 if is_active else 1
            img.draw_rectangle(r, color=color, thickness=thickness)
            img.draw_string(r[0] + 15, r[1] + 5, str(i), scale=1.5, color=255)

    def step(self):
        """执行一次循迹检测"""
        img = sensor.snapshot()
        bitmask = 0

        # 1. 识别
        for i in range(8):
            if img.find_blobs(self.LINE_THRESHOLDS[i], roi=self.LINE_ROI_LIST[i], pixels_threshold=50):
                bitmask |= 1 << (7 - i)

        # 2. 发送数据
        self.uart_comm.send_trace_packet(bitmask)

        # 3. 绘制 ROI 和编号
        self.draw_rois(img, bitmask)

        # 4. 显示
        lcd.display(img)
        if self.debug:
            debug_print("LINE", "Bitmask: 0x{:02X}".format(bitmask))

# ============================================================================
#                                  9. 二维码模块 (修复版)
# ============================================================================
class QRCodeProcessor:
    """二维码处理器 - 疯狗轮询 + 自动点头 + 全覆盖战术版"""
    def __init__(self, sensor_mgr, uart_comm, servo_ctrl, debug=False):
        self.sensor_mgr = sensor_mgr
        self.uart_comm = uart_comm
        self.servo_ctrl = servo_ctrl
        self.debug = debug

        # ==================== 1. 你的原始 ROI ====================
        self.roi_screen_left = (43, 0, 122, 90)
        self.roi_screen_right_top = (141, 0, 180, 134)
        self.roi_screen_right_bottom = (107, 119, 212, 122)
        self.roi_screen_middle = (79, 66, 157, 125)
        self.roi_screen_letf_bottom = (2, 113 , 199 , 128)

        # ==================== 2. 新增战术 ROI (广域覆盖) ====================
        # 针对四个角落的广域覆盖 (带重叠，防止死角)
        self.roi_grid_TL = (0, 0, 180, 140)     # 左上广域
        self.roi_grid_TR = (140, 0, 180, 140)   # 右上广域
        self.roi_grid_BL = (0, 100, 180, 140)   # 左下广域
        self.roi_grid_BR = (140, 100, 180, 140) # 右下广域

        # 极速大中心 (比原本的 middle 更大一点，确保护住核心)
        self.roi_center_large = (60, 40, 200, 160)

    def process(self, scan_time_sec=10, target_count=2):
        self.sensor_mgr.init_qr_sensor()

        # === 舵机初始化 ===
        current_angle = -10
        scan_dir = 1
        scan_min = -15
        scan_max = -2
        last_move_time = 0
        MOVE_INTERVAL = 400
        ANGLE_STEP = 1

        self.servo_ctrl.set_angle(current_angle)
        time.sleep_ms(200)

        raw_basket = set()
        start_tick = time.ticks_ms()
        timeout = scan_time_sec * 1000

        # ==================== 3. 混合轮询列表 ====================
        # 策略：你的精细区域 + 我的广域区域穿插进行
        # 既能抓细节，又能防死角，速度还快
        rois_to_poll = [
            # --- 核心区 (概率最高，优先扫) ---
            (self.roi_screen_middle,       "MID_User"), # 你的中间
            (self.roi_center_large,        "MID_Large"),# 我的大中间

            # --- 左侧战术组 ---
            (self.roi_screen_left,         "LEFT"),     # 你的左边
            (self.roi_grid_TL,             "Grid_TL"),  # 左上广域
            (self.roi_screen_letf_bottom,  "L_BOT"),    # 你的左下

            # --- 右侧战术组 ---
            (self.roi_grid_TR,             "Grid_TR"),  # 右上广域
            (self.roi_screen_right_top,    "R_TOP"),    # 你的右上
            (self.roi_grid_BR,             "Grid_BR"),  # 右下广域
            (self.roi_screen_right_bottom, "R_BOT"),    # 你的右下

            # --- 兜底 ---
            (None,                         "FULL")      # 全屏兜底
        ]

        roi_index = 0
        total_rois = len(rois_to_poll)

        print(">> QR: 全覆盖疯狗轮询启动... 列表长度: {}".format(total_rois))

        while time.ticks_ms() - start_tick < timeout:
            # === 舵机自动点头逻辑 ===
            now = time.ticks_ms()
            if now - last_move_time > MOVE_INTERVAL:
                current_angle += (scan_dir * ANGLE_STEP)
                if current_angle <= scan_min:
                    current_angle = scan_min
                    scan_dir = 1
                elif current_angle >= scan_max:
                    current_angle = scan_max
                    scan_dir = -1
                self.servo_ctrl.set_angle(current_angle)
                last_move_time = now
                time.sleep_ms(80)
                continue

            # === 轮询检测 ===
            img = sensor.snapshot()
            current_roi, current_name = rois_to_poll[roi_index]

            # 开启 x_stride=2 极速检测
            if current_roi is None:
                res = img.find_qrcodes(x_stride=2)
            else:
                res = img.find_qrcodes(roi=current_roi, x_stride=2)

            # 调试画框 (绿色)
            if self.debug and current_roi:
                img.draw_rectangle(current_roi, color=(0, 255, 0), thickness=2)

            # 结果处理
            if len(res) > 0:
                for code in res:
                    payload = code.payload()
                    if self.debug:
                        img.draw_rectangle(code.rect(), color=(255, 0, 0), thickness=3)

                    if payload not in raw_basket:
                        raw_basket.add(payload)
                        print(">> [GOT] {}: {}".format(current_name, payload))

            lcd.display(img)

            if len(raw_basket) >= target_count:
                print(">> 二维码已集齐，退出。")
                break

            roi_index = (roi_index + 1) % total_rois

        print(">> 最终结果:", raw_basket)
        self._analyze_and_send(raw_basket)
        self.uart_comm.send_packet(Protocol.CMD_TRAFFIC, Protocol.SUB_TRAFFIC_STOP, 0, 0)

    def _analyze_and_send(self, raw_basket):
        # 保持你原有的解析逻辑不变
        # ... (这里复制你原来的 _analyze_and_send 代码) ...
        # 为了节省篇幅，这里假设你保留了原有的 _analyze_and_send 方法内容
        found_flags = {'WXD': False, 'FHT': False, 'CP': False, 'CK': False, 'LD': False, "SJ": False,
                       'WD':False,'LS1':False,'XY':False,'AF':False,'MY':False,'IQ':False}
        wxd_index = 0
        for raw_data in raw_basket:
            items = StrictValidator.parse_bracket_content(raw_data,['[]','<>','{}']) or [raw_data]
            for content in items:
                print("items:", items)
                if "0x" in content:
                    data_bytes = StrictValidator.hex_str_to_bytes(content)
                    if len(data_bytes) == 6:
                         self.uart_comm.send_var(VarID.MY, data_bytes)
                    elif len(data_bytes) == 4:
                         if wxd_index == 0:
                             self.uart_comm.send_var(VarID.WXD, data_bytes)
                             wxd_index += 1
                         elif wxd_index == 1:
                             self.uart_comm.send_var(VarID.WXD2, data_bytes)
                             wxd_index += 1
                         elif wxd_index == 2:
                             self.uart_comm.send_var(VarID.WXD3, data_bytes)
                else:
                    length = len(content)
                    if length == 4 and not found_flags['WXD']: # 简单示例，根据你的原逻辑补充
                        self.uart_comm.send_var(VarID.WXD, content.encode())
                    elif length == 2:
                         self.uart_comm.send_var(VarID.XY, content.encode())
                    elif length == 6 and not found_flags['AF']:
                         self.uart_comm.send_var(VarID.AF, content.encode())
    fht = None
    def _analyze_and_send(self, raw_basket):
        found_flags = {'WXD': False, 'FHT': False, 'CP': False, 'CK': False, 'LD': False, "SJ": False,
                       'WD':False,'LS1':False,'XY':False,'AF':False,'MY':False,'IQ':False}
        cp_raw = None      # 保存 DF3
        FHT_a = None
        wxd_index = 0
        for raw_data in raw_basket:  #  # 遍历每一条原始扫码字符串
            items = StrictValidator.parse_bracket_content(raw_data,['[]','<>','{}']) or [raw_data] # 兜底处理
            str1 = StrictValidator.parse_bracket_content(raw_data,['[]'])
            # hex_items = re.findall(r'0x[0-9A-Fa-f]+', str1)
            # dec_list = [int(x, 16) for x in hex_items]
            # print(dec_list)
            # for q in range(len(str1)):
            #     if StrictValidator.is_hex(str1[q]):
            #        cp_raw = ''.join(q)
            #print("cp_raw:",cp_raw)
            # print("len_s1:",len(s1))
            # print("s1:",s1)
            #print("items:",str1)
            for content in items:
                print("items:",items)
                if "0x" in content:
                    data_bytes = StrictValidator.hex_str_to_bytes(content)
                    FHT_a = data_bytes
                    if len(FHT_a) == 6:
                        self.uart_comm.send_var(VarID.MY, FHT_a)
                        print("HEX bytes:", FHT_a)
                        print("HEX len:", len(FHT_a))  # 这里一定是 6
                        continue
                    if len(FHT_a) == 4:
                        if wxd_index == 0:
                            print("1")
                            self.uart_comm.send_var(VarID.WXD, FHT_a)
                            print("WXD:", FHT_a)
                            wxd_index += 1
                        elif wxd_index == 1:
                            print("2")
                            self.uart_comm.send_var(VarID.WXD2, FHT_a)
                            print("WXD2:", FHT_a)
                            wxd_index += 1
                        elif wxd_index == 2:
                            print("3")
                            self.uart_comm.send_var(VarID.WXD3, FHT_a)
                            print("WXD3:", FHT_a)
                        continue
                    continue

                length = len(content)
                # if not found_flags['FHT'] and (length == 6):
                #     print("--------------------- VAR_FHT 开始--------------------")
                #     print(">> BAR_FHT内容分析:", fht, "长度:", length)
                #     print("VAR_FHT:",fht)
                #     self.uart_comm.send_var(VarID.FHT,fht.encode())
                #     print(">> FHT数据发送3:",fht)
                #     print("--------------------- VAR_FHT 结束--------------------")
                #     continue
                if not found_flags['WXD'] and (length == 4):
                    print("--------------------- VAR_WXD 开始--------------------")
                    print(">> BAR_WXD内容分析:", content, "长度:", length)
                    print("VAR_WXD:",content)
                    self.uart_comm.send_var(VarID.WXD,content.encode())
                    print(">> FHT数据发送3:",content)
                    print("--------------------- VAR_WXD 结束--------------------")
                    continue
                if not found_flags['XY'] and (length == 2):
                    print(">> BAR_XY内容分析:", content, "长度:", length)
                    self.uart_comm.send_var(VarID.XY,content.encode())
                    found_flags['XY'] = True
                    continue
                if not found_flags['AF'] and (length == 6):
                     print(">> BAR_AF内容分析:", content, "长度:", length)
                     self.uart_comm.send_var(VarID.AF,content.encode())
                     found_flags['AF'] = True
                     continue
                # if not found_flags['IQ'] and (length == 1):
                #      print(">> BAR_XY内容分析:", content, "长度:", length)
                #      self.uart_comm.send_var(VarID.IQ,content.encode())
                #      found_flags['IQ'] = True
                #      continue
                # if not found_flags['LS1'] and (length == 1):
                #     print("--------------------- VAR_LS1 开始--------------------")
                #     print(">> BAR_WXD内容分析:", content, "长度:", length)
                #     n = int(content)
                #     sqrt_val = int(math.sqrt(n))
                #     content = str(sqrt_val)
                #     print("VAR_LS1:",content)
                #     self.uart_comm.send_var(VarID.LS1,content.encode())
                #     print(">> FHT数据发送3:",n)
                #     print("--------------------- VAR_LS1 结束--------------------")
                #     continue
                # if not found_flags['SJ'] and (length == 10):
                #     print("--------------------- VAR_SJ 开始--------------------")
                #     print(">> VAE_SJ 内容分析:", content, "长度:", length)
                #     self.uart_comm.send_var(VarID.SJ, content.encode())
                #     found_flags['SJ'] = True
                #     print(">> SJ数据发送:1", content.encode())
                #     print("--------------------- VAR_SJ 结束--------------------")
                #     continue

                # if not found_flags['WD'] and (length >= 2) and StrictValidator.is_num(content):
                #     print("--------------------- VAR_WD 开始--------------------")
                #     print(">> BAR_WD内容分析:", content, "长度:", length)
                #     self.uart_comm.send_var(VarID.WD, content.encode())
                #     found_flags['WD'] = True
                #     print(">> WD 数据发送:2", content.encode())
                #     print("--------------------- VAR_WD 结束--------------------")
                #     continue
                # raw_cp = list(content)
                # if not found_flags['CP'] and (length == 6) and StrictValidator.is_letter(raw_cp[0]) and  StrictValidator.is_num(raw_cp[1]) and  StrictValidator.is_letter(raw_cp[2]):
                #    print("--------------------- VAR_CP 开始--------------------")
                #    print(">> BAR_CP内容分析:", content, "长度:", length)
                #    print("VAR_CP:",content)
                #    self.uart_comm.send_var(VarID.CP,content.encode())
                #    found_flags['CP'] = True
                #    print(">> CP数据发送3:",content)
                #    print("--------------------- VAR_CP 结束--------------------")
                #    continue

# ============================================================================
#                                  10. 命令处理器
# ============================================================================

class CommandProcessor:
    def __init__(self, servo_ctrl, mode_mgr):
        self.servo_ctrl = servo_ctrl
        self.mode_mgr = mode_mgr
        # 【新增】存储二维码目标数量，默认为2
        self.qr_target_count = 2

        self.handlers = {
            (Protocol.CMD_SERVO, Protocol.SUB_SERVO_SET): self._handle_servo,
            (Protocol.CMD_SERVO, Protocol.SUB_TRACE_START): lambda p1, p2: self.mode_mgr.set_mode(Mode.LINE),
            (Protocol.CMD_SERVO, Protocol.SUB_TRACE_STOP): lambda p1, p2: self.mode_mgr.set_mode(Mode.IDLE),
            (Protocol.CMD_TRAFFIC, Protocol.SUB_TRAFFIC_START): lambda p1, p2: self.mode_mgr.set_mode(Mode.TRAFFIC),
            # 【修改】绑定到专门的 _handle_qr_start 方法
            (Protocol.CMD_TRAFFIC, Protocol.SUB_QR_START): self._handle_qr_start,
            (Protocol.CMD_TRAFFIC, Protocol.SUB_TRAFFIC_STOP): lambda p1, p2: self.mode_mgr.set_mode(Mode.IDLE),
        }

    def _handle_servo(self, p1, p2):
        angle = p2 if p1 == Protocol.SERVO_POSITIVE_FLAG else -p2
        self.servo_ctrl.set_angle(angle)

    # 【新增】处理二维码开始指令
    def _handle_qr_start(self, p1, p2):
        if p1 > 0:
            self.qr_target_count = p1
            print(">> [CMD] 设定二维码扫描数量为:", self.qr_target_count)
        else:
            self.qr_target_count = 2 # 如果收到0，使用默认值
            print(">> [CMD] 使用默认数量:", self.qr_target_count)
        self.mode_mgr.set_mode(Mode.QRCODE)

    def process(self, data):
        if len(data) < 8 or data[0] != Protocol.PKT_HEAD or data[7] != Protocol.PKT_TAIL:
            return False
        cmd, sub, p1, p2 = data[2], data[3], data[4], data[5]
        handler = self.handlers.get((cmd, sub))
        if handler:
            # 【修改】允许 QR_START 指令也接收参数
            if (cmd, sub) == (Protocol.CMD_SERVO, Protocol.SUB_SERVO_SET) or \
               (cmd, sub) == (Protocol.CMD_TRAFFIC, Protocol.SUB_QR_START):
                try:
                    handler(p1, p2)
                except TypeError:
                    handler() # 兼容
            else:
                # 其他不需要参数的 lambda 保持原样调用（如果 lambda 不接收参数的话）
                # 为了安全，这里使用 try/except 或统一 lambda 定义。
                # 由于上面 handlers 定义里 lambda 都加了 p1,p2，直接调用 handler(p1, p2) 也可以
                # 但为了不破坏未修改的逻辑结构，保持这里的调用方式：
                try:
                    handler(p1, p2)
                except TypeError:
                    handler()
            return True
        return False

# ============================================================================
#                                  11. 主控制器
# ============================================================================

class K210Controller:
    """K210主控制器"""
    def __init__(self):
        self._init_hardware()
        self.sensor_mgr = SensorManager(debug=DEBUG_SENSOR)
        self.uart_comm = UARTComm(self.uart, debug=False)
        self.mode_mgr = ModeManager(debug=False)
        self.servo_ctrl = ServoController(self.servo_pwm, debug=DEBUG_SERVO)
        self.traffic_detector = TrafficLightDetector(self.sensor_mgr, self.uart_comm, debug=DEBUG_TRAFFIC)
        self.line_tracker = LineTracker(self.uart_comm, debug=DEBUG_LINE)
        # 传递 servo_ctrl 给二维码处理器，实现自动点头功能
        self.qr_processor = QRCodeProcessor(self.sensor_mgr, self.uart_comm, self.servo_ctrl, debug=True)
        self.cmd_processor = CommandProcessor(self.servo_ctrl, self.mode_mgr)
        self.run_lock = False

    def _init_hardware(self):
        fm.register(6, fm.fpioa.UART1_RX, force=True)
        fm.register(7, fm.fpioa.UART1_TX, force=True)
        fm.register(12, fm.fpioa.GPIO0)
        self.uart = UART(UART.UART1, 115200, read_buf_len=1024)
        self.led_run = GPIO(GPIO.GPIO0, GPIO.OUT)
        self.servo_pwm = PWM(
            Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PWM),
            freq=50, duty=0, pin=17
        )

    def run(self):
        lcd.init()
        self.sensor_mgr.init_gray_sensor()
        self.mode_mgr.set_mode(Mode.IDLE)
        self.servo_ctrl.set_angle(-55)
        print("System Start. Servo -55.")

        while True:
            # 1. 串口命令处理
            if self.uart_comm.has_data() and not self.run_lock:
                data = self.uart_comm.read_packet(8)
                if data:
                    self.cmd_processor.process(data)

            # 2. 交通灯模式
            if self.mode_mgr.is_mode(Mode.TRAFFIC) and not self.run_lock:
                self.run_lock = True
                self.traffic_detector.detect()
                self.run_lock = False

                self.sensor_mgr.init_gray_sensor()
                self.servo_ctrl.set_angle(-55)
                self.mode_mgr.set_mode(Mode.LINE)
                img = sensor.snapshot()
                lcd.display(img)
                continue

            # 3. 二维码模式
            if self.mode_mgr.is_mode(Mode.QRCODE) and not self.run_lock:
                self.run_lock = True

                # 【修改】从 CommandProcessor 获取目标数量，传给二维码处理函数
                target = self.cmd_processor.qr_target_count
                self.qr_processor.process(scan_time_sec = 15, target_count = target)
                self.run_lock = False

                # 扫描结束后，恢复灰度传感器和空闲模式
                self.sensor_mgr.init_gray_sensor()
                self.servo_ctrl.set_angle(-55)
                self.mode_mgr.set_mode(Mode.IDLE)
                img = sensor.snapshot()
                continue

            # 4. 循迹模式
            if self.mode_mgr.is_mode(Mode.LINE) and not self.run_lock:
                self.line_tracker.step()
                continue

            # 5. 空闲模式 (显示带编号的ROI)
            if self.mode_mgr.is_mode(Mode.IDLE):
                img = sensor.snapshot()
                self.line_tracker.draw_rois(img, active_bitmask=0)
                lcd.display(img)
                continue

if __name__ == "__main__":
    try:
        K210Controller().run()
    except Exception as e:
        print("[FATAL]", e)
        try:
            led = GPIO(GPIO.GPIO0, GPIO.OUT)
            while True:
                led.value(1)
                time.sleep_ms(100)
                led.value(0)
                time.sleep_ms(100)
        except:
            pass
