// DCMotor.h

#ifndef _DCMOTOR_h
#define _DCMOTOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define TRACK_ADDR		0x6000	//低八位循迹灯（gd）
#define TRACK_ogd		0x6001	//高八位循迹灯

#define	R_F_M_PIN		8		/*控制 右 前 边电机的引脚，输出PWM*/
#define	R_B_M_PIN		7		/*控制 右 后 边电机的引脚，输出PWM*/

#define	L_F_M_PIN		3		/*控制 左 前 边电机的引脚，输出PWM*/
#define	L_B_M_PIN		2		/*控制 左 后 边电机的引脚，输出PWM*/

#define R_CONTROL_PIN	5
#define L_CONTROL_PIN	6

#define DEFAULT_SPEED	50

#define MOVE_RUNMODE_STOP	0
#define MOVE_RUNMODE_FF		1
#define MOVE_RUNMODE_BB		2
#define MOVE_RUNMODE_LIFT	3
#define MOVE_RUNMODE_RIGHT	4
#define MOVE_RUNMODE_TRACK	5

class _DCMotor
{
public:
	_DCMotor();
	~_DCMotor();

	uint8_t H8_light;
	uint8_t Q7_light;
	uint8_t Find_Terrain_Flag;
	uint8_t Find_Cross_Rfid;
	uint8_t Go_Flag;
	unsigned long go_time;
	uint8_t RFID_Flag = 0;		// RFID标志位
	uint8_t Terrain_Flag = 0;	// 地形检测标志位

	void Initialization(uint32_t fHz = 80000);
	uint16_t Go(uint8_t speed, uint16_t _distance);
	void Back(uint8_t speed);
	uint16_t Back(uint8_t speed, uint16_t _distance);
	void TurnLeft(int8_t Lspeed, int8_t Rspeed);
	void TurnLeft(int8_t speed);
	void TurnRight(int8_t Lspeed, int8_t Rspeed);
	void TurnRight(int8_t speed);
	void Car_GoLR(uint8_t Lspeed, uint8_t Rspeed, uint16_t distance);
	void Stop(void);
	void StartUp(void);
	void CarTrack(uint8_t Car_Spend);
	void MotorSpeed(uint8_t runmode, int8_t l_speed, int8_t r_speed);
	void SpeedSetOne(int16_t s, uint8_t* c1, uint8_t* c2);
	void SpeedCtr(int16_t L_speed, int16_t R_speed);
	void SpeedCtr_2(int16_t L_speed, int16_t R_speed);
	void SpeedCtr_3(int16_t L_speed, int16_t R_speed);
	void ParameterInit(void);
	bool ClearCodeDisc(void);
	uint8_t SearchBit(uint8_t mode, uint8_t s);

	void Control(int16_t L_Spend, int16_t R_Spend);
	void _DCMotor::Go(uint8_t speed);
	void Tracking_light_Count(uint8_t gd, uint8_t ogd);
	void Roadway_mp_syn(void);		//码盘同步
	uint16_t Roadway_mp_Get(void);	//码盘获取
	void Car_Go(uint8_t speed, uint16_t distance);
	void Car_Back(uint8_t speed, uint16_t distance);
	void Car_BackBlackLine(uint8_t speed, uint16_t _distance);
	void Car_Left(uint8_t speed);
	void Car_Right(uint8_t speed);
	void Car_LeftTime(uint8_t speed, uint16_t time);	// 左旋转
	void Car_RightTime(uint8_t speed, uint16_t time);	// 右旋转
	void Car_Track(uint8_t Car_Spend);			// 复杂路况循迹
	void Track_Terrain(uint8_t Car_Spend, uint16_t temp_MP);
	void car_go(uint8_t speed, uint16_t distance);
	void Car_Track(uint8_t Car_Spend, uint16_t mp);	// 复杂路况时间循迹
	/* MY */
	uint16_t _DCMotor::TurnRightAngle(uint8_t speed, uint16_t _distance);
	uint16_t _DCMotor::TurnRightAngle_1(uint8_t* Alarm_buf, uint8_t speed, uint16_t _distance);//右
	uint16_t _DCMotor::TurnLeftAngle(uint8_t speed, uint16_t _distance);
	uint16_t _DCMotor::TurnLeftAngle_1(uint8_t* Alarm_buf, uint8_t speed, uint16_t _distance);//左
	/* MY_END */

private:
	boolean track_flag;

	uint32_t fHz;
	uint8_t  RFSpeed, RBSpeed;

	uint32_t LfHz;
	uint8_t  LFSpeed, LBSpeed;

	/*void MotorSpeed(uint8_t runmode, int8_t l_speed, int8_t r_speed);*/
	//void MotorFrequency(uint32_t /*PWM波的频率，单位：Hz*/);
	void MotorFrequency(uint32_t _fHz);

	void LeftMotorSpeed(uint8_t /*PWM波的占空比*/, uint8_t /*PWM波的占空比*/);
	void LeftMotorFrequency(uint32_t /*PWM波的频率，单位：Hz*/);

	//void StartUp(void);
	void ShutDown(void);

	uint8_t ShiftBitAdd(uint8_t);
	uint8_t JudgeAround(uint8_t);

	uint8_t speed[101];
	//uint8_t SearchBit(uint8_t mode, uint8_t s);
};

extern _DCMotor DCMotor;

#endif
