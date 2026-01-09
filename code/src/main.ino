//百科荣创官方库
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
#include "Infrared_driver.hpp"
#include "Display3D.hpp"
#include "multi_button.h"
#include "button_user.h"
#include "qrcode_datamap.hpp"
#include "ultrasonic.hpp"

// 各种演示
#include "qrcode_demos.hpp"

#define LOG_MAIN_EN  1

#if LOG_MAIN_EN
  #define log_main(fmt, ...)  LOG_P("[MAIN] " fmt "\r\n", ##__VA_ARGS__)
  #define alarm_fail()  do { BEEP.ToggleNTimes(3, 120); } while(0)
  #define alarm_log1()   do { BEEP.ToggleNTimes(1, 120); } while(0)
  #define alarm_log2()   do { BEEP.ToggleNTimes(2, 120); } while(0)
#else
  #define log_main(...) do {} while(0)
  #define alarm_fail()  do {} while(0)
  #define alarm_log1()   do {} while(0)
  #define alarm_log2()   do {} while(0)
#endif

void on_key_a_click(void) {
    uint8_t my_weather = 0;
    int8_t my_temp = 0;  
    uint8_t light_gear = 0;
    ProtocolData_t data;
    uint8_t speed_val = 82;
    const char str[] = "B1B2B5B4";
    LSM6DSV16X_RezeroYaw_Fresh(200); 
    if (!Wait_Start_Cmd(1000)) {
        alarm_fail();
    }
    Zigbee_Garage_Ctrl(GARAGE_TYPE_B, 1, true);
    Car_MoveForward(speed_val, 1350); 
    if (!Zigbee_Bus_Read_Env(&my_weather, &my_temp)) {
        alarm_fail();
    }
    Car_Turn_Gryo(-90);
    light_gear = my_temp % 4 + 1;
    Infrared_Street_Light_Set(light_gear);
    Car_Turn_Gryo(90);
    Car_TrackToCross(speed_val);
    data.traffic_light_abc[TRAFFIC_A] = identifyTraffic_New(TRAFFIC_A);
    if (!SendData(&data)) {
        alarm_fail();
    }
    Car_Turn_Gryo(180);
    Car_MoveForward(60,300);
    Car_MoveBackward(60, 300);
    if (!identifyQrCode_New(3)) {
        alarm_fail();
    }  
    if (str) {
        size_t n = strlen(str);
        size_t cap = 4 * 3;    
        if (n > cap) n = cap;
        for (size_t i = 0; i < n; i++) {
            data.random_route[i] = (uint8_t)str[i];
        }
    }
    if (!SendData(&data)) {
        alarm_fail();
    }
    Car_Turn_Gryo(90);
    Car_TrackToCross(speed_val);
    if (!Wait_Start_Cmd(1000)) {
        alarm_fail();
    }
    Car_Turn_Gryo(0); 
    identifyTraffic_New(TRAFFIC_B);
    Car_TrackToCross(speed_val);
    Zigbee_Gate_Display_LicensePlate("A12345");	
    Car_TrackToCross(speed_val);
    Car_Turn_Gryo(90);
    Car_MoveForward(60,300);		
    if (!identifyQrCode_New(2)) {
        alarm_fail();
    }
    Car_MoveBackward(60, 300);
    Car_Turn_Gryo(-90); 
    Car_TrackToCross(speed_val);
    Car_MoveForward_Gyro(85, 1950, -90);
    Car_Turn_Gryo(-135);
    Car_BackIntoGarage_Gyro(90, 1200, -180); //倒车入库
    if (!Wait_Start_Cmd(1000)) {
        alarm_fail();
    }
    Zigbee_Garage_Ctrl(GARAGE_TYPE_A, 3, false);
}



void on_key_b_click(void) {
    uint8_t speed_val = 88; // 局部速度变量
    int sl_gear = 0;
    int n = 1;
    uint8_t fht_key[6];
    // 路径段: D1 -> D2
    LSM6DSV16X_RezeroYaw_Fresh(200); // 陀螺仪归零
    Car_MoveForward(speed_val, 1200); // 发车段特殊指令
    Car_Turn_Gryo(-90); // 左转 90°
    identifyTraffic_New(TRAFFIC_A);
    // 路径段: D2 -> B2
    Car_TrackToCross(speed_val);
    Car_MoveToTarget(18);
    if (!identifyQrCode_New(2)) {
        alarm_fail();
    } else {
        alarm_log2();
        qr_data_map();
        uint8_t *qr_data = (uint8_t *)qr_get_typed_data(kQrType_Bracket_LT);
        uint8_t qr_data_len  = qr_get_typed_len(kQrType_Bracket_LT);
        if (qr_data && qr_data_len) {
            sort_u8_asc(qr_data, qr_data_len);
            for (int i = 0; i < 6; i++) {
                fht_key[i] = qr_data[i];
            }
        }
    }
    Car_MoveBackward(70, 350);
    Car_Turn_Gryo(0); // 左转 90°
    Zigbee_Gate_SetState(true);
    // 路径段: B2 -> B4
    Car_TrackToCross(speed_val);
     Zigbee_Gate_SetState(false);
    Car_Turn_Gryo(-90); // 左转 90°
    sl_gear = Infrared_Street_Light_GetGear();
    Infrared_Street_Light_Set(n);
    Car_Turn_Gryo(90); // 左转 90°
    identifyTraffic_New(TRAFFIC_B);
    // 路径段: B4 -> D4
    Car_TrackToCross(speed_val);
    Zigbee_Bus_SpeakRandom();
    uint8_t data = BKRC_Voice.Voice_WaitFor() - 5;
    Zigbee_TFT_Display_Hex(TFT_ID_B, 0, 0, data);
    // 路径段: D4 -> F4
    Car_TrackToCross(speed_val);
    Car_Turn_Gryo(180); 
    // 路径段: F4 -> F2
    Car_TrackToCross(speed_val);
    // 路径段: F2 -> F1
    Car_BackIntoGarage_Gyro(30, 1400, 0); //倒车入库
}

void on_key_c_click(void) {
    ProtocolData_t data;
    if (!identifyQrCode_New(3)) {
        alarm_fail();
        return;
    }
    qr_data_map();
    uint8_t *qr_data1 = (uint8_t *)qr_get_typed_data(kQrType_Bracket_LT); 
    uint8_t qr_data1_len = qr_get_typed_len(kQrType_Bracket_LT);
    if (qr_data1 && qr_data1_len) {
        log_main("%s", (const char *)qr_data1);
        qr_data1 = (uint8_t *)algo_extract_bracket((const char*)qr_data1, '<');
        uint8_t route_len = qr_data1_len - 6;
        uint8_t route_str[12];
        uint8_t route = 0;
        for (int i = 0; i < route_len; i++) {
            route_str[i] = qr_data1[2 + i];
        }
        route_str[route_len] = '\0';
        if (strstr((const char *)route_str, "B4") != NULL) {
            route = 1;
        } else if (strstr((const char *)route_str, "D2") != NULL) {
            route = 2;
        }
        log_main("%s, %d", (const char *)route_str, route);
        data.random_route[0] = route;
        if (!SendData(&data)) {
            alarm_fail();
        }
        alarm_log2();
    }
}

void on_key_d_click(void) {
    identifyQrCode_New(2);                          
}

void btn_callback(void *btn_ptr) {
    alarm_log1();
    struct Button* btn = (struct Button*)btn_ptr;
    uint8_t id = btn->button_id;   // ✅ 直接取值
    switch (id)
    {
        case KEY_A: log_main("key a pressed");  on_key_a_click(); break;
        case KEY_B: log_main("key b pressed");  on_key_b_click(); break;
        case KEY_C: log_main("key c pressed");  on_key_c_click(); break;
        case KEY_D: log_main("key d pressed");  on_key_d_click(); break;
        default:
            log_main("Unknown key id: %d", id);
            alarm_fail();
            break;
    }
}

void setup()
{
	// CPP库百科荣创官方维护
	CoreLED.Initialization();           //核心板led灯初始化
	CoreBeep.Initialization();		    //核心板蜂鸣器初始化
	ExtSRAMInterface.Initialization();  //底层通讯初始化
	LED.Initialization();               // 任务板LED初始化
	BH1750.Initialization();            // 光强
	BEEP.Initialization();              // 任务板蜂鸣器
	Infrare.Initialization();           // 红外
	DCMotor.Initialization();           //电机
	BKRC_Voice.Initialization();        //小创初始化
	// C库用户维护
    Ultrasonic_Init();
	Log_Init(115200);//串口初始化
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
    if (now - last >= 100) {
        led ^= 1;
        CoreLED.TurnOnOff(led);
        last = now;
    }
    button_ticks();
    delay(5);
}          
