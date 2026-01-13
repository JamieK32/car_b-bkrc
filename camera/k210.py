import sensor, image, time, lcd
from Maix import GPIO
from machine import UART, Timer, PWM
from fpioa_manager import fm

# =============================================================================
# 0) Unified Debug Logger (简洁统一版)
# =============================================================================
# 总开关：False = 全部调试输出静音；True = 允许按 TAG 输出
LOG_ALL = True

# 分模块开关（你以前的 DEBUG_xxx 统一到这里）
LOG_TAG = {
    "TRAFFIC": False,
    "SERVO":   False,
    "SENSOR":  False,
    "LINE":    False,
    "QR":      False,
    "UART":    False,
    "MODE":    False,
}

def _hex_bytes(data):
    if not data:
        return ""
    return " ".join("{:02X}".format(b) for b in data)

def log(tag, msg, always=False):
    """统一日志输出：总开关 + 分 TAG 开关 + 可强制输出(always)."""
    if always or (LOG_ALL and LOG_TAG.get(tag, False)):
        print("[{:>8}] [{}] {}".format(time.ticks_ms(), tag, msg))

def set_log_all(enable: bool):
    global LOG_ALL
    LOG_ALL = bool(enable)

def set_log_tag(tag: str, enable: bool):
    LOG_TAG[tag] = bool(enable)

# =============================================================================
# 2) Protocol / Mode
# =============================================================================
class Protocol:
    PKT_HEAD_MSB = 0x55
    PKT_HEAD_LSB = 0x02
    PKT_GATE_CMD = 0x91
    PKT_TAIL     = 0xBB

    CMD_VAR_LEN            = 0x03
    CMD_SET_SERVO_POSITIVE = 0x04
    CMD_SET_SERVO_NEGATIVE = 0x05
    CMD_TRACE_START        = 0x06
    CMD_TRACE_STOP         = 0x07
    CMD_TRACE_RESULT       = 0x08
    CMD_TRAFFIC_START      = 0x09
    CMD_TRAFFIC_STOP       = 0x0A
    CMD_TRAFFIC_RESULT     = 0x0B
    CMD_QR_CODE_START      = 0x0C
    CMD_QR_CODE_STOP       = 0x0D

class Mode:
    IDLE, LINE, TRAFFIC, QRCODE = 0, 1, 2, 3
    NAMES = ("IDLE", "LINE", "TRAFFIC", "QRCODE")

# =============================================================================
# 3) Sensor Manager (精简：灰度 / RGB 两条主线)
# =============================================================================
class SensorManager:
    def __init__(self, debug=False):
        self.debug = debug

    def _reset_qvga(self):
        sensor.reset()
        sensor.set_vflip(1)
        sensor.set_framesize(sensor.QVGA)

    def _init_gray(self, *, brightness, contrast, auto_gain=None, gain_db=None, auto_exposure=None, skip_ms=300):
        self._reset_qvga()
        sensor.set_pixformat(sensor.GRAYSCALE)

        if auto_gain is not None:
            if gain_db is None:
                sensor.set_auto_gain(auto_gain)
            else:
                sensor.set_auto_gain(auto_gain, gain_db=gain_db)

        if auto_exposure is not None:
            sensor.set_auto_exposure(auto_exposure)

        sensor.set_brightness(brightness)
        sensor.set_contrast(contrast)
        sensor.skip_frames(time=skip_ms)

        log("SENSOR", "GRAY init b={} c={}".format(brightness, contrast), always=False)

    def init_gray_sensor(self):
        # 循迹
        self._init_gray(brightness=-30, contrast=2, skip_ms=300)

    def init_qr_sensor(self):
        # 二维码
        self._init_gray(
            brightness=2, contrast=4,
            auto_gain=False, gain_db=12,
            auto_exposure=True,
            skip_ms=500
        )

    def init_rgb_sensor(self):
        # 交通灯：不 reset（沿用主摄像头配置，只切格式/曝光等）
        sensor.set_pixformat(sensor.RGB565)
        sensor.set_brightness(-10)
        sensor.set_auto_gain(False)
        sensor.set_auto_whitebal(False)
        log("SENSOR", "RGB init", always=False)

# =============================================================================
# 4) UART Comm (精简：打包集中)
# =============================================================================
class UARTComm:
    def __init__(self, uart_obj, debug=False):
        self.uart = uart_obj
        self.debug = debug

    def _write(self, pkt, tag):
        if self.debug:
            log("UART", "{} {}".format(tag, _hex_bytes(pkt)))
        self.uart.write(pkt)

    def send_cmd(self, cmd, d1=0, d2=0):
        gate = Protocol.PKT_GATE_CMD
        chk = (gate + cmd + d1 + d2) & 0xFF
        pkt = bytes([
            Protocol.PKT_HEAD_MSB, Protocol.PKT_HEAD_LSB,
            gate, cmd, d1, d2, chk, Protocol.PKT_TAIL
        ])
        self._write(pkt, "TX FIXED")

    def send_var(self, var_id, data_bytes: bytes):
        gate = Protocol.PKT_GATE_CMD
        cmd  = Protocol.CMD_VAR_LEN

        payload = bytes([var_id]) + bytes(data_bytes)
        ln = len(payload)

        pkt = bytearray([Protocol.PKT_HEAD_MSB, Protocol.PKT_HEAD_LSB, gate, cmd, ln])
        pkt.extend(payload)

        chk = (gate + cmd + ln + sum(payload)) & 0xFF
        pkt.append(chk)
        pkt.append(Protocol.PKT_TAIL)

        self._write(pkt, "TX VAR")
        time.sleep_ms(5)

    def has_data(self):
        return self.uart.any()

    def read_packet(self, length=8):
        if not self.uart.any():
            return None
        return self.uart.read(length)

# =============================================================================
# 5) Mode Manager
# =============================================================================
class ModeManager:
    def __init__(self, debug=False):
        self.current_mode = Mode.IDLE
        self.debug = debug

    def set_mode(self, new_mode):
        if self.debug:
            log("MODE", "{} -> {}".format(Mode.NAMES[self.current_mode], Mode.NAMES[new_mode]))
        self.current_mode = new_mode

    def is_mode(self, mode):
        return self.current_mode == mode

# =============================================================================
# 6) Servo Controller
# =============================================================================
class ServoController:
    def __init__(self, pwm_obj, min_angle=-90, max_angle=40, debug=False):
        self.pwm = pwm_obj
        self.min_angle = min_angle
        self.max_angle = max_angle
        self.debug = debug

    def set_angle(self, angle):
        angle = max(self.min_angle, min(self.max_angle, angle))
        self.pwm.duty((angle + 90) / 18 + 2.5)
        if self.debug:
            log("SERVO", "Angle {}".format(angle))
        return angle

# =============================================================================
# 7) Traffic Light Detector
# =============================================================================
class TrafficLightDetector:
    COLOR_UNKNOWN, COLOR_RED, COLOR_GREEN, COLOR_YELLOW = 0, 1, 2, 3

    TRAFFIC_RED    = [(75, 26, 29, 114, 1, 127)]
    TRAFFIC_GREEN  = [(100, 0, -128, -20, -128, 127)]
    TRAFFIC_YELLOW = [(80, 60, -10, 127, 10, 127)]
    ROI = (0, 0, 320, 120)

    TIMEOUT_MS = 1000

    def __init__(self, sensor_mgr, uart_comm, debug=False):
        self.sensor_mgr = sensor_mgr
        self.uart_comm  = uart_comm
        self.debug      = debug

    def detect(self):
        self.sensor_mgr.init_rgb_sensor()

        votes = [0, 0, 0, 0]
        start = time.ticks_ms()

        for _ in range(5):
            if time.ticks_diff(time.ticks_ms(), start) > self.TIMEOUT_MS:
                result = self.COLOR_YELLOW
                self.uart_comm.send_cmd(Protocol.CMD_TRAFFIC_RESULT, result, 0)
                return result

            img = sensor.snapshot()
            r = img.find_blobs(self.TRAFFIC_RED,    roi=self.ROI, pixels_threshold=150)
            g = img.find_blobs(self.TRAFFIC_GREEN,  roi=self.ROI, pixels_threshold=150)
            y = img.find_blobs(self.TRAFFIC_YELLOW, roi=self.ROI, pixels_threshold=150)

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
        self.uart_comm.send_cmd(Protocol.CMD_TRAFFIC_RESULT, result, 0)
        if self.debug:
            log("TRAFFIC", "votes={} -> {}".format(votes, result))
        return result

# =============================================================================
# 8) Line Tracker
# =============================================================================
class LineTracker:
    # 统一灰度阈值：0~72 认为是线(深色)
    LINE_THRESHOLD = (0, 100)

    def __init__(self, uart_comm, debug=False, roi_y=170, roi_h=70, roi_width_ratio=0.75):
        self.uart_comm = uart_comm
        self.debug = debug
        self.roi_y = roi_y
        self.roi_h = roi_h
        self.roi_width_ratio = roi_width_ratio

    def _rois(self, w):
        total_w = int(w * self.roi_width_ratio)   # 保留缩放：总ROI宽度占比
        roi_w = total_w // 8
        start_x = (w - total_w) // 2
        return [(start_x + i * roi_w, self.roi_y, roi_w, self.roi_h) for i in range(8)]

    def _draw_rois(self, img, rois, mask):
        for i, r in enumerate(rois):
            active = (mask >> (7 - i)) & 1
            img.draw_rectangle(r, color=(255 if active else 127), thickness=(2 if active else 1))
            img.draw_string(r[0] + 3, r[1] + 5, str(i), scale=1.5, color=255)

    def step(self):
        img = sensor.snapshot()
        rois = self._rois(img.width())

        mask = 0
        for i, roi in enumerate(rois):
            # GRAYSCALE 下 find_blobs 需要 thresholds 是 list：[(min,max)]
            if img.find_blobs([self.LINE_THRESHOLD], roi=roi, pixels_threshold=50):
                mask |= 1 << (7 - i)

        self.uart_comm.send_cmd(Protocol.CMD_TRACE_RESULT, mask, 0)
        self._draw_rois(img, rois, mask)
        lcd.display(img)

        if self.debug:
            log("LINE", "Bitmask: 0x{:02X}".format(mask))


# =============================================================================
# 9) QR Code Processor (保留槽位发送 + 舵机点头 + ROI轮询)
# =============================================================================
class QRCodeProcessor:
    def __init__(self, sensor_mgr, uart_comm, servo_ctrl, debug=False, max_slots=3, varid_offset=0):
        self.sensor_mgr = sensor_mgr
        self.uart_comm  = uart_comm
        self.servo_ctrl = servo_ctrl
        self.debug      = debug

        # UART 槽位管理逻辑
        self.max_slots = max_slots
        self.varid_offset = varid_offset
        self.payload_to_slot = {}
        self.next_slot = 0

    def reset_slots(self):
        """重置已识别的二维码记录，为下一次扫描做准备"""
        self.payload_to_slot.clear()
        self.next_slot = 0
        if self.debug:
            log("QR", "slots cleared")

    def _send_payload_with_slot(self, payload):
        """将识别结果分配槽位并通过 UART 发送"""
        s = str(payload).strip()
        if not s:
            return

        # 如果是已识别过的，获取原有槽位；否则分配新槽位
        if s in self.payload_to_slot:
            slot = self.payload_to_slot[s]
        else:
            if self.next_slot >= self.max_slots:
                if self.debug:
                    log("QR", "slots full, ignore: {}".format(s))
                return
            slot = self.next_slot
            self.payload_to_slot[s] = slot
            self.next_slot += 1

        # 计算 VarID 并通过串口发送协议包
        var_id = slot + self.varid_offset
        self.uart_comm.send_var(var_id, s.encode())

        if self.debug:
            log("QR", "TX -> slot:{} varid:{} data:{}".format(slot, var_id, s))

    def process(self, scan_time_sec=20, target_count=2):
        """主处理循环：包含传感器初始化、舵机点头及识别发送"""
        # 1. 初始化传感器 (请确保 SensorManager 已包含你满意的参数)
        self.sensor_mgr.init_qr_sensor()
        self.reset_slots()

        # 2. 舵机点头参数控制
        angle = -9
        scan_dir = 1
        scan_min, scan_max = 3, 8
        last_move = 0
        MOVE_INTERVAL = 100
        ANGLE_STEP = 0.2

        start = time.ticks_ms()
        timeout = scan_time_sec * 1000

        if self.debug:
            log("QR", "Process start. Target:{}".format(target_count))

        while time.ticks_ms() - start < timeout:
            now = time.ticks_ms()

            # 3. 舵机微动逻辑：增加捕捉角度
            if now - last_move > MOVE_INTERVAL:
                angle += scan_dir * ANGLE_STEP
                if angle <= scan_min:
                    angle = scan_min
                    scan_dir = 1
                elif angle >= scan_max:
                    angle = scan_max
                    scan_dir = -1
                self.servo_ctrl.set_angle(angle)
                last_move = now
                time.sleep_ms(1)
                continue

            # 4. 图像获取与识别
            img = sensor.snapshot()

            # 采用固定 ROI 提高帧率和成功率
            res = img.find_qrcodes(x_stride=1)

            if res:
                for code in res:
                    payload = code.payload()
                    img.draw_rectangle(code.rect(), color=(255, 0, 0), thickness=3)
                    # 分配槽位并发送数据
                    self._send_payload_with_slot(payload)

            # 5. 显示画面
            lcd.display(img)

            # 6. 判定退出条件
            if len(self.payload_to_slot) >= target_count:
                if self.debug:
                    log("QR", "Target reached, exiting...")
                break

        if self.debug:
            log("QR", "Final Results: {}".format(self.payload_to_slot))

        return self.payload_to_slot

# =============================================================================
# 10) Command Processor
# =============================================================================
class CommandProcessor:
    def __init__(self, servo_ctrl, mode_mgr):
        self.servo_ctrl = servo_ctrl
        self.mode_mgr   = mode_mgr
        self.qr_target_count = 2
        self.qr_timeout_s = 20
        self.handlers = {
            Protocol.CMD_SET_SERVO_POSITIVE: self._servo_pos,
            Protocol.CMD_SET_SERVO_NEGATIVE: self._servo_neg,

            Protocol.CMD_TRACE_START:   self._trace_start,
            Protocol.CMD_TRACE_STOP:    self._idle,

            Protocol.CMD_TRAFFIC_START: self._traffic_start,
            Protocol.CMD_TRAFFIC_STOP:  self._idle,

            Protocol.CMD_QR_CODE_START: self._qr_start,
            Protocol.CMD_QR_CODE_STOP:  self._idle,
        }

    def _servo_pos(self, angle_abs):
        self.servo_ctrl.set_angle(angle_abs)

    def _servo_neg(self, angle_abs):
        self.servo_ctrl.set_angle(-angle_abs)

    def _trace_start(self):
        self.mode_mgr.set_mode(Mode.LINE)

    def _traffic_start(self):
        self.mode_mgr.set_mode(Mode.TRAFFIC)

    def _idle(self):
        self.mode_mgr.set_mode(Mode.IDLE)

    def _qr_start(self, target_count, timeout_s):
        self.qr_target_count = target_count if target_count > 0 else 2
        self.qr_timeout_s = timeout_s if timeout_s > 0 else 20  # ✅ d2=0 就用默认 20s
        log("QR", "CMD target_count={} timeout_s={}".format(self.qr_target_count, self.qr_timeout_s), always=True)
        self.mode_mgr.set_mode(Mode.QRCODE)

    def process(self, data: bytes):
        if (not data) or len(data) < 8:
            return False

        if data[0] != Protocol.PKT_HEAD_MSB: return False
        if data[1] != Protocol.PKT_HEAD_LSB: return False
        if data[2] != Protocol.PKT_GATE_CMD: return False
        if data[7] != Protocol.PKT_TAIL:     return False

        chk = (data[2] + data[3] + data[4] + data[5]) & 0xFF
        if chk != data[6]:
            log("UART", "CHK mismatch got={} calc={}".format(hex(data[6]), hex(chk)), always=True)
            return False

        cmd = data[3]
        d1  = data[4]
        d2  = data[5]   # ✅ 新增
        fn = self.handlers.get(cmd)
        if not fn:
            return False

        if cmd in (Protocol.CMD_SET_SERVO_POSITIVE,
                   Protocol.CMD_SET_SERVO_NEGATIVE):
            fn(d1)
        elif cmd == Protocol.CMD_QR_CODE_START:
            fn(d1, d2)   # ✅ 把 timeout_s 传进去
        else:
            fn()

        return True

# =============================================================================
# 11) Main Controller
# =============================================================================
class K210Controller:
    def __init__(self):
        self._init_hw()

        self.sensor_mgr = SensorManager(debug=LOG_TAG.get("SENSOR", False))
        self.uart_comm  = UARTComm(self.uart, debug=LOG_TAG.get("UART", False))
        self.mode_mgr   = ModeManager(debug=LOG_TAG.get("MODE", False))

        self.servo_ctrl = ServoController(self.servo_pwm, debug=LOG_TAG.get("SERVO", False))
        self.traffic    = TrafficLightDetector(self.sensor_mgr, self.uart_comm, debug=LOG_TAG.get("TRAFFIC", False))
        self.line       = LineTracker(self.uart_comm, debug=LOG_TAG.get("LINE", False))
        self.qr         = QRCodeProcessor(self.sensor_mgr, self.uart_comm, self.servo_ctrl, debug=LOG_TAG.get("QR", False))

        self.cmd        = CommandProcessor(self.servo_ctrl, self.mode_mgr)
        self.run_lock   = False

    def _init_hw(self):
        fm.register(6,  fm.fpioa.UART1_RX, force=True)
        fm.register(7,  fm.fpioa.UART1_TX, force=True)
        fm.register(12, fm.fpioa.GPIO0)

        self.uart = UART(UART.UART1, 115200, read_buf_len=1024)
        self.led_run = GPIO(GPIO.GPIO0, GPIO.OUT)

        self.servo_pwm = PWM(
            Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PWM),
            freq=50, duty=0, pin=17
        )

    def _handle_uart(self):
        if self.run_lock or (not self.uart_comm.has_data()):
            return
        data = self.uart_comm.read_packet(8)
        if not data:
            return
        if LOG_ALL and LOG_TAG.get("UART", False):
            log("UART", "RX len={} lock={} data={}".format(len(data), self.run_lock, _hex_bytes(data)))
        self.cmd.process(data)

    def run(self):
        lcd.init()
        self.sensor_mgr.init_gray_sensor()
        self.mode_mgr.set_mode(Mode.IDLE)

        while True:
            self._handle_uart()

            mode = self.mode_mgr.current_mode

            if mode == Mode.TRAFFIC and not self.run_lock:
                self.run_lock = True
                self.traffic.detect()
                self.run_lock = False

                self.sensor_mgr.init_gray_sensor()
                self.mode_mgr.set_mode(Mode.LINE)
                lcd.display(sensor.snapshot())

            elif mode == Mode.QRCODE and not self.run_lock:
                self.run_lock = True
                target = self.cmd.qr_target_count
                timeout_s = self.cmd.qr_timeout_s
                self.qr.process(scan_time_sec=timeout_s, target_count=target)
                self.run_lock = False

                self.sensor_mgr.init_gray_sensor()
                self.mode_mgr.set_mode(Mode.IDLE)
                lcd.display(sensor.snapshot())

            elif mode == Mode.LINE and not self.run_lock:
                self.line.step()

            else:
                lcd.display(sensor.snapshot())

if __name__ == "__main__":
    try:
        K210Controller().run()
    except Exception as e:
        log("FATAL", str(e), always=True)
        try:
            led = GPIO(GPIO.GPIO0, GPIO.OUT)
            while True:
                led.value(1); time.sleep_ms(100)
                led.value(0); time.sleep_ms(100)
        except:
            pass
