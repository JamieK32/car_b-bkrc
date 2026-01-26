import sensor, image, time, lcd
from Maix import GPIO
from machine import UART, Timer, PWM
from fpioa_manager import fm

# ===================== 简洁日志 =====================
DEBUG = False
def log(*a):
    if DEBUG:
        print("[{}]".format(time.ticks_ms()), *a)

# ===================== 协议 / 命令 =====================
PKT_HEAD_MSB = 0x55
PKT_HEAD_LSB = 0x02
PKT_GATE_CMD = 0x91
PKT_TAIL     = 0xBB

# 按你给的 K210_CMD，STOP 只忽略
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

# ===================== 模式 =====================
MODE_IDLE, MODE_LINE, MODE_TRAFFIC, MODE_QR = 0, 1, 2, 3
mode = MODE_IDLE
run_lock = False

# QR 参数（可按需改）
QR_MAX_SLOTS   = 3
QR_VARID_OFF   = 0

# ===================== UART 打包 =====================
def send_cmd(uart, cmd, d1=0, d2=0):
    chk = (PKT_GATE_CMD + cmd + d1 + d2) & 0xFF
    pkt = bytes([PKT_HEAD_MSB, PKT_HEAD_LSB, PKT_GATE_CMD, cmd, d1, d2, chk, PKT_TAIL])
    uart.write(pkt)

def send_var(uart, var_id, data_bytes):
    # CMD_VAR_LEN: [head][gate][cmd][len][var_id + data...][chk][tail]
    payload = bytes([var_id]) + bytes(data_bytes)
    ln = len(payload)
    chk = (PKT_GATE_CMD + CMD_VAR_LEN + ln + sum(payload)) & 0xFF
    pkt = bytearray([PKT_HEAD_MSB, PKT_HEAD_LSB, PKT_GATE_CMD, CMD_VAR_LEN, ln])
    pkt.extend(payload)
    pkt.append(chk)
    pkt.append(PKT_TAIL)
    uart.write(pkt)
    time.sleep_ms(5)

def read_fixed_pkt(uart):
    if uart.any() < 8:
        return None
    data = uart.read(8)
    if not data or len(data) < 8:
        return None
    if data[0] != PKT_HEAD_MSB or data[1] != PKT_HEAD_LSB:
        return None
    if data[2] != PKT_GATE_CMD or data[7] != PKT_TAIL:
        return None
    chk = (data[2] + data[3] + data[4] + data[5]) & 0xFF
    if chk != data[6]:
        log("CHK mismatch", data)
        return None
    return data

# ===================== 硬件初始化 =====================
def init_hw():
    fm.register(6,  fm.fpioa.UART1_RX, force=True)
    fm.register(7,  fm.fpioa.UART1_TX, force=True)
    fm.register(12, fm.fpioa.GPIO0, force=True)

    uart = UART(UART.UART1, 115200, read_buf_len=1024)
    led  = GPIO(GPIO.GPIO0, GPIO.OUT)

    pwm = PWM(Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PWM),
              freq=50, duty=0, pin=17)
    return uart, led, pwm

# ===================== 舵机 =====================
SERVO_MIN, SERVO_MAX = -90, 40
def servo_set_angle(pwm, angle):
    if angle < SERVO_MIN: angle = SERVO_MIN
    if angle > SERVO_MAX: angle = SERVO_MAX
    pwm.duty((angle + 90) / 18 + 2.5)
    return angle

# ===================== 传感器配置（3套） =====================
def sensor_reset_qvga():
    sensor.reset()
    sensor.set_vflip(1)
    sensor.set_framesize(sensor.QVGA)

def init_line_sensor():
    sensor_reset_qvga()
    sensor.set_pixformat(sensor.GRAYSCALE)
    sensor.set_brightness(-30)
    sensor.set_contrast(2)
    sensor.skip_frames(time=300)

def init_qr_sensor():
    sensor_reset_qvga()
    sensor.set_pixformat(sensor.GRAYSCALE)
    sensor.set_brightness(2)
    sensor.set_contrast(4)
    sensor.set_auto_gain(False, gain_db=12)
    sensor.set_auto_exposure(True)
    sensor.skip_frames(time=500)

def init_rgb_sensor():
    # 交通灯：只切到 RGB 并锁定增益/白平衡
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_brightness(-10)
    sensor.set_auto_gain(False)       # 关自动增益
    sensor.set_auto_exposure(False)   # 关自动曝光(若固件支持)
    time.sleep_ms(50)

# ===================== 循迹（不显示） =====================
LINE_THRESHOLD = (0, 70)
LINE_ROI_Y, LINE_ROI_H = 170, 70
LINE_ROI_W_RATIO = 0.83

def line_step(uart):
    img = sensor.snapshot()
    img = img.rotation_corr(z_rotation=90)
    w = img.width()

    total_w = int(w * LINE_ROI_W_RATIO)
    roi_w = total_w // 8
    start_x = (w - total_w) // 2

    mask = 0
    for i in range(8):
        roi = (start_x + i * roi_w, LINE_ROI_Y, roi_w, LINE_ROI_H)
        if img.find_blobs([LINE_THRESHOLD], roi=roi, pixels_threshold=50):
            mask |= 1 << (7 - i)

    send_cmd(uart, CMD_TRACE_RESULT, mask, 0)

# ===================== 交通灯识别 =====================
COLOR_UNKNOWN, COLOR_RED, COLOR_GREEN, COLOR_YELLOW = 0, 1, 2, 3
TRAFFIC_RED    = [(75, 0, 29, 114, 1, 127)]   # 用第一份
TRAFFIC_GREEN  = [(100, 0, -128, -20, -128, 127)]
TRAFFIC_YELLOW = [(80, 60, -10, 127, 10, 127)]
TRAFFIC_ROI    = (0, 0, 320, 120)
TRAFFIC_TIMEOUT_MS = 1000

def detect_traffic(uart):
    init_rgb_sensor()
    votes = [0, 0, 0, 0]
    start = time.ticks_ms()

    for _ in range(5):
        if time.ticks_diff(time.ticks_ms(), start) > TRAFFIC_TIMEOUT_MS:
            result = COLOR_YELLOW  # 你的原逻辑：超时给黄
            send_cmd(uart, CMD_TRAFFIC_RESULT, result, 0)
            return result

        img = sensor.snapshot()
        r = img.find_blobs(TRAFFIC_RED,    roi=TRAFFIC_ROI, pixels_threshold=150)
        g = img.find_blobs(TRAFFIC_GREEN,  roi=TRAFFIC_ROI, pixels_threshold=150)
        y = img.find_blobs(TRAFFIC_YELLOW, roi=TRAFFIC_ROI, pixels_threshold=150)

        rp = max((b.pixels() for b in r), default=0)
        gp = max((b.pixels() for b in g), default=0)
        yp = max((b.pixels() for b in y), default=0) * 1.5  # 保持你原来的偏置

        if yp > rp and yp > gp and yp > 200:
            votes[COLOR_YELLOW] += 1
        elif gp > rp and gp > 200:
            votes[COLOR_GREEN] += 1
        elif rp > 300:
            votes[COLOR_RED] += 1
        else:
            votes[COLOR_UNKNOWN] += 1

    result = votes.index(max(votes)) if max(votes) >= 2 else COLOR_UNKNOWN
    send_cmd(uart, CMD_TRAFFIC_RESULT, result, 0)
    log("TRAFFIC votes", votes, "->", result)
    return result

# ===================== QR + 舵机扫描 + 槽位映射 =====================
def qr_process(uart, pwm, target_count=2, timeout_s=20):
    init_qr_sensor()

    # 槽位映射：payload -> slot（旧数据复用旧槽位；新数据占新槽位直到满）
    payload2slot = {}
    next_slot = 0

    # 舵机扫描参数（保留你的“点头”思路，略微简化）
    angle = -9
    scan_dir = 1
    scan_min, scan_max = 3, 8
    last_move = time.ticks_ms()
    MOVE_INTERVAL = 100
    ANGLE_STEP = 0.2

    t0 = time.ticks_ms()
    t_end = timeout_s * 1000

    while time.ticks_diff(time.ticks_ms(), t0) < t_end:
        now = time.ticks_ms()

        # 舵机微动
        if time.ticks_diff(now, last_move) > MOVE_INTERVAL:
            angle += scan_dir * ANGLE_STEP
            if angle <= scan_min:
                angle = scan_min
                scan_dir = 1
            elif angle >= scan_max:
                angle = scan_max
                scan_dir = -1
            servo_set_angle(pwm, angle)
            last_move = now

        img = sensor.snapshot()
        res = img.find_qrcodes(x_stride=1)

        if res:
            for code in res:
                s = str(code.payload()).strip()
                if not s:
                    continue

                # 分配/复用槽位（不做“去重不发送”，检测到就发，但槽位固定）
                if s in payload2slot:
                    slot = payload2slot[s]
                else:
                    if next_slot >= QR_MAX_SLOTS:
                        continue
                    slot = next_slot
                    payload2slot[s] = slot
                    next_slot += 1

                var_id = QR_VARID_OFF + slot
                send_var(uart, var_id, s.encode())
                log("QR send slot", slot, "varid", var_id, "data", s)

        # 达到目标数量就退出（与你原逻辑一致）
        if len(payload2slot) >= target_count:
            break

    return payload2slot

# ===================== 命令处理（STOP 全忽略） =====================
qr_target_count = 2
qr_timeout_s = 20

def handle_cmd(pkt, pwm):
    global mode, qr_target_count, qr_timeout_s

    cmd = pkt[3]
    d1  = pkt[4]
    d2  = pkt[5]

    if cmd == CMD_SET_SERVO_POSITIVE:
        servo_set_angle(pwm, d1)
        return

    if cmd == CMD_SET_SERVO_NEGATIVE:
        servo_set_angle(pwm, -d1)
        return

    if cmd == CMD_TRACE_START:
        mode = MODE_LINE
        return

    if cmd == CMD_TRAFFIC_START:
        mode = MODE_TRAFFIC
        return

    if cmd == CMD_QR_CODE_START:
        qr_target_count = d1 if d1 > 0 else 2
        qr_timeout_s = d2 if d2 > 0 else 20
        mode = MODE_QR
        return

    # STOP 命令全部忽略
    if cmd in (CMD_TRACE_STOP, CMD_TRAFFIC_STOP, CMD_QR_CODE_STOP):
        return

# ===================== 主循环 =====================
def main():
    global mode, run_lock

    lcd.init()
    uart, led, pwm = init_hw()

    init_line_sensor()
    mode = MODE_IDLE

    servo_set_angle(pwm, -45)

    while True:
        pkt = read_fixed_pkt(uart)
        if pkt and (not run_lock):
            handle_cmd(pkt, pwm)

        if mode == MODE_TRAFFIC and not run_lock:
            run_lock = True
            detect_traffic(uart)
            run_lock = False

            init_line_sensor()
            mode = MODE_LINE

        elif mode == MODE_QR and not run_lock:
            run_lock = True
            qr_process(uart, pwm, target_count=qr_target_count, timeout_s=qr_timeout_s)
            run_lock = False

            init_line_sensor()
            mode = MODE_IDLE

        elif mode == MODE_LINE and not run_lock:
            line_step(uart)

        else:
            lcd.display(sensor.snapshot())

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("FATAL:", e)
        try:
            led = GPIO(GPIO.GPIO0, GPIO.OUT)
            while True:
                led.value(1); time.sleep_ms(100)
                led.value(0); time.sleep_ms(100)
        except:
            pass
