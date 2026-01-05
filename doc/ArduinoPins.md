# Arduino Mega2560 已使用引脚（bkrc_libs/Inc）

| 引脚 | 宏定义             | 所在文件     | 用途说明（中文）       |
| ---- | ------------------ | ------------ | ---------------------- |
| 9    | BEEP_PIN           | BEEP.h       | 蜂鸣器                 |
| 10   | COREBEEP_PIN       | CoreBeep.h   | 核心蜂鸣器             |
| 8    | R_F_M_PIN          | DCMotor.h    | 右电机前进控制         |
| 7    | R_B_M_PIN          | DCMotor.h    | 右电机后退控制         |
| 3    | L_F_M_PIN          | DCMotor.h    | 左电机前进控制         |
| 2    | L_B_M_PIN          | DCMotor.h    | 左电机后退控制         |
| 5    | R_CONTROL_PIN      | DCMotor.h    | 右电机使能/调速（PWM） |
| 6    | L_CONTROL_PIN      | DCMotor.h    | 左电机使能/调速（PWM） |
| 13   | RITX_PIN           | infrare.h    | 红外发射               |
| 12   | LED_PIN_R          | LED.h        | 右侧 LED               |
| 11   | LED_PIN_L          | LED.h        | 左侧 LED               |
| 4    | TRIG_PIN           | Ultrasonic.h | 超声波 TRIG 触发       |
| A15  | ECHO_PIN (PIN_A15) | Ultrasonic.h | 超声波 ECHO 回波       |
| D20  | SDA                | BH1750.h     | 光照传感器SDA          |
| D21  | SCL                | BH1750.h     | 光照传感器SDA          |

## 串口分配说明（Mega2560）

- **Serial（串口0）**：

  - **RX0 = D0**
  - **TX0 = D1**
  - 用途：预留给系统调试（USB 连接电脑串口监视器/日志输出）

  **Serial2（串口2）**：

  - **RX2 = D17**
  - **TX2 = D16**
  - 用途：连接语音模块

  **Serial1**：

  - **RX1 = D19**
  
    **TX1 = D18**
  - 用途：连接视觉模块 K210
