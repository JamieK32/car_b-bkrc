#include "DCMotor.h"
#include <Command.h>
#include "wiring_private.h"
#include <ExtSRAMInterface.h>
#include <Metro.h>
#include "CoreBeep.h"
#include "My_Lib.h"

_DCMotor DCMotor;
Metro DCMotorMetro(20);

uint8_t H8_light = 0;
uint8_t Q7_light = 0;
uint8_t Find_Terrain_Flag = 0;
uint16_t LSpeed, RSpeed;
uint8_t gd = 0;
uint8_t ogd = 0;
uint8_t Go_Flag = 0;
uint8_t Find_Cross_Rfid = 0;
unsigned long go_time = 0;
uint16_t car_godis;
uint16_t rw_i = 0;
_DCMotor::_DCMotor()
{
	ExtSRAMInterface.Initialization();
}

_DCMotor::~_DCMotor()
{
}
int16_t CanHost_Mp;		//当前mp
int16_t Roadway_cmp;	//获取mp
						/*
						码盘同步
						**/
void _DCMotor::Roadway_mp_syn(void)
{
	Roadway_cmp = ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8);
}
uint8_t Stop_Car_lukou()		//路口遇到卡
{
	uint8_t Rturn = 0;
	u8 _gd = ExtSRAMInterface.ExMem_Read(TRACK_ADDR);
	if (_gd == 0xF8 || _gd == 0xFE || _gd == 0xFC)                  //	   1111 1110  -  1111 1100  -	1111 1000
	{
		Rturn = 1;
	}
	else if (_gd == 0x1F || _gd == 0x7F || _gd == 0x3F)		        //      0111 1111  -  0011 1111  -	0001 1111
	{
		Rturn = 1;
	}
	else if (_gd == 0xE0 || _gd == 0x07								//		1110 0000  -  0000 0111
		|| _gd == 0xF0 || _gd == 0x0F)								//		1111 0000  -  0000 1111
	{
		Rturn = 1;
	}
	else if (_gd == 0x7E || _gd == 0x3C)  							//		0111 1110  -  0011 1100
	{
		Rturn = 1;
	}
	else if (_gd == 0x7C)											//		0111 1100  -  0111 1000
	{
		Rturn = 1;
	}
	else if (_gd == 0x3E || _gd == 0x1E) 							//		0011 1110  -  0001 1110
	{
		Rturn = 1;
	}
	else
		Rturn = 0;

	return Rturn;
}
/*
码盘获取
**/
uint16_t _DCMotor::Roadway_mp_Get(void)
{
	uint32_t ct;
	CanHost_Mp = ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8);
	if (CanHost_Mp > Roadway_cmp)//求出当前码盘与之前同步时的码盘差值
		ct = CanHost_Mp - Roadway_cmp;
	else
		ct = Roadway_cmp - CanHost_Mp;
	if (ct > 0x8000)//如果差值大于16位?
		ct = 0xffff - ct;
	return ct;
}

///************************************************************************************************************
//【函 数 名】：	Initialization	直流电机初始化
//【参数说明】：	fHz		：		初始化PWM输出频率，单位：Hz
//【返 回 值】：	无
//【简    例】：	Initialization(8000);
//************************************************************************************************************/
//void _DCMotor::Initialization(uint32_t fHz)
//{
//	pinMode(L_CONTROL_PIN, OUTPUT);
//	pinMode(R_CONTROL_PIN, OUTPUT);
//
//	pinMode(R_F_M_PIN, OUTPUT);
//	pinMode(R_B_M_PIN, OUTPUT);
//	pinMode(L_F_M_PIN, OUTPUT);
//	pinMode(L_B_M_PIN, OUTPUT);
//
//	TCCR4A = 0x00;
//	TCCR4B = 0x00;
//	TCCR3A = 0x00;
//	TCCR3B = 0x00;
//
//	//选择工作模式,模式14--fast PWM
//	TCCR4A |= _BV(WGM41);
//	TCCR4B |= _BV(WGM42) | _BV(WGM43);
//	TCCR3A |= _BV(WGM31);
//	TCCR3B |= _BV(WGM32) | _BV(WGM33);
//
//	//设置PWM波的频率
//	MotorFrequency(fHz);
//
//	//设置PWM波的占空比
//	//SpeedCtr(0, 0);
//	Stop();
//
//	//设置输出通道
//	//TCCR4A |= _BV(COM4C1) | _BV(COM4B1) | _BV(COM4C0) | _BV(COM4B0);
//	//TCCR3A |= _BV(COM3C1) | _BV(COM3B1) | _BV(COM3C0) | _BV(COM3B0);
//	TCCR4A |= _BV(COM4C1) | _BV(COM4B1);
//	TCCR3A |= _BV(COM3C1) | _BV(COM3B1);
//
//	ParameterInit();
//}

/************************************************************************************************************
【函 数 名】：	Initialization	直流电机初始化
【参数说明】：	fHz	：		初始化PWM输出频率，单位：Hz
【返 回 值】：	无
【简    例】：	Initialization(30000);
************************************************************************************************************/
void _DCMotor::Initialization(uint32_t fHz)
{
	/*ExtSRAMInterface.Initialization();*/
	pinMode(L_CONTROL_PIN, OUTPUT);
	pinMode(R_CONTROL_PIN, OUTPUT);

	pinMode(R_F_M_PIN, OUTPUT);
	pinMode(R_B_M_PIN, OUTPUT);
	pinMode(L_F_M_PIN, OUTPUT);
	pinMode(L_B_M_PIN, OUTPUT);
	digitalWrite(L_CONTROL_PIN, HIGH);
	digitalWrite(R_CONTROL_PIN, HIGH);

	//选择工作模式,模式15--fast PWM 该模式下即重装载值为OCRnA（输出比较寄存器）
	TCCR4A |= _BV(WGM41) | _BV(WGM40);
	TCCR4B |= _BV(WGM42) | _BV(WGM43);

	TCCR3A |= _BV(WGM31) | _BV(WGM30);
	TCCR3B |= _BV(WGM32) | _BV(WGM33);

	//设置输出通道
	// B C 通道计数器值小于比较寄存器时引脚输出1，大于OCR比较寄存器值时输出0
	TCCR4A |= _BV(COM4C1) | _BV(COM4B1);
	TCCR3A |= _BV(COM3C1) | _BV(COM3B1);

	//设置PWM波的频率
	MotorFrequency(fHz);
	ParameterInit();
}

//void _DCMotor::ParameterInit(void)
//{
//	for (uint8_t i = 0; i < 101; i++)	//占空比初始化
//	{
//		speed[i] = fHz * i / 100;
//	}
//}

/************************************************************************************************************
【函 数 名】：  ParameterInit	分配速度对应的比较数值
【参数说明】：	_fHz：
【返 回 值】：	无
【简    例】：	ParameterInit();
************************************************************************************************************/
//void _DCMotor::ParameterInit(void)
//{
//	for (uint8_t i = 1; i < 101; i++)	//占空比初始化
//	{
//		speed[i] = (fHz * 0.6) + (fHz * 0.4) * i / 100;  // *0.6 因为电机在60%以上时扭矩才足以驱动小车
//	}
//	speed[0] = 0;
//}
void _DCMotor::ParameterInit(void)
{
 for (uint8_t i = 0; i < 101; i++) //占空比初始化
 {
  speed[i] = fHz * i / 100;
 }
}
void _DCMotor::Tracking_light_Count(uint8_t gd, uint8_t ogd)
{
	H8_light = 0;
	Q7_light = 0;
	for (int i = 0x01; i < 0x100; i <<= 1)
	{
		if (i & gd)
		{
			H8_light++;
		}
		if (i & ogd)
		{
			Q7_light++;
		}
	}
}

void _DCMotor::SpeedSetOne(int16_t s, uint8_t* c1, uint8_t* c2)
{
	uint8_t t;
	t = (s >= 0) ? s : s * (-1);
	if (t > 100)
		t = 100;
	if (t < 5)
		t = 5;
	if (s == 0)
	{
		*c1 = speed[100];		//100;
		*c2 = speed[100];		//100;
	}
	else if (s > 0)
	{
		*c1 = speed[t];
		*c2 = speed[0];
	}
	else
	{
		*c1 = speed[0];
		*c2 = speed[t];
	}
}

void _DCMotor::SpeedCtr(int16_t L_speed, int16_t R_speed)
{
	uint8_t ocr3b, ocr3c, ocr4b, ocr4c;

	SpeedSetOne(L_speed, &ocr4c, &ocr4b);
	SpeedSetOne(R_speed, &ocr3b, &ocr3c);

	(ocr3b == 0) ? (TCCR3A |= _BV(COM3B0), ocr3b = fHz) : (TCCR3A &= ~_BV(COM3B0));
	(ocr3c == 0) ? (TCCR3A |= _BV(COM3C0), ocr3c = fHz) : (TCCR3A &= ~_BV(COM3C0));
	(ocr4b == 0) ? (TCCR4A |= _BV(COM4B0), ocr4b = fHz) : (TCCR4A &= ~_BV(COM4B0));
	(ocr4c == 0) ? (TCCR4A |= _BV(COM4C0), ocr4c = fHz) : (TCCR4A &= ~_BV(COM4C0));

	OCR4C = ocr4c;
	OCR4B = ocr4b;
	OCR3C = ocr3c;
	OCR3B = ocr3b;
}
void _DCMotor::SpeedCtr_2(int16_t L_speed, int16_t R_speed)
{
	uint8_t ocr3b, ocr3c, ocr4b, ocr4c;

	SpeedSetOne(10, &ocr4c, &ocr4b);
	SpeedSetOne(10, &ocr3b, &ocr3c);

	ocr4c = 36;
	ocr3b = 36;
	(ocr3b == 0) ? (TCCR3A |= _BV(COM3B0), ocr3b = fHz) : (TCCR3A &= ~_BV(COM3B0));
	(ocr3c == 0) ? (TCCR3A |= _BV(COM3C0), ocr3c = fHz) : (TCCR3A &= ~_BV(COM3C0));
	(ocr4b == 0) ? (TCCR4A |= _BV(COM4B0), ocr4b = fHz) : (TCCR4A &= ~_BV(COM4B0));
	(ocr4c == 0) ? (TCCR4A |= _BV(COM4C0), ocr4c = fHz) : (TCCR4A &= ~_BV(COM4C0));

	OCR4C = ocr4c;
	OCR4B = ocr4b;
	OCR3C = ocr3c;
	OCR3B = ocr3b;
}
void _DCMotor::SpeedCtr_3(int16_t L_speed, int16_t R_speed)
{
	uint8_t ocr3b, ocr3c, ocr4b, ocr4c;

	SpeedSetOne(-10, &ocr4c, &ocr4b);
	SpeedSetOne(-10, &ocr3b, &ocr3c);

	ocr4c = 36;
	ocr3b = 36;
	(ocr3b == 0) ? (TCCR3A |= _BV(COM3B0), ocr3b = fHz) : (TCCR3A &= ~_BV(COM3B0));
	(ocr3c == 0) ? (TCCR3A |= _BV(COM3C0), ocr3c = fHz) : (TCCR3A &= ~_BV(COM3C0));
	(ocr4b == 0) ? (TCCR4A |= _BV(COM4B0), ocr4b = fHz) : (TCCR4A &= ~_BV(COM4B0));
	(ocr4c == 0) ? (TCCR4A |= _BV(COM4C0), ocr4c = fHz) : (TCCR4A &= ~_BV(COM4C0));

	OCR4C = ocr4c;
	OCR4B = ocr4b;
	OCR3C = ocr3c;
	OCR3B = ocr3b;
}

bool _DCMotor::ClearCodeDisc(void)
{
	uint16_t distance;
	unsigned long t;
	Command.Judgment(Command.command01);
	for (size_t i = 0; i < 8; i++)
	{
		ExtSRAMInterface.ExMem_JudgeWrite(WRITEADDRESS + i, Command.command01[i]);
	}
	DCMotorMetro.interval(20);
	for (size_t i = 0; i < 5; i++)
	{
		if (DCMotorMetro.check() == 1)
		{
			distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
			if (distance == 0x0000)
			{
				return false;
			}
		}
	}
	return true;
}
/************************************************************************************************************
小车动作的相关函数
************************************************************************************************************/
/************************************************************************************************************
【函 数 名】：  Go		小车前进函数
【参数说明】：	speed	：设置速度
distance: 设置前进的距离
【返 回 值】：	无
【简    例】：	Go(70);	小车动作：前进，前进速度：70
************************************************************************************************************/
void _DCMotor::Go(uint8_t speed)
{
	SpeedCtr(speed, speed);
	StartUp();
}
uint16_t _DCMotor::Go(uint8_t speed, uint16_t _distance)
{
	unsigned long t;
	uint16_t distance;
	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed, speed);
	StartUp();

	t = millis();
	do
	{
		delay(1);
		distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((65516 > distance) && (distance > 20))
		{
			if ((distance >= _distance) || ((millis() - t) >= 30000))
			{
				Stop();
				delay(100);
				break;
			}
		}
	} while (true);
	return distance;
}

/************************************************************************************************************
【函 数 名】：  Back		小车后退函数
【参数说明】：	speed	:	设置速度
distance:	设置后退的距离
【返 回 值】：	无
【简    例】：	Back(70);	小车动作：后退，后退速度：70
************************************************************************************************************/
void _DCMotor::Back(uint8_t speed)
{
	SpeedCtr(speed * (-1), speed * (-1));
	StartUp();
}
uint16_t _DCMotor::Back(uint8_t speed, uint16_t _distance)
{
	unsigned long t;
	uint16_t distance;

	delay(100);
	while (ClearCodeDisc())
	{
	}
	Back(speed);
	t = millis();
	do
	{
		delay(1);
		distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((65536 > distance) && (distance > 20))
		{
			if (((65536 - distance) >= _distance) || ((millis() - t) > 30000))
			{
				Stop();
				delay(100);
				break;
			}
		}
	} while (true);
	return (65536 - distance);
}

/************************************************************************************************************
【函 数 名】：  TurnLeft	小车左转函数,Lspeed <= Rspeed
【参数说明】：	Lspeed	：	设置左轮速度
				Rspeed	：	设置右轮速度
【返 回 值】：	无
【简    例】：	TurnLeft(70);	小车动作：左转，左转速度：70
************************************************************************************************************/
void _DCMotor::TurnLeft(int8_t Lspeed, int8_t Rspeed)
{
	SpeedCtr(Lspeed * (-1), Rspeed);
	StartUp();
}
void _DCMotor::TurnLeft(int8_t speed)
{
	uint8_t tgd, tp;
	unsigned long t;
	uint8_t  trackval;
	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed * (-1), speed);
	StartUp();
	do
	{
		tgd = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		tp = SearchBit(1, tgd);
		if (tp <= 0x04)
		{
			break;
		}
	} while (true);

	t = millis();
	do
	{
		trackval = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		if ((!(trackval & 0x10)) || ((millis() - t) > 10000))
		{
			Stop();
			delay(100);
			break;
		}
	} while (true);
}

/************************************************************************************************************
【函 数 名】：  TurnRight	小车右转函数,Rspeed <= Lspeed
【参数说明】：	Lspeed	：	设置左轮速度
				Rspeed	：	设置右轮速度
【返 回 值】：	无
【简    例】：	TurnRight(70);	小车动作：右转，右转速度：70
************************************************************************************************************/
void _DCMotor::TurnRight(int8_t Lspeed, int8_t Rspeed)
{
	SpeedCtr(Lspeed, Rspeed * (-1));
	StartUp();
}
void _DCMotor::TurnRight(int8_t speed)
{
	uint8_t tgd, tp;
	unsigned long t;
	uint8_t  trackval;
	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed, speed * (-1));
	StartUp();
	do
	{
		tgd = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		tp = SearchBit(0, tgd);
		if (tp >= 0x20)
		{
			break;
		}
	} while (true);

	t = millis();
	do
	{
		trackval = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		if ((!(trackval & 0x08)) || ((millis() - t) > 10000))
		{
			Stop();
			delay(100);
			break;
		}
	} while (true);
}
/*
左转任意角度
**/
uint16_t _DCMotor::TurnLeftAngle(uint8_t speed, uint16_t _distance)
{
	unsigned long t;
	uint16_t distance;
	while (ClearCodeDisc())
	{
	}
	TurnLeft(speed, speed);
	t = millis();
	do
	{
		distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));

		if ((65516 > distance) && (distance > 20))
		{
			if (((65536 - distance) >= _distance) || ((millis() - t) > 30000))
			{
				Stop();
				delay(50);
				distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
				break;
			}
		}
		delay(1);
	} while (true);
	return (65536 - distance);
}

uint16_t _DCMotor::TurnLeftAngle_1(uint8_t* Alarm_buf, uint8_t speed, uint16_t _distance)//左
{
	unsigned long t;
	uint16_t distance;
	while (ClearCodeDisc())
	{
	}
	TurnLeft(speed, speed);
	t = millis();
	do
	{
		Infrared_Send_degree(Alarm_buf, 2, 200);
		distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));

		if ((65516 > distance) && (distance > 20))
		{
			if (((65536 - distance) >= _distance) || ((millis() - t) > 30000))
			{
				Stop();
				delay(50);
				distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
				break;
			}
		}
		delay(1);
	} while (true);
	return (65536 - distance);
}

/*
右转任意角度
**/
uint16_t _DCMotor::TurnRightAngle(uint8_t speed, uint16_t _distance)
{
	unsigned long t;
	uint16_t distance;
	while (ClearCodeDisc())
	{
	}
	TurnRight(speed, speed);

	t = millis();
	do
	{
		distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((65516 > distance) && (distance > 20))
		{
			if ((distance >= _distance) || ((millis() - t) >= 30000))
			{
				Stop();
				delay(50);
				distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
				break;
			}
		}
		delay(1);
	} while (true);
	return distance;
}
uint16_t _DCMotor::TurnRightAngle_1(uint8_t* Alarm_buf, uint8_t speed, uint16_t _distance)//右
{
	unsigned long t;
	uint16_t distance;
	while (ClearCodeDisc())
	{
	}
	TurnRight(speed, speed);

	t = millis();
	do
	{
		Infrared_Send_degree(Alarm_buf, 2, 200);
		distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((65516 > distance) && (distance > 20))
		{
			if ((distance >= _distance) || ((millis() - t) >= 30000))
			{
				Stop();
				delay(50);
				distance = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
				break;
			}
		}
		delay(1);
	} while (true);
	return distance;
}
/************************************************************************************************************
【函 数 名】：  Stop	小车停止函数
【参数说明】：	无
【返 回 值】：	无
【简    例】：	Stop();	小车动作：停止
************************************************************************************************************/
void _DCMotor::Stop(void)
{
	/*这两段代码都有电机停止功能，但停止的效果不一样*/

	/**********电机不锁死******************/
	//PORTE &= ~_BV(PE3);
	//PORTH &= ~_BV(PH3);
	//SpeedCtr(0, 0);
	/***********END**************************/

	//电机瞬间锁死
	SpeedCtr(0, 0);
	PORTE |= _BV(PE3);
	PORTH |= _BV(PH3);
	/**********END************/
}

/************************************************************************************************************
【函 数 名】：  StartUp		小车启动函数
【参数说明】：	无
【返 回 值】：	无
【简    例】：	StartUp();	小车动作：启动
************************************************************************************************************/
void _DCMotor::StartUp(void)
{
	PORTE |= _BV(PE3);
	PORTH |= _BV(PH3);
}

/************************************************************************************************************
【函 数 名】：  ShutDown	小车关闭函数
【参数说明】：	无
【返 回 值】：	无
【简    例】：	ShutDown();	小车动作：关闭
************************************************************************************************************/
void _DCMotor::ShutDown(void)
{
	SpeedCtr(100, 100);
	PORTE |= _BV(PE3);
	PORTH |= _BV(PH3);
}
/*************************************************************END***********************************************/

void _DCMotor::CarTrack(uint8_t Car_Spend)
{
	unsigned long t;
	uint8_t mode, _mode = 10;
	uint8_t tp;
	uint8_t gd, ogd, tgd;
	uint8_t LSpeed, RSpeed;
	uint16_t count = 0;
	uint8_t  firstbit[8];
	uint16_t distance;
	ogd = 0xff;
	StartUp();
	while (true)
	{
		tp = 0;
		firstbit[0] = 0;
		gd = ExtSRAMInterface.ExMem_Read(TRACK_ADDR);//获取后八位的循迹灯状态
		for (size_t i = 0x01; i < 0x100; i <<= 1)
		{
			if ((gd & i) == 0)
			{
				firstbit[tp++] = uint8_t(i);
			}
		}

		if (tp >= 0x05)			/*循迹灯灭的个数≥5表示全灭*/
		{
			Stop();
			delay(100);
			break;
		}
		else
		{
			switch (firstbit[0])
			{
			case 0x00:
				SpeedCtr(Car_Spend, Car_Spend);
				break;
			case 0x01:
				SpeedCtr(Car_Spend + 60, Car_Spend - 120);
				break;
			case 0x02:
				SpeedCtr(Car_Spend + 40, Car_Spend - 70);
				break;
			case 0x04:
				SpeedCtr(Car_Spend + 30, Car_Spend - 30);
				break;
			case 0x08:
				SpeedCtr(Car_Spend, Car_Spend);
				break;
			case 0x10:
				SpeedCtr(Car_Spend - 30, Car_Spend + 30);
				break;
			case 0x20:
				SpeedCtr(Car_Spend - 70, Car_Spend + 40);
				break;
			case 0x40:
				SpeedCtr(Car_Spend - 120, Car_Spend + 60);
				break;
			case 0x80:
				SpeedCtr(Car_Spend - 120, Car_Spend + 60);
				break;
			}
		}
	}
}

/********************************************* 自定义函数 ****************************************************/
// 前进
void _DCMotor::Car_Go(uint8_t speed, uint16_t distance)
{
	uint16_t cardis;
	unsigned long t;

	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed, speed);
	StartUp();
	t = millis();
	do
	{
		delay(1);
		cardis = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((32768 > cardis) && (cardis > 0))
		{
			if ((cardis >= distance) || ((millis() - t) >= 30000))
			{
				Stop();
				delay(100);
				break;
			}
		}
	} while (true);
}

void _DCMotor::Car_GoLR(uint8_t Lspeed, uint8_t Rspeed, uint16_t distance)
{
	uint16_t cardis;
	unsigned long t;

	while (ClearCodeDisc())
	{
	}
	SpeedCtr(Lspeed, Rspeed);
	StartUp();
	t = millis();
	do
	{
		delay(1);
		cardis = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((32768 > cardis) && (cardis > 0))
		{
			if ((cardis >= distance) || ((millis() - t) >= 30000))
			{
				Stop();
				delay(100);
				break;
			}
		}
	} while (true);
}

void _DCMotor::car_go(uint8_t speed, uint16_t distance)
{
	delay(1);
	car_godis = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
	if ((32768 > car_godis) && (car_godis > 0))
	{
		if ((car_godis >= distance) || ((millis() - go_time) >= 30000))
		{
			Stop();
			Go_Flag = 0;
			delay(50);
			//break;
		}
	}
}

// 后退
void _DCMotor::Car_Back(uint8_t speed, uint16_t distance)
{
	uint16_t cardis;
	unsigned long t;

	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed * (-1), speed * (-1));
	StartUp();
	t = millis();
	do
	{
		delay(1);
		cardis = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((65536 > cardis) && (cardis > 32768))
		{
			if (((65536 - cardis) >= distance) || ((millis() - t) > 30000))
			{
				Stop();
				delay(100);
				break;
			}
		}
	} while (true);
}

void _DCMotor::Car_BackBlackLine(uint8_t speed, uint16_t distance)
{
	uint16_t cardis;
	unsigned long t;

	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed * (-1), speed * (-1));
	StartUp();
	t = millis();
	do
	{
		delay(1);
		Tracking_light_Count(ExtSRAMInterface.ExMem_Read(TRACK_ADDR), ExtSRAMInterface.ExMem_Read(TRACK_ogd));
		cardis = uint16_t(ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET) + (ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1) << 8));
		if ((65536 > cardis) && (cardis > 32768))
		{
			if (((65536 - cardis) >= distance) || ((millis() - t) > 30000))
			{
				Stop();
				delay(100);
				break;
			}
			else if (H8_light <= 3 || Stop_Car_lukou())
			{
				Stop();
				delay(100);
				break;
			}
		}
	} while (true);
}

// 左转
void _DCMotor::Car_Left(uint8_t speed)
{
	uint8_t tgd, tp;
	uint8_t  trackval;
	unsigned long t;

	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed * (-1), speed);
	StartUp();
	do
	{
		tgd = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		tp = SearchBit(1, tgd);
		if (tp <= 0x04)
		{
			break;
		}
	} while (true);

	t = millis();
	do
	{
		trackval = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		if ((!(trackval & 0x10)) || ((millis() - t) > 10000))
		{
			Stop();
			delay(100);
			break;
		}
	} while (true);
}

// 右转
void _DCMotor::Car_Right(uint8_t speed)
{
	uint8_t tgd, tp;
	uint8_t  trackval;
	unsigned long t;

	while (ClearCodeDisc())
	{
	}
	SpeedCtr(speed, speed * (-1));
	StartUp();
	do
	{
		tgd = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		tp = SearchBit(0, tgd);
		if (tp >= 0x20)
		{
			break;
		}
	} while (true);

	t = millis();
	do
	{
		trackval = ExtSRAMInterface.ExMem_Read(BASEADDRESS + TRACKOFFSET);
		if ((!(trackval & 0x08)) || ((millis() - t) > 10000))
		{
			Stop();
			delay(100);
			break;
		}
	} while (true);
}

// 左旋转
void _DCMotor::Car_LeftTime(uint8_t speed, uint16_t time)
{
	SpeedCtr(speed * (-1), speed);
	delay(time);
	Stop();
}

// 右旋转
void _DCMotor::Car_RightTime(uint8_t speed, uint16_t time)
{
	SpeedCtr(speed, speed * (-1));
	StartUp();
	delay(time);
	Stop();
}
/***************************************************************
** 功能：     电机控制函数
** 参数：	  L_Spend：电机左轮速度
**            R_Spend：电机右轮速度
** 返回值：   无
****************************************************************/
void _DCMotor::Control(int16_t L_Spend, int16_t R_Spend)
{
	if (L_Spend >= 0)
	{
		if (L_Spend > 100)L_Spend = 100; if (L_Spend < 5)L_Spend = 5;		//限制速度参数
	}
	else
	{
		if (L_Spend < -100)L_Spend = -100; if (L_Spend > -5)L_Spend = -5;     //限制速度参数
	}
	if (R_Spend >= 0)
	{
		if (R_Spend > 100)R_Spend = 100; if (R_Spend < 5)R_Spend = 5;		//限制速度参数
	}
	else
	{
		if (R_Spend < -100)R_Spend = -100; if (R_Spend > -5)R_Spend = -5;		//限制速度参数
	}
	SpeedCtr(L_Spend, R_Spend);
}

#if 0
void _DCMotor::Track_Terrain(uint8_t Car_Spend, uint16_t temp_MP)
{
	gd = ExtSRAMInterface.ExMem_Read(TRACK_ADDR);
	ogd = ExtSRAMInterface.ExMem_Read(TRACK_ogd);

	if (Terrain_Flag == 1 || Terrain_Flag == 2)
	{
		if (temp_MP <= Roadway_mp_Get())
		{
			Terrain_Flag = 0;
			SpeedCtr(0, 0);
		}
	}
	if (Terrain_Flag == 1)
	{
		if (gd == 0x18)						//00011000
		{
			LSpeed = Car_Spend;
			RSpeed = Car_Spend;
		}
		else if (gd == 0x08)				//00001000 || 00001100
		{
			LSpeed = Car_Spend + 40;				//微右转
			RSpeed = Car_Spend - 50;
		}
		else if (gd == 0x0C)
		{
			LSpeed = Car_Spend + 60;				//微右转
			RSpeed = Car_Spend - 70;
		}
		else if ((gd == 0x04) || (gd == 0x06))				//00000100 || 00000110
		{
			LSpeed = Car_Spend + 60;				//右转
			RSpeed = Car_Spend - 70;
		}
		else if ((gd == 0x02) || (gd == 0x03))				//00000010 || 00000011
		{
			LSpeed = Car_Spend + 70;				//强右转
			RSpeed = Car_Spend - 90;
		}
		else if (gd == 0x01)												//00000001
		{
			LSpeed = Car_Spend + 80;				//强强右转
			RSpeed = Car_Spend - 120;
		}
		else if (gd == 0x10)				//00010000
		{
			RSpeed = Car_Spend + 40;			//微左
			LSpeed = Car_Spend - 50;
		}
		else if (gd == 0x30)				// 00110000
		{
			RSpeed = Car_Spend + 60;
			LSpeed = Car_Spend - 70;
		}
		else if ((gd == 0x20) || (gd == 0x60))				//00100000 || 01100000
		{
			RSpeed = Car_Spend + 60;			//左
			LSpeed = Car_Spend - 70;
		}
		else if ((gd == 0x40) || (gd == 0xC0))				//01000000 || 11000000
		{
			RSpeed = Car_Spend + 70;			//强左
			LSpeed = Car_Spend - 90;
		}
		else if (gd == 0X80)												//10000000
		{
			RSpeed = Car_Spend + 80;			//强强左
			LSpeed = Car_Spend - 120;
		}
		else
		{
			LSpeed = Car_Spend;
			RSpeed = Car_Spend;
		}
		if (Terrain_Flag != 0)
		{
			Control(LSpeed, RSpeed);
		}
	}
	else if (Terrain_Flag == 2)
	{
		ogd = ogd & 0x3E;
		if (ogd == 0x18)						//00011000
		{
			LSpeed = Car_Spend;
			RSpeed = Car_Spend;
		}
		else if (ogd == 0x08)				//00001000 || 00001100
		{
			LSpeed = Car_Spend + 40;				//微右转
			RSpeed = Car_Spend - 50;
		}
		else if (ogd == 0x0C)
		{
			LSpeed = Car_Spend + 60;				//微右转
			RSpeed = Car_Spend - 70;
		}
		else if ((ogd == 0x04) || (ogd == 0x06))				//00000100 || 00000110
		{
			LSpeed = Car_Spend + 60;				//右转
			RSpeed = Car_Spend - 70;
		}
		else if ((ogd == 0x02) || (ogd == 0x03))				//00000010 || 00000011
		{
			LSpeed = Car_Spend + 70;				//强右转
			RSpeed = Car_Spend - 90;
		}
		else if (ogd == 0x01)												//00000001
		{
			LSpeed = Car_Spend + 80;				//强强右转
			RSpeed = Car_Spend - 120;
		}
		else if (ogd == 0x10)				//00010000
		{
			RSpeed = Car_Spend + 40;			//微左
			LSpeed = Car_Spend - 50;
		}
		else if (ogd == 0x30)				// 00110000
		{
			RSpeed = Car_Spend + 60;
			LSpeed = Car_Spend - 70;
		}
		else if ((ogd == 0x20) || (ogd == 0x60))				//00100000 || 01100000
		{
			RSpeed = Car_Spend + 60;			//左
			LSpeed = Car_Spend - 70;
		}
		else
		{
			LSpeed = Car_Spend;
			RSpeed = Car_Spend;
		}
		if (Terrain_Flag != 0)
		{
			Control(LSpeed, RSpeed);
		}
	}
}
#endif
uint8_t allline_blackFlag = 0;
uint8_t trackTer_mode = 0;
uint16_t TureTimeMP = 0;
uint8_t ago_H8l = 8;
uint8_t ago_gd = 0, ago_ogd = 0;
uint8_t H4_temp, L4_temp;
uint8_t H4_L, L4_L;
uint8_t Terrain_i;
int temp_Sp = 0;
uint8_t threeVertical_Flag = 0;
uint16_t ensure_i = 0;

void _DCMotor::Track_Terrain(uint8_t Car_Spend, uint16_t temp_MP)
{
	gd = ExtSRAMInterface.ExMem_Read(TRACK_ADDR);
	ogd = ExtSRAMInterface.ExMem_Read(TRACK_ogd);
	TureTimeMP = Roadway_mp_Get();
#if 1
	if (TureTimeMP < 300)
	{
		if ((gd == 0x00 || ogd == 0x00) && allline_blackFlag == 0)		//如果有一条循迹灯位全黑  那么就是不能普通循迹
		{
			//Send_InfoData_To_Fifo("jd", 2);
			allline_blackFlag = 1;
		}
		if (allline_blackFlag == 1)			//如果检测到全黑线，持续判断循迹灯的状态
		{
			H8_light = 0;
			Q7_light = 0;
			for (rw_i = 0x01; rw_i < 0x100; rw_i <<= 1)
			{
				if (rw_i & gd)
				{
					H8_light++;
				}
				if (rw_i & ogd)
				{
					Q7_light++;
				}
			}

			if ((gd == 0xFF || ogd == 0x7F || Q7_light >= 6 || H8_light >= 7) && (trackTer_mode == 0 || trackTer_mode == 3))
			{
				//斑马线 检测全亮循迹灯
				if ((Q7_light + H8_light) > 10 && (((gd & 0x01) == 0x01) || ((gd & 0x80) == 0x80)))
				{
					//Serial.print("-BM-");
					//Car_Go(70, 30);
					while (DCMotor.Roadway_mp_Get() < 100)
					{
						DCMotor.Control(90, 90);
					}
					trackTer_mode = 2;
					temp_Sp = 40;
				}
			}
			if ((trackTer_mode == 0 || trackTer_mode == 3) && (ogd == 0x00 || gd == 0x00))
			{	//检测虚线
				if ((ago_H8l < 4 && ago_H8l >= 2) && ensure_i > 3000)
				{
					//Serial.print("-XX-");
					trackTer_mode = 3;
				}
				ensure_i++;
			}
			ago_H8l = H8_light;
		}
		else		//如果没有检测到，
		{
			//Control(0, 0);
			trackTer_mode = 3;
		}
	}
	else
	{
		//Send_InfoData_To_Fifo("%",1);
		if (trackTer_mode == 0)
		{
			trackTer_mode = 1;
			ensure_i = 0;
			temp_Sp = 50;
			threeVertical_Flag = 0;
		}
	}

	//if()

#endif
//	trackTer_mode = 2;
	//temp_Sp = 40;
	if (trackTer_mode == 1)		//菱形情况
	{
		if (gd == 0x00)
		{
			trackTer_mode = 4;
		}
		else
		{
			//Serial.print("-LX-");
//检测到白，再检测黑大于10左右，就判断左右转
			if (threeVertical_Flag == 0)
			{
				H8_light = 0;
				Q7_light = 0;
				for (rw_i = 0x01; rw_i < 0x100; rw_i <<= 1)
				{
					if (rw_i & gd)
					{
						H8_light++;
					}
					if (rw_i & ogd)
					{
						Q7_light++;
					}
				}
				if ((H8_light + Q7_light) >= 4)
				{
					//Send_InfoData_To_Fifo("-1-" ,3);
					threeVertical_Flag = 1;
				}
			}
			else if (threeVertical_Flag == 1)
			{
				H8_light = 0;
				Q7_light = 0;
				for (rw_i = 0x01; rw_i < 0x100; rw_i <<= 1)
				{
					if (rw_i & gd)
					{
						H8_light++;
					}
					if (rw_i & ogd)
					{
						Q7_light++;
					}
				}
				if ((H8_light + Q7_light) < 6)
				{
					//Send_InfoData_To_Fifo("-2-" ,3);
					//Serial.print("-2-");
					threeVertical_Flag = 2;
				}
			}
			if (threeVertical_Flag == 2 /*&& TureTimeMP < 700*/)
			{
				H4_L = 0;
				L4_L = 0;
				H4_temp = ((gd & 0xF0) >> 4);
				L4_temp = gd & 0x0F;
				for (Terrain_i = 0x01; Terrain_i < 0x10; Terrain_i <<= 1)
				{
					if (Terrain_i & H4_temp)
					{
						H4_L++;
					}
					if (Terrain_i & L4_temp)
					{
						L4_L++;
					}
				}
				if (H4_L > L4_L && ensure_i < 4000)			//如果高四位亮的灯多一点
				{	//右转
					//Send_InfoData_To_Fifo("R",1);
					RSpeed = Car_Spend - 60; //(temp_Sp + 10);
					LSpeed = Car_Spend + 55;//temp_Sp;
	//				if(temp_Sp >= 0)
	//				{
	//					temp_Sp	-= 10;
	//				}
					ensure_i++;
					//&& (ensure_i < 2)
				}
				else if (L4_L > H4_L && ensure_i < 4000)			//如果低四位亮的灯多一点
				{	//左转
					//Send_InfoData_To_Fifo("L",1);
					RSpeed = Car_Spend + 55;//temp_Sp;
					LSpeed = Car_Spend - 60;//(temp_Sp+10);
	//				if(temp_Sp >= 0)
	//				{
	//					temp_Sp	-= 10;
	//				}
					ensure_i++;
				}
				else
				{
					LSpeed = Car_Spend;
					RSpeed = Car_Spend;
				}
				if (Terrain_Flag != 0)
				{
					Control(LSpeed, RSpeed);
				}
			}
		}
	}
	else if (trackTer_mode == 2)		//斑马线
	{
		if (ago_gd == 0x00 && TureTimeMP < 1000)		//上一刻全灭 要进入全白的地方  00000111
		{
			H4_L = 0;
			L4_L = 0;
			H4_temp = ((gd & 0xF0) >> 4);
			L4_temp = gd & 0x0F;
			for (Terrain_i = 0x01; Terrain_i < 0x10; Terrain_i <<= 1)
			{
				if (Terrain_i & H4_temp)
				{
					H4_L++;
				}
				if (Terrain_i & L4_temp)
				{
					L4_L++;
				}
			}
			if (H4_L > L4_L/* && ensure_i < 100*/)		//高4位亮的灯多一些   左转
			{
				//Send_InfoData_To_Fifo("R",1);
				//R_Flag = 1;
				//Serial.print("L:");
				RSpeed = Car_Spend + temp_Sp;//temp_Sp + 10//temp_Sp;
				LSpeed = Car_Spend - (temp_Sp + 5);
				//temp_Sp -= 10;
				//
				//if (temp_Sp <= 0)
				//{
				//	temp_Sp = 0;
				//}
				//Serial.print(temp_Sp, DEC);
				//ensure_i++;
			}
			else if (H4_L < L4_L/* && ensure_i < 100*/)			//低四位亮的灯多一些   右转
			{
				//Serial.print("R:");
				RSpeed = Car_Spend - (temp_Sp + 5);     //(temp_Sp + 10);
				LSpeed = Car_Spend + temp_Sp;//temp_Sp;
				//temp_Sp -= 10;
				////
				//if (temp_Sp <= 0)
				//{
				//	temp_Sp = 0;
				//}
				//Serial.print(temp_Sp, DEC);
			}
			else
			{
				LSpeed = Car_Spend;
				RSpeed = Car_Spend;
			}
			//Serial.print("RL:");
			//Serial.print(ensure_i,DEC);
			//ensure_i++;
		}
		ago_gd = gd;
		if (Terrain_Flag != 0)
		{
			Control(LSpeed, RSpeed);
		}
	}
	else if (trackTer_mode == 3)		//三竖线
	{
		//	Serial.print("-SS-");
			//	Control(0, 0);
				//while (1);
		//		Control(0, 0);
		//		Serial.print(ago_H8l,DEC);
#if 1
		ogd = ogd & 0x3E;
		if (ogd == 0x0C)			//0001100
		{
			LSpeed = Car_Spend + 45;				//微右转
			RSpeed = Car_Spend - 50;
			//Send_InfoData_To_Fifo("1",1);
		}
		else if ((ogd == 0x04) || (ogd == 0x06))				//0000100  || 0000110
		{
			LSpeed = Car_Spend + 50;				//右转
			RSpeed = Car_Spend - 60;
			//Send_InfoData_To_Fifo("2",1);
		}
		else if ((ogd == 0x02) || (ogd == 0x03))				//0000010 || 0000011
		{
			LSpeed = Car_Spend + 45;				//强右转		在这里可能出地形的时候可能回受到地形白色边缘的干扰然后突然向右转
			RSpeed = Car_Spend - 50;
			// Send_InfoData_To_Fifo("3",1);
		}
		else if (ogd == 0x01)												//0000001
		{
			LSpeed = Car_Spend + 80;				//强强右转
			RSpeed = Car_Spend - 120;
			//	Send_InfoData_To_Fifo("4",1);
		}
		else if (ogd == 0x18)						//0011000
		{
			RSpeed = Car_Spend + 45;			//微左
			LSpeed = Car_Spend - 50;
			//Send_InfoData_To_Fifo("5",1);
		}
		else if (ogd == 0x10)				//0010000
		{
			RSpeed = Car_Spend + 50;			//左转
			LSpeed = Car_Spend - 60;
			//Send_InfoData_To_Fifo("6",1);
		}
		else if (ogd == 0x30)				// 0110000
		{
			RSpeed = Car_Spend + 60;		//强左
			LSpeed = Car_Spend - 80;
			//Send_InfoData_To_Fifo("7",1);
		}
		else if ((ogd == 0x20) || (ogd == 0x60))				//0100000 || 1100000
		{
			RSpeed = Car_Spend + 80;			//强强左
			LSpeed = Car_Spend - 120;
			//Send_InfoData_To_Fifo("8",1);
		}
		else
		{
			LSpeed = Car_Spend;
			RSpeed = Car_Spend;
		}
		if (Terrain_Flag != 0)
		{
			Control(LSpeed, RSpeed);
		}
#endif
	}
	else if (trackTer_mode == 4)
	{
		SpeedCtr(Car_Spend, Car_Spend);
	}
	if (temp_MP <= TureTimeMP)
	{
		SpeedCtr(0, 0);
		//Send_InfoData_To_Fifo("mode", 4);
		//Num_Debug_Info(trackTer_mode);
		trackTer_mode = 0;
		temp_Sp = 0;
		ago_H8l = 8;

		threeVertical_Flag = 0;
		allline_blackFlag = 0;
		//Serial.print("ensure_i:");
		//Serial.print(ensure_i,DEC);
		ensure_i = 0;
		Terrain_Flag = 0;
		//Stop_Flag = 6;
	}
}

// 复杂路况循迹到十字路口
void _DCMotor::Car_Track(uint8_t Car_Spend)
{
	uint8_t Line_Flag = 0;
	uint8_t Crossing_Rfid_Flag = 0;

	uint8_t Track_Run_Flag = 0;
	uint32_t count = 0;
	unsigned long t = 0;
	StartUp();
	Roadway_mp_syn();
	Track_Run_Flag = 1;
	//while (Roadway_mp_Get() < 50)
	//{
	//	SpeedCtr(Spend, Spend);
	//}

	while (Roadway_mp_Get() < 20)
	{
		Control(90, 90);
	};
	while (Track_Run_Flag)
	{
		Tracking_light_Count(ExtSRAMInterface.ExMem_Read(TRACK_ADDR), ExtSRAMInterface.ExMem_Read(TRACK_ogd));
		if (Roadway_mp_Get() > 350 || Find_Cross_Rfid == 1)
			Crossing_Rfid_Flag = 1;
		gd = ExtSRAMInterface.ExMem_Read(TRACK_ADDR);
		ogd = ExtSRAMInterface.ExMem_Read(TRACK_ogd);
		if (gd == 0x00)	// 循迹灯全灭 停止
		{
			//Serial.print("1");
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		else if ((gd == 0xFF && ogd == 0x7f) && Find_Terrain_Flag == 1)		//地形
		{
			Find_Terrain_Flag = 0;
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		//		||gd == 0xFE||gd == 0xFC||					(	1111 1110  -  1111 1100 -)
		else if ((gd == 0xF8 || gd == 0xFE || gd == 0xFC) && Crossing_Rfid_Flag)     //1111 1000
		{
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}//		||gd == 0x7F||gd == 0x3F||					(	0111 1111  -  0011 1111 -)
		else if ((gd == 0x1F || gd == 0x7F || gd == 0x3F) && Crossing_Rfid_Flag)		//0001 1111
		{
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		else if ((gd == 0x7E) && Crossing_Rfid_Flag)  				//0111 1110  -  0011 1100
		{
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		else if ((gd == 0x7C) && Crossing_Rfid_Flag)								//0111 1100  -  0111 1000
		{
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		else if ((gd == 0x3E || gd == 0x1E || gd == 0x3C) && Crossing_Rfid_Flag) 			    	//	0011 1110  -  0001 1110 - 0011 1100
		{
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		else if (H8_light < 5 && Q7_light < 4)
		{
			//Serial.print("8");
			Track_Run_Flag = 0;
			SpeedCtr(0, 0);
		}
		else
		{
			if (gd == 0XE7)   //1、中间3/4传感器检测到黑线，全速运行										11100111
			{
				LSpeed = Car_Spend;
				RSpeed = Car_Spend;
			}
			if (gd == 0XF7)							//		1111 0111																11110111
			{
				LSpeed = Car_Spend + 40;			//2019年4月24日19:36:02 从40改34
				RSpeed = Car_Spend - 40;			//2019年4月24日16:55:23 从50改低到45，因为刚开始循迹会抖一下
			}
			if (gd == 0XEF)							//		1110 1111																	11101111
			{
				LSpeed = Car_Spend - 40;			//2019年4月24日16:55:23 从50改低到45，因为刚开始循迹会抖一下
				RSpeed = Car_Spend + 40;			//2019年4月24日19:36:02 从40改34
			}
			if (Line_Flag != 2)		//——————右————————————右———————————右————————
			{
				if (gd == 0XF3) //2、中间4、3传感器检测到黑线，微右拐					11110011
				{
					LSpeed = Car_Spend + 60;		//原：52
					RSpeed = Car_Spend - 70;		//原：62
					Line_Flag = 0;
				}
				else if (gd == 0XF9 || gd == 0XFB)//3、中间3、2传感器检测到黑线，再微右拐			11111001 	-		11111011
				{
					LSpeed = Car_Spend + 60;			//原：60		70
					RSpeed = Car_Spend - 70;			//原：70		80
					Line_Flag = 0;
				}
				else if (gd == 0XFC || gd == 0XFD)//4、中间2、1传感器检测到黑线，强右拐								11111100  	-		11111101
				{
					LSpeed = Car_Spend + 70;
					RSpeed = Car_Spend - 90;
					Line_Flag = 0;
				}
				else if (gd == 0XFE)//5、最右边1传感器检测到黑线，再强右拐								11111110
				{
					LSpeed = Car_Spend + 80;
					RSpeed = Car_Spend - 120;
					Line_Flag = 1;
				}
			}
			if (Line_Flag != 1)			//——————左————————————左————————————左—————————
			{
				if (gd == 0XCF)//6、中间6、5传感器检测到黑线，微左拐						11001111
				{
					RSpeed = Car_Spend + 60;
					LSpeed = Car_Spend - 70;
					Line_Flag = 0;
				}
				else if (gd == 0X9F || gd == 0XDF)//7、中间7、6传感器检测到黑线，再微左拐		 	10011111 		-		11011111
				{
					RSpeed = Car_Spend + 60;
					LSpeed = Car_Spend - 70;
					Line_Flag = 0;
				}
				else if (gd == 0X3F || gd == 0XBF)//8、中间8、7传感器检测到黑线，强左拐								 00111111 	-		10111111
				{
					RSpeed = Car_Spend + 70;
					LSpeed = Car_Spend - 90;
					Line_Flag = 0;
				}
				else if (gd == 0X7F)//9、最左8传感器检测到黑线，再强左拐		 						  01111111
				{
					RSpeed = Car_Spend + 80;
					LSpeed = Car_Spend - 120;
					Line_Flag = 2;
				}
			}//-----------------------------------------------------------------------------------------------------------
			if (gd == 0xFF)   //循迹灯全亮																						11111111
			{
				if (ogd == 0X77)   //1、中间4传感器检测到黑线，全速运行										0111 0111
				{
					LSpeed = Car_Spend;
					RSpeed = Car_Spend;
				}
				else if (ogd == 0X7B || ogd == 0x67)							//			0111 1011		-		0110 0111																11110111
				{
					LSpeed = Car_Spend + 40;
					RSpeed = Car_Spend - 50;
				}
				else if (ogd == 0X6F || ogd == 0x73)							//			0110 1111		-		0111 0011													11101111
				{
					LSpeed = Car_Spend - 50;
					RSpeed = Car_Spend + 40;
				}
				else
				{
					if (count > 15000)
					{
						//if (Roadway_mp_Get() > 1000)
						//{
							//Serial.print("mp:");
							//Serial.print(Roadway_mp_Get());
							//Serial.println(" ");
							//Serial.print("break:");
							//Serial.print("9");
							//Serial.println(" ");

						count = 0;
						SpeedCtr(0, 0);
						Track_Run_Flag = 0;
						Cba_Beep(50, 50, 3);
						//}
					}
					else
					{
						count++;
						LSpeed = Car_Spend;
						RSpeed = Car_Spend;
					}
				}
			}
			else
			{
				count = 0;
				//SpeedCtr(Spend, Spend);
			}
		}
		if (Track_Run_Flag != 0)
		{
			Control(LSpeed, RSpeed);
		}

		/*		if((millis() - tim_t) >= time)
		{
			Stop();
			delay(100);
			break;
		}*/
	}
}

// 复杂路况mp循迹
void _DCMotor::Car_Track(uint8_t Spend, uint16_t mp)
{
	uint8_t Crossing_Rfid_Flag = 0;
	uint8_t gd = 0;
	unsigned long t = 0;
	Roadway_mp_syn();
	//SpeedCtr(Spend, Spend);
	while (Roadway_mp_Get() < 20)
	{
		Control(90, 90);
	};
	while (true)
	{
		Tracking_light_Count(ExtSRAMInterface.ExMem_Read(TRACK_ADDR), ExtSRAMInterface.ExMem_Read(TRACK_ogd));
		if (Roadway_mp_Get() > 350)
			Crossing_Rfid_Flag = 1;
		gd = ExtSRAMInterface.ExMem_Read(TRACK_ADDR);
		if (gd == 0x00)	// 循迹灯全灭 停止
		{
			Stop();
			delay(100);
			break;
		}
		else if ((gd == 0xF8 || gd == 0xFE || gd == 0xFC) && Crossing_Rfid_Flag)    //				1111 1110  -  1111 1100  -	1111 1000
		{
			Stop();
			delay(100);
			break;
		}
		else if ((gd == 0x1F || gd == 0x7F || gd == 0x3F) && Crossing_Rfid_Flag)		//				0111 1111  -  0011 1111  -	 0001 1111
		{
			Stop();
			delay(100);
			break;
		}
		else if ((gd == 0xE0 || gd == 0x07																				//				1110 0000  -  0000 0111
			|| gd == 0xF0 || gd == 0x0F) && Crossing_Rfid_Flag)								//				1111 0000  -  0000 1111
		{
			Stop();
			delay(100);
			break;
		}
		else if ((gd == 0x7E || gd == 0x3C) && Crossing_Rfid_Flag)  							//				0111 1110  -  0011 1100
		{
			Stop();
			delay(100);
			break;
		}
		else if ((gd == 0x7C) && Crossing_Rfid_Flag)														//		  	0111 1100  -  0111 1000
		{
			Stop();
			delay(100);
			break;
		}
		else if ((gd == 0x3E || gd == 0x1E) && Crossing_Rfid_Flag) 							//				0011 1110  -  0001 1110
		{
			Stop();
			delay(100);
			break;
		}
		else if (H8_light < 5 && Q7_light < 4)
		{
			Stop();
			delay(100);
			break;
		}
		else
		{
			if (gd == 0xE7)
			{
				SpeedCtr(Spend, Spend);
			}
			else if (gd == 0xF7)
			{
				SpeedCtr(Spend + 40, Spend - 50);
			}
			else if (gd == 0XF3)
			{
				SpeedCtr(Spend + 60, Spend - 70);
			}
			else if ((gd == 0XFB) || (gd == 0XF9))
			{
				SpeedCtr(Spend + 60, Spend - 70);
			}
			else if ((gd == 0XFD) || (gd == 0XFC))
			{
				SpeedCtr(Spend + 70, Spend - 90);
			}
			else if (gd == 0XFE)
			{
				SpeedCtr(Spend + 80, Spend - 120);
			}
			else if (gd == 0XEF)
			{
				SpeedCtr(Spend - 50, Spend + 40);
			}
			else if (gd == 0XCF)
			{
				SpeedCtr(Spend - 70, Spend + 60);
			}
			else if ((gd == 0XDF) || (gd == 0X9F))
			{
				SpeedCtr(Spend - 70, Spend + 60);
			}
			else if ((gd == 0XBF) || (gd == 0X3F))
			{
				SpeedCtr(Spend - 90, Spend + 70);
			}
			else if (gd == 0X7F)
			{
				SpeedCtr(Spend - 120, Spend + 80);
			}
		}

		if (gd == 0xFF)   //循迹灯全亮
		{
			SpeedCtr(Spend, Spend);
			if ((millis() - t) > 1000)
			{
				Stop();
				delay(100);
				break;
			}
		}
		else
		{
			t = millis();
		}
		if (Roadway_mp_Get() >= mp)
		{
			Stop();
			delay(100);
			break;
		}
	}
}

uint8_t _DCMotor::SearchBit(uint8_t mode, uint8_t s)
{
	if (mode == 1)
	{
		for (size_t i = 0x80; i > 0x00; i >>= 1)
		{
			if ((s & i) == 0)	return i;
		}
		return 0;
	}
	else
	{
		for (size_t i = 0x01; i < 0x100; i <<= 1)
		{
			if ((s & i) == 0)	return i;
		}
		return 0xff;
	}
}

/************************************************************************************************************
【函 数 名】：  RightMotorSpeed	设置PWM波的占空比，频率保持最后一次设置值  通道A-----pin6，通道B-----pin7
【参数说明】：	_Fduty	：设置通道 A 输出PWM波的占空比
				_Bduty	：设置通道 B 输出PWM波的占空比
【返 回 值】：	无
【简    例】：	RightMotorSpeed(70,0);
************************************************************************************************************/
void _DCMotor::MotorSpeed(uint8_t runmode, int8_t l_speed, int8_t r_speed)
{
	/*sbi(TCCR4A, COM4C1);
	sbi(TCCR4A, COM4B1);
	sbi(TCCR3A, COM3C1);
	sbi(TCCR3A, COM3B1);*/
	switch (runmode)
	{
	case MOVE_RUNMODE_STOP:
		PORTE &= ~(_BV(PE4) | _BV(PE5));
		PORTH &= ~(_BV(PH4) | _BV(PH5));
		OCR4C = 0x0000;
		OCR4B = 0x0000;
		OCR3C = 0x0000;
		OCR3B = 0x0000;
		break;
	case MOVE_RUNMODE_FF:
		OCR4C = 0;
		OCR4B = fHz * l_speed / 100;
		OCR3C = 0;
		OCR3B = fHz * r_speed / 100;
		break;
	case MOVE_RUNMODE_BB:
		OCR4B = 0;
		OCR4C = fHz * l_speed / 100;
		OCR3B = 0;
		OCR3C = fHz * r_speed / 100;
		break;
	case MOVE_RUNMODE_LIFT:
		OCR4C = 0;
		OCR4B = fHz * l_speed / 100;
		OCR3B = 0;
		OCR3C = fHz * r_speed / 100;
		break;
	case MOVE_RUNMODE_RIGHT:
		OCR4B = 0;
		OCR4C = fHz * l_speed / 100;
		OCR3C = 0;
		OCR3B = fHz * r_speed / 100;
		break;
	case MOVE_RUNMODE_TRACK:

		break;
	}
}

/************************************************************************************************************
【函 数 名】：  MotorFrequency	设置PWM波的频率，占空比保持最后一次设置值
【参数说明】：	_fHz	：
【返 回 值】：	无
【简    例】：	MotorFrequency(30000);
************************************************************************************************************/
void _DCMotor::MotorFrequency(uint32_t _fHz)
{
	// 配置预分频系数 0 0 1 不分频
	TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
	TCCR4B |= _BV(CS40);
	TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
	TCCR3B |= _BV(CS30);

	// f=16Mhz(mega2560时钟源)/(预分频系数)/(重装载值）
	fHz = 16000000 / _fHz;
	// 模式15--fast PWM该模式下即重装载值为OCRnA（输出比较寄存器）
	OCR3A = fHz;
	OCR4A = fHz;

	// 将BC通道值都置为0，通讯显示板正反转的显示：是通过判断对应引脚是否有脉冲，优先判断反转的引脚脉冲
	TCCR3A |= _BV(COM3B0);
	OCR3B = fHz;
	TCCR3A |= _BV(COM3C0);
	OCR3C = fHz;
	TCCR4A |= _BV(COM4B0);
	OCR4B = fHz;
	TCCR4A |= _BV(COM4C0);
	OCR4C = fHz;
}

///************************************************************************************************************
//【函 数 名】：  RightMotorFrequency	设置PWM波的频率，占空比保持最后一次设置值  通道A-----pin6，通道B-----pin7
//【参数说明】：	_fHz	：设置通道 A/B 输出PWM波的频率
//【返 回 值】：	无
//【简    例】：	RightMotorFrequency(7000);	//频率范围为245Hz～8MHz
//************************************************************************************************************/
//void _DCMotor::MotorFrequency(uint32_t _fHz)
//{
//	fHz = 16000000 / _fHz;
//	TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
//	TCCR4B |= _BV(CS40);
//	TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
//	TCCR3B |= _BV(CS30);
//
//	ICR4 = fHz;
//	ICR3 = fHz;
//}
/***************************************************END*****************************************************/

/***************************Timer5设置Fast PWM工作模式，以下是一些设置函数**********************************/

/************************************************************************************************************
【函 数 名】：  LeftMotorSpeed	设置PWM波的占空比，频率保持最后一次设置值  通道A-----pin46，通道B-----pin45
【参数说明】：	_front_speed	：设置通道 A 输出PWM波的占空比
				_behind_speed	：设置通道 B 输出PWM波的占空比
【返 回 值】：	无
【简    例】：	LeftMotorSpeed(70,50);
************************************************************************************************************/
void _DCMotor::LeftMotorSpeed(uint8_t _front_speed, uint8_t _behind_speed)
{
	//LfHz = TCNT3;
	LFSpeed = _front_speed;
	LBSpeed = _behind_speed;
	if ((_front_speed == 100) && (_behind_speed == 100))
	{
		sbi(TCCR3A, COM3C1);
		sbi(TCCR3A, COM3B1);
		OCR3C = 0xffff;// LfHz;
		OCR3B = 0xffff;// LfHz;
	}
	else
	{
		if (_front_speed >= 100)
		{
			sbi(TCCR3A, COM3C1);
			OCR3C = 0xffff;
		}
		else if (_front_speed <= 0)
		{
			sbi(TCCR3A, COM3C1);
			OCR3C = 0;
		}
		else
		{
			sbi(TCCR3A, COM3C1);
			OCR3C = LfHz * _front_speed / 100;
		}

		if (_behind_speed >= 100)
		{
			sbi(TCCR3A, COM3B1);
			OCR3B = 0xffff;
		}
		else if (_behind_speed <= 0)
		{
			sbi(TCCR3A, COM3B1);
			OCR3B = 0;
		}
		else
		{
			sbi(TCCR3A, COM3B1);
			OCR3B = LfHz * _behind_speed / 100;
		}
	}
}

/************************************************************************************************************
【函 数 名】：  LeftMotorFrequency	设置PWM波的频率，占空比保持最后一次设置值  通道A-----pin46，通道B-----pin45
【参数说明】：	_fHz	：设置通道 A/B 输出PWM波的频率
【返 回 值】：	无
【简    例】：	LeftMotorFrequency(7000);
************************************************************************************************************/
void _DCMotor::LeftMotorFrequency(uint32_t _fHz)
{
	LfHz = _fHz;

	if (_fHz > 1000000)	//频率最大为1MHz
	{
		TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
		TCCR3B |= _BV(CS30);
		ICR3 = 16;
		LfHz = 16;
		LeftMotorSpeed(LFSpeed, LBSpeed);
	}
	else if (_fHz < 30)	//频率最小为30Hz
	{
		TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
		TCCR3B |= _BV(CS31);
		ICR3 = 0xffff;
		LfHz = 0xffff;
		LeftMotorSpeed(LFSpeed, LBSpeed);
	}
	else				//频率范围为30Hz~1MHz
	{
		LfHz = 16000000 / _fHz;
		if (LfHz >= 0xffff)
		{
			TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
			TCCR3B |= _BV(CS31);
			LfHz = 2000000 / _fHz;
			ICR3 = LfHz;
			LeftMotorSpeed(LFSpeed, LBSpeed);
		}
		else
		{
			TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
			TCCR3B |= _BV(CS30);
			/*LfHz = 16000000 / _fHz;*/
			ICR3 = LfHz;
			LeftMotorSpeed(LFSpeed, LBSpeed);
		}
	}
}

uint8_t _DCMotor::ShiftBitAdd(uint8_t n)
{
	uint8_t bit = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		/*if (((n >> i) & 0x01) == 0x01)	bit ++;*/
		bit += bitRead(n, i);
	}
	return bit;
}

uint8_t _DCMotor::JudgeAround(uint8_t n)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		if ((((n >> 3) & 0x01) == 0x00) && (((n >> 4) & 0x01) == 0x00))
		{
			return 8;
		}
		else if (((n >> (3 - i)) & 0x01) == 0x00)
		{
			return (3 - i);
		}
		else if (((n >> (4 + i)) & 0x01) == 0x00)
		{
			return (4 + i);
		}
	}
	return 9;
}