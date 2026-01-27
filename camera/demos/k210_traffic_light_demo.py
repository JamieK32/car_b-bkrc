import sensor, image, time, lcd
from Maix import GPIO
from machine import Timer, PWM
from fpioa_manager import fm

# ===================== Debug =====================
DEBUG = True
def log(*a):
    if DEBUG:
        print("[{}]".format(time.ticks_ms()), *a)

# ===================== Servo =====================
SERVO_MIN, SERVO_MAX = -90, 40

def servo_init():
    # PWM pin=17（与你原工程一致）
    pwm = PWM(
        Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PWM),
        freq=50, duty=0, pin=17
    )
    return pwm

def servo_set_angle(pwm, angle):
    if angle < SERVO_MIN: angle = SERVO_MIN
    if angle > SERVO_MAX: angle = SERVO_MAX
    pwm.duty((angle + 90) / 18 + 2.5)
    return angle

# ===================== Sensor Init =====================
def sensor_reset_qvga():
    sensor.reset()
    sensor.set_vflip(1)
    sensor.set_framesize(sensor.QVGA)

def init_rgb_sensor():
    # 交通灯：RGB + 锁增益/白平衡（保持你原来的参数）
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_brightness(-10)
    sensor.set_auto_gain(False)       # 关自动增益
    sensor.set_auto_exposure(False)   # 关自动曝光(若固件支持)
    time.sleep_ms(50)

# ===================== Traffic Light Detect =====================
COLOR_UNKNOWN, COLOR_RED, COLOR_GREEN, COLOR_YELLOW = 0, 1, 2, 3

TRAFFIC_RED = [(75, 0, 29, 114, 1, 127)]
TRAFFIC_GREEN  = [(100, 0, -128, -20, -128, 127)]
TRAFFIC_YELLOW = [(80, 60, -10, 127, 10, 127)]
TRAFFIC_ROI    = (0, 0, 320, 120)
TRAFFIC_TIMEOUT_MS = 1000

def detect_traffic():
    """
    ✅ 保持你原 detect() 的核心逻辑：
    - init_rgb_sensor()
    - 5 次投票
    - 超时返回黄灯
    - yp = max(y)*1.5
    - 阈值规则完全一致
    """
    init_rgb_sensor()

    votes = [0, 0, 0, 0]
    start = time.ticks_ms()

    for _ in range(5):
        if time.ticks_diff(time.ticks_ms(), start) > TRAFFIC_TIMEOUT_MS:
            return COLOR_YELLOW

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
    return result

def color_name(c):
    return ("UNK", "RED", "GRN", "YEL")[c] if 0 <= c <= 3 else "?"

# ===================== Main Demo =====================
def main():
    lcd.init()

    # 摄像头初始化（QVGA）
    sensor_reset_qvga()
    init_rgb_sensor()

    # 舵机固定 15 度（不再晃动）
    pwm = servo_init()
    fixed = servo_set_angle(pwm, 15)
    log("servo fixed angle =", fixed)

    # 控制交通灯检测频率（每 200ms 做一次完整检测：5帧投票）
    DETECT_INTERVAL_MS = 200
    last_detect = 0
    last_result = -1

    while True:
        now = time.ticks_ms()

        if time.ticks_diff(now, last_detect) > DETECT_INTERVAL_MS:
            res = detect_traffic()
            log("traffic:", color_name(res))
            last_result = res

        # 画面显示（方便你观察；不需要就删掉这两行）
        img = sensor.snapshot()
        lcd.display(img)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("FATAL:", e)
        try:
            fm.register(12, fm.fpioa.GPIO0, force=True)
            led = GPIO(GPIO.GPIO0, GPIO.OUT)
            while True:
                led.value(1); time.sleep_ms(100)
                led.value(0); time.sleep_ms(100)
        except:
            pass
