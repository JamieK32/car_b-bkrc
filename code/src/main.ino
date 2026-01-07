//百科荣创官方库
#include "DCMotor.h"
#include "CoreLED.h"
#include "CoreBeep.h"
#include "ExtSRAMInterface.h"
#include "LED.h"
#include "BH1750.h"
#include "Command.h"
#include "BEEP.h"
#include "Ultrasonic.h"
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

// 各种演示
#include "qrcode_demos.hpp"

#define LOG_MAIN_EN  1

#if LOG_MAIN_EN
  #define log_main(fmt, ...)  LOG_P("[MAIN] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define log_main(...)  do {} while(0)
#endif

#define alarm_fail()  do { BEEP.ToggleNTimes(3, 120); } while(0)
#define alarm_log()   do { BEEP.ToggleNTimes(1, 120); } while(0)


void on_key_a_click(void) {
    log_main("key a pressed");
    uint8_t my_weather = 0;
    int8_t my_temp = 0;  
    uint8_t Light = 0;
    #define speed_val 82
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
    Light = my_temp % 4 + 1;
    Infrared_Street_Light_Set(Light);
    Car_Turn_Gryo(90);
    Car_TrackToCross(speed_val);
    identifyTraffic_New(TRAFFIC_A);
    Car_Turn_Gryo(180);
    Car_MoveForward(60,300);
    Car_MoveBackward(60, 300);
    if (!identifyQrCode_New(3)) {
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
    log_main("key b pressed");
    ZigbeeBusInfo info;
    Zigbee_Bus_Read_All(&info, 1000);
    PrintBusInfo(&info);
    BKRC_Voice.Voice_SpeakWeather(info.weather);
    BKRC_Voice.Voice_SpeakTemperature(info.temp);
}

void on_key_c_click(void) {
    log_main("key c pressed");

}

void on_key_d_click(void) {
    log_main("key d pressed");
}



void btn_callback(void *btn_ptr) {
    alarm_log();
    struct Button* btn = (struct Button*)btn_ptr;
    uint8_t id = btn->button_id;   // ✅ 直接取值
    switch (id)
    {
        case KEY_A: on_key_a_click(); break;
        case KEY_B: on_key_b_click(); break;
        case KEY_C: on_key_c_click(); break;
        case KEY_D: on_key_d_click(); break;
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
	Ultrasonic.Initialization();        //超声波
	DCMotor.Initialization();           //电机
	BKRC_Voice.Initialization();        //小创初始化
	// C库用户维护
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
    alarm_log();
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