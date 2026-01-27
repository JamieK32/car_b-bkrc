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
#include "FHT.hpp"
#include "Display3D.hpp"
#include "multi_button.h"
#include "button_user.h"
#include "qrcode_datamap.hpp"
#include "ultrasonic.hpp"
#include "vehicle_exchange_v2.h"
#include "path_executor.h"

// 各种演示
#include "qrcode_demos.hpp"

#define LOG_MAIN_EN  1

#if LOG_MAIN_EN
  #define log_main(fmt, ...)  LOG_P("[MAIN] " fmt "\r\n", ##__VA_ARGS__)
  #define log_printf(fmt, ...)  LOG_P(fmt, ##__VA_ARGS__)
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
  
}

void gate_on(void) {
    Zigbee_Gate_SetState(true);
}

void trafficb(void) {
    identifyTraffic_New(TRAFFIC_B);
}

void on_key_b_click(void) {
    plan_cfg_t cfg = { .default_speed = 75 };

    path_avoid_seg_t avoids[2] = { {"D4", "D2"}, {"D4", "D6"}};
    path_mark_seg_t marksp[2] = { {"D2", "F2", gate_on, PATH_MARK_CALL_TRACK_ONLY},
                                  {"F2", "F4", trafficb, PATH_MARK_CALL_TURN_ONLY}};

    action_t acts[MAX_ACTIONS];
    char path_buf[128];

    int n = Path_PlanAndBuildActions("B2", "F6",
                                    avoids, 2,
                                    marksp, 2,
                                    &cfg,
                                    acts, MAX_ACTIONS,
                                    path_buf, sizeof(path_buf));
                                
    log_main("path_buf = '%s'", path_buf);
    
    Path_ExecuteActions(acts, n);

    Car_BackIntoGarage_TrackingBoard(3);
}

void on_key_c_click(void) {
    identifyQrCode_New(2);
}

void on_key_d_click(void) {
  
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
