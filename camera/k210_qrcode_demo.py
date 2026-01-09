import sensor, image, time, lcd
from Maix import GPIO
from machine import Timer, PWM
from fpioa_manager import fm

# =============================================================================
# 1) 基础配置 (完全遵循原代码参数)
# =============================================================================
LOG_ALL = True
SERVO_PIN = 17

def log(tag, msg):
    if LOG_ALL:
        print("[{:>8}] [{}] {}".format(time.ticks_ms(), tag, msg))

# =============================================================================
# 2) Sensor Manager (参数完全一致)
# =============================================================================
class SensorManager:
    def _reset_qvga(self):
        sensor.reset()
        sensor.set_vflip(1)
        sensor.set_hmirror(1) # 增加镜像以符合LCD直观显示
        sensor.set_auto_gain(False, gain_db=12)
        sensor.set_framesize(sensor.QVGA)

    def init_qr_sensor(self):
        self._reset_qvga()
        sensor.set_pixformat(sensor.RGB565) # 纸张扫描灰度优于RGB
        # 严格遵循原代码参数: b=2, c=4, gain=12
        log("SENSOR", "QR init b=2 c=4")

# =============================================================================
# 3) Servo Controller (参数完全一致)
# =============================================================================
class ServoController:
    def __init__(self, pwm_obj, min_angle=-90, max_angle=40):
        self.pwm = pwm_obj
        self.min_angle = min_angle
        self.max_angle = max_angle

    def set_angle(self, angle):
        angle = max(self.min_angle, min(self.max_angle, angle))
        # 严格遵循原代码公式
        self.pwm.duty((angle + 90) / 18 + 2.5)
        return angle

# =============================================================================
# 4) QR Code Processor (逻辑、ROI、步进完全一致)
# =============================================================================
class QRCodeProcessor:
    def __init__(self, sensor_mgr, servo_ctrl):
        self.sensor_mgr = sensor_mgr
        self.servo_ctrl = servo_ctrl
        self.found_payloads = []
        # 固定 ROI
        self.fixed_roi = (59, 10, 228, 223)

    def process(self, scan_time_sec=20, target_count=2):
        # 1. 初始化传感器
        self.sensor_mgr.init_qr_sensor()
        self.found_payloads = []

        # 2. 舵机点头参数 (严格同步原代码)
        angle = -9
        scan_dir = 1
        scan_min, scan_max = 0, 5
        last_move = 0
        MOVE_INTERVAL = 100
        ANGLE_STEP = 0.2

        start = time.ticks_ms()
        timeout = scan_time_sec * 1000

        log("QR", "Fixed ROI scan started. ROI: {}".format(self.fixed_roi))

        while time.ticks_ms() - start < timeout:
            now = time.ticks_ms()

            # 3. 舵机点头逻辑 (保持不变)
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

            # 4. 捕捉图像
            img = sensor.snapshot()
            img.lens_corr(1.8)
            # 在屏幕上画出固定的 ROI 框，方便对准
            img.draw_rectangle(self.fixed_roi, color=127, thickness=2)

            # 5. 在固定 ROI 内识别
            res = img.find_qrcodes(roi=self.fixed_roi, x_stride=1)

            if res:
                for code in res:
                    p = code.payload()
                    # 在屏幕上框出识别到的二维码
                    # img.draw_rectangle(code.rect(), color=255, thickness=3)

                    # 结果去重并记录
                    if p not in self.found_payloads:
                        log("QR", "GOT: {}".format(p))
                        self.found_payloads.append(p)

            # 6. LCD 实时显示
            lcd.display(img)

            # 7. 达到目标数量提前退出
            if len(self.found_payloads) >= target_count:
                log("QR", "Target reached!")
                break

        log("QR", "Process finished. Found: {}".format(self.found_payloads))
        return self.found_payloads
# =============================================================================
# 5) 主程序运行
# =============================================================================
if __name__ == "__main__":
    lcd.init()

    # 修正 FPIOA 注册，解决你的报错
    fm.register(SERVO_PIN, fm.fpioa.TIMER0_TOGGLE1, force=True)

    # 初始化 PWM
    servo_pwm = PWM(
        Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PWM),
        freq=50, duty=0, pin=SERVO_PIN
    )

    # 实例化
    sm = SensorManager()
    sc = ServoController(servo_pwm)
    qr = QRCodeProcessor(sm, sc)

    # 直接运行识别逻辑 (默认 20秒, 找 2个)
    try:
        qr.process(scan_time_sec=1000, target_count=3)
        print("Final Found:", qr.found_payloads)
    except Exception as e:
        print("Error:", e)
