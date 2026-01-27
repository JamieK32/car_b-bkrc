// 百科荣创官方库
#include "DCMotor.h"
#include "CoreLED.h"
#include "CoreBeep.h"
#include "ExtSRAMInterface.h"
#include "LED.h"
#include "BH1750.h"
#include "Command.h"
#include "BEEP.h"
#include "Drive.h"
#include "infrare.h"
#include "BKRC_Voice.h"

// C库
#include "string.h"
#include "math.h"

// 自己的库
#include "carb_config.h"
#include "car_controller.hpp"
#include "log.hpp"
#include "k210_protocol.hpp"
#include "k210_app.hpp"
#include "lsm6dsv16x.h"
#include "qrcode_datamap.hpp"
#include "algorithm.hpp"
#include "vehicle_exchange.h"
#include "street_light.hpp"
#include "zigbee_driver.hpp"

#include "Display3D.hpp"
#include "multi_button.h"
#include "button_user.h"
#include "qrcode_datamap.hpp"
#include "ultrasonic.hpp"
#include "vehicle_exchange_v2.h"
#include "path_executor.h"

// 各种演示
#include "qrcode_demos.hpp"

#define LOG_MAIN_EN 1

#if LOG_MAIN_EN
#define log_main(fmt, ...) LOG_P("[MAIN] " fmt "\r\n", ##__VA_ARGS__)
#define log_printf(fmt, ...) LOG_P(fmt, ##__VA_ARGS__)
#define alarm_fail()               \
    do                             \
    {                              \
        BEEP.ToggleNTimes(3, 120); \
    } while (0)
#define alarm_log1()               \
    do                             \
    {                              \
        BEEP.ToggleNTimes(1, 120); \
    } while (0)
#define alarm_log2()               \
    do                             \
    {                              \
        BEEP.ToggleNTimes(2, 120); \
    } while (0)
#else
#define log_main(...) \
    do                \
    {                 \
    } while (0)
#define alarm_fail() \
    do               \
    {                \
    } while (0)
#define alarm_log1() \
    do               \
    {                \
    } while (0)
#define alarm_log2() \
    do               \
    {                \
    } while (0)
#endif

void move_string(uint8_t *str, uint8_t str_size, int move)
{
    bool dir = move >= 0 ? true : false;
    int k = abs(move);
    k %= str_size;
    uint8_t arr[str_size];
    if (dir)
    {
        for (int i = k; i < str_size; i++)
        {
            arr[i - k] = str[i];
        }
        for (int i = 0; i < k; i++)
        {
            arr[str_size - k + i] = str[i];
        }
    }
    else
    {
        for (int i = 0; i < k; i++)
        {
            arr[i] = str[str_size - k + i];
        }
        for (int i = k; i < str_size; i++)
        {
            arr[i] = str[i - k];
        }
    }
    for (int i = 0; i < str_size; i++)
    {
        str[i] = arr[i];
    }
}

bool to_10(const uint8_t *str, uint8_t str_size, int *a, int *b)
{
    if (!str || !a || !b)
        return false;
    if (str_size != 14)
        return false;

    char a_b[9]; // 8 λ + '\0'
    char b_b[7]; // 6 λ + '\0'

    for (int i = 0; i < 8; i++)
    {
        if (str[i] != '0' && str[i] != '1')
            return false;
        a_b[i] = str[i];
    }
    a_b[8] = '\0';

    for (int i = 0; i < 6; i++)
    {
        if (str[i + 8] != '0' && str[i + 8] != '1')
            return false;
        b_b[i] = str[i + 8];
    }
    b_b[6] = '\0';

    *a = (int)strtol(a_b, NULL, 2);
    *b = (int)strtol(b_b, NULL, 2);

    return true;
}

void on_key_a_click(void)
{
#define speed_val 82
    Wait_Ack(10000000);
    Zigbee_Garage_Ctrl(GARAGE_TYPE_A, 1);
    Car_MoveForward(speed_val, 1000);

    static bool qr_done = false;

    int DH = 0;
    int DL = 0;

    Car_Turn(90);
    Car_Turn(45);
    if (!qr_done)
    {
        if (identifyQrCode_New(2))
        {
            alarm_log2();
            qr_data_map();
            uint8_t *data = (uint8_t *)qr_get_typed_data(kQrType_Formula);
            uint8_t data_size = qr_get_typed_len(kQrType_Formula);
            if (data)
            {
                uint8_t zeros[16];
                uint8_t z_idx = 0;
                bool first = false;
                uint8_t move = 0;
                for (int i = 0; i < data_size; i++)
                {
                    if (!first)
                    {
                        if (data[i] == '+')
                            move = 2;
                        else if (data[i] == '-')
                            move = -3;
                        first = true;
                    }
                    if (data[i] == '0' || data[i] == '1')
                    {
                        zeros[z_idx++] = data[i];
                    }
                }
                move_string(zeros, sizeof(zeros) - 1, move);
                to_10(zeros, sizeof(zeros), &DH, &DL);

                qr_done = true;
            }
        }
        else
        {
            alarm_fail();
        }
    }
    Car_Turn(-45);
    Car_Turn(-90);

    uint8_t le_id;
    uint8_t le_buf[1];
    uint8_t le_len = sizeof(le_buf);
    bool ok_le = Zigbee_WaitItemReliable(&le_id, le_buf, &le_len, 0);
    if (ok_le)
    {
        alarm_fail();
    }
    delay(20000);
    Car_TrackToCrossTrackingBoard(speed_val);

    uint8_t rx_id;
    uint8_t rx_buf[16];
    uint8_t rx_len = 6;
    bool ok_rx = Zigbee_WaitItemReliable(&rx_id, rx_buf, &rx_len, 0);
    if (ok_rx)
    {
        rx_buf[rx_len] = '\0';
        printf("chepai: %s\n", rx_buf);
    }
    else
    {
        printf("recv timeout\n");
    }

    Car_Turn(90);
    Zigbee_Gate_Display_LicensePlate((const char *)rx_buf);
    Car_TrackToCrossTrackingBoard(speed_val);
    Car_TrackToCrossTrackingBoard(speed_val);

    uint8_t lt_id;
    uint8_t lt_buf[64];
    uint8_t lt_len = 64;
    bool ok_lt = Zigbee_WaitItemReliable(&lt_id, lt_buf, &lt_len, 20000);
    if (ok_lt)
    {
        alarm_fail();
    }

    Car_Turn(-90);
    Car_TrackToCrossTrackingBoard(speed_val);

    Car_Turn(90);
    Car_Turn(45);
    Display3D_Data_WriteText((const char *)lt_len);
    Car_Turn(-45);
    Car_Turn(-90);

    Car_Turn(-90);
    identifyTraffic_New(TRAFFIC_A);
    Car_TrackToCrossTrackingBoard(speed_val);

    Car_Turn(-90);
    Car_MoveBackward(speed_val, 1000);
    // Car_BackIntoGarage_TrackingBoard();

    Zigbee_Garage_Ctrl(GARAGE_TYPE_A, (DH + DL) * le_buf[0] % 4 + 1);
}

void on_key_b_click(void)
{
#define speed_val 82
    Car_MoveForward(speed_val, 1000);

    int DH = 0;
    int DL = 0;

    Car_Turn(90);
    Car_Turn(45);
    
    Car_SquareByUltrasonicProbe();
    Car_Turn(-6);

    if (identifyQrCode_New(2))
    {
        alarm_log2();
        qr_data_map();
        uint8_t *data = (uint8_t *)qr_get_typed_data(kQrType_Formula);
        uint8_t data_size = qr_get_typed_len(kQrType_Formula);
        if (data)
        {
            uint8_t zeros[16];
            uint8_t z_idx = 0;
            bool first = false;
            uint8_t move = 0;
            for (int i = 0; i < data_size; i++)
            {
                if (!first)
                {
                    if (data[i] == '+')
                        move = 2;
                    else if (data[i] == '-')
                        move = -3;
                    first = true;
                }
                if (data[i] == '0' || data[i] == '1')
                {
                    zeros[z_idx++] = data[i];
                }
            }
            move_string(zeros, sizeof(zeros) - 1, move);
            to_10(zeros, sizeof(zeros), &DH, &DL);
        }
    }
    else
    {
        alarm_fail();
    }

    Car_Turn(-45);
    Car_Turn(-90);
    Car_TrackToCrossTrackingBoard(speed_val);
    Car_Turn(90);

    // Zigbee_Gate_Display_LicensePlate((const char*)1);
    Car_TrackToCrossTrackingBoard(speed_val);
    Car_TrackToCrossTrackingBoard(speed_val);

    Car_Turn(-90);
    Car_TrackToCrossTrackingBoard(speed_val);

    Car_Turn(90);
    Car_Turn(45);
    // Display3D_Data_WriteText((const char*)1);
    Car_Turn(-45);
    Car_Turn(-90);

    Car_Turn(-90);

    for (int i = 0; i < 3; i++)
    {
        Car_TrackForwardTrackingBoard(75, 480, (uint8_t)(i + 1));
        DCMotor.Car_Back(75, 480);
    }
    DCMotor.Car_Back(75, 50);
    identifyTraffic_New(TRAFFIC_A);

    Car_TrackToCrossTrackingBoard(speed_val);
    Car_Turn(-90);
    Car_BackIntoGarage_TrackingBoard(3);
}

void on_key_c_click(void)
{
    identifyQrCode_New(2);
}

void on_key_d_click(void)
{
}

void btn_callback(void *btn_ptr)
{
    alarm_log1();
    struct Button *btn = (struct Button *)btn_ptr;
    uint8_t id = btn->button_id; // ✅ 直接取值
    switch (id)
    {
    case KEY_A:
        log_main("key a pressed");
        on_key_a_click();
        break;
    case KEY_B:
        log_main("key b pressed");
        on_key_b_click();
        break;
    case KEY_C:
        log_main("key c pressed");
        on_key_c_click();
        break;
    case KEY_D:
        log_main("key d pressed");
        on_key_d_click();
        break;
    default:
        log_main("Unknown key id: %d", id);
        alarm_fail();
        break;
    }
}

void setup()
{
    // CPP库百科荣创官方维护
    CoreLED.Initialization();          // 核心板led灯初始化
    CoreBeep.Initialization();         // 核心板蜂鸣器初始化
    ExtSRAMInterface.Initialization(); // 底层通讯初始化
    LED.Initialization();              // 任务板LED初始化
    BH1750.Initialization();           // 光强
    BEEP.Initialization();             // 任务板蜂鸣器
    Infrare.Initialization();          // 红外
    DCMotor.Initialization();          // 电机
    BKRC_Voice.Initialization();       // 小创初始化
    // C库用户维护
    Ultrasonic_Init();
    Log_Init(115200); // 串口初始化
    K210_SerialInit();
    Servo_SetAngle(SERVO_DEFAULT_ANGLE);
    muti_button_init(btn_callback);
    // 是否使用陀螺仪
#ifdef USE_GYRO
    LSM6DSV16X_Init();
    LSM6DSV16X_RezeroYaw_Fresh();
    delay(2000); // 强行静止2秒
#endif
    alarm_log2();
}

void loop()
{
    static uint32_t last = 0;
    static uint8_t led = 0;
    uint32_t now = millis();
    if (now - last >= 100)
    {
        led ^= 1;
        CoreLED.TurnOnOff(led);
        last = now;
    }
    button_ticks();
    delay(5);
}
void gate_on(void)
{
    Zigbee_Gate_SetState(true);
}

void trafficb(void)
{
    identifyTraffic_New(TRAFFIC_B);
}
