//百科荣创官方库
#include "DCMotor.h"
#include "CoreLED.h"
#include "CoreKEY.h"
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

// 各种演示
#include "qrcode_demos.hpp"
#include "ve_demo.h"

void KEY_Handler(uint8_t k_value);

void setup()
{
	// CPP库百科荣创官方维护
	CoreLED.Initialization();           //核心板led灯初始化
	CoreKEY.Initialization();           //核心板按键初始化
	CoreBeep.Initialization();		    //核心板蜂鸣器初始化
	ExtSRAMInterface.Initialization();  //底层通讯初始化
	LED.Initialization();               // 任务板LED初始化
	BH1750.Initialization();            // 光强
	BEEP.Initialization();              //  任务板蜂鸣器
	Infrare.Initialization();           // 红外
	Ultrasonic.Initialization();        //超声波
	DCMotor.Initialization();           //电机
	BKRC_Voice.Initialization();        //小创初始化
	// C库用户维护
	Log_Init(115200);//串口初始化
	K210_SerialInit();
	Servo_SetAngle(SERVO_DEFAULT_ANGLE);
	// 是否使用陀螺仪
#ifdef USE_GYRO
	LSM6DSV16X_Init();
	LSM6DSV16X_RezeroYaw_Fresh();
	delay(2000); // 强行静止2秒
#endif 
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

	CoreKEY.Kwhile(KEY_Handler);                                  //按键检测
}

/******************************************** 按键检测函数 ********************************************************/
void KEY_Handler(uint8_t k_value)
{
	switch (k_value)
	{
	case 1: 
		uint8_t data = Zigbee_Garage_Get(GARAGE_TYPE_A);
		LOG_P("data = %d\n", data);
		break;
	case 2:	


		break;
	case 3:

		break;
	case 4:	

		break;
	case 5:

		break;
	}

}