#include "DCMotor.h"
#include <Command.h>
#include "wiring_private.h"
#include <ExtSRAMInterface.h>
#include <Metro.h>
#include "CoreBeep.h"

/* ------------------------------------------------------------------------------------------------
 * 全局对象（保持不变）
 * ------------------------------------------------------------------------------------------------ */
_DCMotor DCMotor;
Metro DCMotorMetro(20);

/* ------------------------------------------------------------------------------------------------
 * 文件内常量/工具函数（仅对内）
 * ------------------------------------------------------------------------------------------------ */
namespace
{
	constexpr uint32_t kMotionTimeoutMs = 30000UL;

	constexpr uint16_t kDiscValidMin      = 20;
	constexpr uint16_t kDiscInvalidHighLt = 65516;   // (distance < 65516) 认为有效（转向时用）

	inline uint16_t ReadCodeDiscRaw()
	{
		const uint16_t lo = ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET);
		const uint16_t hi = ExtSRAMInterface.ExMem_Read(BASEADDRESS + CODEOFFSET + 1);
		return uint16_t(lo | (hi << 8));
	}

	inline void WriteCommand01ToExtSram()
	{
		Command.Judgment(Command.command01);
		for (size_t i = 0; i < 8; i++)
		{
			ExtSRAMInterface.ExMem_JudgeWrite(WRITEADDRESS + i, Command.command01[i]);
		}
	}

	inline void WaitCodeDiscCleared()
	{
		// ClearCodeDisc(): false=已清零，true=未清零（需继续清）
		while (DCMotor.ClearCodeDisc())
		{
			// busy wait
		}
	}
}

/* ------------------------------------------------------------------------------------------------
 * 码盘同步变量（外部不需要知道，限制在本文件内）
 * ------------------------------------------------------------------------------------------------ */
static int16_t g_canHostMp  = 0;   // 当前 mp
static int16_t g_roadwayCmp = 0;   // 同步时记录的 mp（基准）

/* ------------------------------------------------------------------------------------------------
 * 构造 / 析构
 * ------------------------------------------------------------------------------------------------ */
_DCMotor::_DCMotor()
{
	ExtSRAMInterface.Initialization();
}

_DCMotor::~_DCMotor()
{
}

/* ------------------------------------------------------------------------------------------------
 * 码盘同步
 * ------------------------------------------------------------------------------------------------ */
void _DCMotor::Roadway_mp_sync(void)
{
	g_roadwayCmp = int16_t(ReadCodeDiscRaw());
}

/* ------------------------------------------------------------------------------------------------
 * 码盘差值获取（带环绕处理，保持你原逻辑）
 * ------------------------------------------------------------------------------------------------ */
uint16_t _DCMotor::Roadway_mp_Get(void)
{
	uint32_t diff;

	g_canHostMp = int16_t(ReadCodeDiscRaw());

	if (g_canHostMp > g_roadwayCmp)
		diff = uint32_t(g_canHostMp - g_roadwayCmp);
	else
		diff = uint32_t(g_roadwayCmp - g_canHostMp);

	if (diff > 0x8000)
		diff = 0xFFFFu - diff;

	return uint16_t(diff);
}

/************************************************************************************************************
【函 数 名】：	Initialization	直流电机初始化
【参数说明】：	fHz：初始化PWM输出频率，单位：Hz
************************************************************************************************************/
void _DCMotor::Initialization(uint32_t fHz)
{
	pinMode(L_CONTROL_PIN, OUTPUT);
	pinMode(R_CONTROL_PIN, OUTPUT);

	pinMode(R_F_M_PIN, OUTPUT);
	pinMode(R_B_M_PIN, OUTPUT);
	pinMode(L_F_M_PIN, OUTPUT);
	pinMode(L_B_M_PIN, OUTPUT);

	digitalWrite(L_CONTROL_PIN, HIGH);
	digitalWrite(R_CONTROL_PIN, HIGH);

	// Timer4 / Timer3：Fast PWM，模式 15（TOP=OCRnA）
	TCCR4A |= _BV(WGM41) | _BV(WGM40);
	TCCR4B |= _BV(WGM42) | _BV(WGM43);

	TCCR3A |= _BV(WGM31) | _BV(WGM30);
	TCCR3B |= _BV(WGM32) | _BV(WGM33);

	// 输出通道：非反相
	TCCR4A |= _BV(COM4C1) | _BV(COM4B1);
	TCCR3A |= _BV(COM3C1) | _BV(COM3B1);

	MotorFrequency(fHz);
	ParameterInit();
}

void _DCMotor::ParameterInit(void)
{
	for (uint8_t i = 0; i <= 100; i++)
	{
		speed[i] = uint16_t((uint32_t)fHz * i / 100);
	}
}

/* s: -100~100 -> 输出两路PWM比较值 */
void _DCMotor::SpeedSetOne(int16_t s, uint16_t* c1, uint16_t* c2)
{
	uint8_t t = (s >= 0) ? uint8_t(s) : uint8_t(-s);

	if (t > 100) t = 100;
	if (t < 5)   t = 5;

	if (s == 0)
	{
		*c1 = speed[100];
		*c2 = speed[100];
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
	uint16_t ocr3b, ocr3c, ocr4b, ocr4c;

	// 左轮：Timer4 B/C
	SpeedSetOne(L_speed, &ocr4c, &ocr4b);
	// 右轮：Timer3 B/C
	SpeedSetOne(R_speed, &ocr3b, &ocr3c);

	// 保留原逻辑：当比较值为0时用 COMx0 处理
	(ocr3b == 0) ? (TCCR3A |= _BV(COM3B0), ocr3b = fHz) : (TCCR3A &= ~_BV(COM3B0));
	(ocr3c == 0) ? (TCCR3A |= _BV(COM3C0), ocr3c = fHz) : (TCCR3A &= ~_BV(COM3C0));
	(ocr4b == 0) ? (TCCR4A |= _BV(COM4B0), ocr4b = fHz) : (TCCR4A &= ~_BV(COM4B0));
	(ocr4c == 0) ? (TCCR4A |= _BV(COM4C0), ocr4c = fHz) : (TCCR4A &= ~_BV(COM4C0));

	OCR4C = ocr4c;
	OCR4B = ocr4b;
	OCR3C = ocr3c;
	OCR3B = ocr3b;
}

/*
 * ClearCodeDisc()
 * 返回语义保持原项目习惯：
 *   false：码盘已清零（成功）
 *   true ：码盘未清零（还需继续清）
 */
bool _DCMotor::ClearCodeDisc(void)
{
	WriteCommand01ToExtSram();

	DCMotorMetro.interval(20);

	for (size_t i = 0; i < 5; i++)
	{
		if (DCMotorMetro.check() == 1)
		{
			const uint16_t distance = ReadCodeDiscRaw();
			if (distance == 0x0000)
			{
				return false; // 已清零
			}
		}
	}
	return true; // 未清零
}

/*
左转任意角度（原地旋转：左轮后退、右轮前进）
**/
uint16_t _DCMotor::TurnLeftAngle(uint8_t speed, uint16_t _distance)
{
	uint16_t distance = 0;
	const unsigned long startMs = millis();

	WaitCodeDiscCleared();

	// 原地左转：左轮反转，右轮正转
	SpeedCtr(int16_t(speed) * (-1), int16_t(speed));
	StartUp();

	while (true)
	{
		distance = ReadCodeDiscRaw();

		if ((kDiscInvalidHighLt > distance) && (distance > kDiscValidMin))
		{
			// 保留原逻辑：左转计数会从 0 往 65535 方向“回绕”
			const uint16_t moved = uint16_t(65536u - distance);
			if ((moved >= _distance) || ((millis() - startMs) > kMotionTimeoutMs))
			{
				Stop();
				delay(50);
				distance = ReadCodeDiscRaw();
				break;
			}
		}

		delay(1);
	}

	return uint16_t(65536u - distance);
}

/*
右转任意角度（原地旋转：左轮前进、右轮后退）
**/
uint16_t _DCMotor::TurnRightAngle(uint8_t speed, uint16_t _distance)
{
	uint16_t distance = 0;
	const unsigned long startMs = millis();

	WaitCodeDiscCleared();

	// 原地右转：左轮正转，右轮反转
	SpeedCtr(int16_t(speed), int16_t(speed) * (-1));
	StartUp();

	while (true)
	{
		distance = ReadCodeDiscRaw();

		if ((kDiscInvalidHighLt > distance) && (distance > kDiscValidMin))
		{
			// 保留原逻辑：右转计数从 0 往上增
			if ((distance >= _distance) || ((millis() - startMs) >= kMotionTimeoutMs))
			{
				Stop();
				delay(50);
				distance = ReadCodeDiscRaw();
				break;
			}
		}

		delay(1);
	}

	return distance;
}

/************************************************************************************************************
【函 数 名】：  Stop	小车停止函数（锁死）
************************************************************************************************************/
void _DCMotor::Stop(void)
{
	SpeedCtr(0, 0);
	PORTE |= _BV(PE3);
	PORTH |= _BV(PH3);
}

/************************************************************************************************************
【函 数 名】：  StartUp	小车启动函数
************************************************************************************************************/
void _DCMotor::StartUp(void)
{
	PORTE |= _BV(PE3);
	PORTH |= _BV(PH3);
}

/************************************************************************************************************
【函 数 名】：  ShutDown	小车关闭函数
************************************************************************************************************/
void _DCMotor::ShutDown(void)
{
	SpeedCtr(100, 100);
	PORTE |= _BV(PE3);
	PORTH |= _BV(PH3);
}

/********************************************* 自定义函数 ****************************************************/
// 前进
void _DCMotor::Car_Go(uint8_t speed, uint16_t distance)
{
	uint16_t cardis = 0;
	const unsigned long startMs = millis();

	WaitCodeDiscCleared();

	SpeedCtr(speed, speed);
	StartUp();

	while (true)
	{
		delay(1);
		cardis = ReadCodeDiscRaw();

		if ((32768u > cardis) && (cardis > 0))
		{
			if ((cardis >= distance) || ((millis() - startMs) >= kMotionTimeoutMs))
			{
				Stop();
				delay(100);
				break;
			}
		}
	}
}

// 后退
void _DCMotor::Car_Back(uint8_t speed, uint16_t distance)
{
	uint16_t cardis = 0;
	const unsigned long startMs = millis();

	WaitCodeDiscCleared();

	SpeedCtr(int16_t(speed) * (-1), int16_t(speed) * (-1));
	StartUp();

	while (true)
	{
		delay(1);
		cardis = ReadCodeDiscRaw();

		if ((65536u > cardis) && (cardis > 32768u))
		{
			const uint16_t moved = uint16_t(65536u - cardis);
			if ((moved >= distance) || ((millis() - startMs) > kMotionTimeoutMs))
			{
				Stop();
				delay(100);
				break;
			}
		}
	}
}

/************************************************************************************************************
【函 数 名】：  MotorFrequency	设置PWM波的频率，占空比保持最后一次设置值
【参数说明】：	_fHz：目标 PWM 频率（Hz）
************************************************************************************************************/
void _DCMotor::MotorFrequency(uint32_t _fHz)
{
	// 预分频：不分频（CSn0 = 1）
	TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
	TCCR4B |= _BV(CS40);

	TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
	TCCR3B |= _BV(CS30);

	// TOP = 16MHz / f_pwm
	fHz = uint16_t(16000000UL / _fHz);

	// Fast PWM 模式 15（TOP=OCRnA）
	OCR3A = fHz;
	OCR4A = fHz;

	// BC 通道初始状态：通过判断引脚是否有脉冲显示正反转（保留原逻辑）
	TCCR3A |= _BV(COM3B0); OCR3B = fHz;
	TCCR3A |= _BV(COM3C0); OCR3C = fHz;
	TCCR4A |= _BV(COM4B0); OCR4B = fHz;
	TCCR4A |= _BV(COM4C0); OCR4C = fHz;
}
