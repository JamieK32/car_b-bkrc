#include "Routeplan.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "My_Lib.h"
#include "ExtSRAMInterface.h"

extern uint8_t AGV_GO_sp;        // 小车前进速度
extern uint8_t AGV_wheel_sp;     // 小车转弯速度
extern uint16_t AGV_GO_mp;      // 小车前进码盘

uint8_t i_Coordinate;      //i坐标
uint8_t j_Coordinate;      //j坐标
uint8_t Run_Flag;
uint8_t Ago_Direct;		//之前的方向
uint8_t Run_Direct;		//要动作的方向

uint8_t CorrectX;
uint8_t CorrectY;
uint8_t Correct_X;
uint8_t Correct_Y;
uint8_t	Correct_Front;

uint8_t AgoBit4;
uint8_t AfterBit4;
uint8_t AgoBit4_buf;
uint8_t AfterBit4_buf;

uint8_t Letter_Gap;
uint8_t digital_Gap;

uint8_t Act_Array[30];

uint16_t i_Act = 0;

/*************************************************************************************************/
/***********************************************************************************************/
//YYYYK
void Correct_direction(uint8_t X_buf, uint8_t _X_buf, uint8_t Y_buf, uint8_t _Y_buf)//  1 2 3 4
{
	CorrectX = X_buf;
	CorrectY = Y_buf;
	Correct_X = _X_buf;
	Correct_Y = _Y_buf;
}
void Correct_Front_Orienta(uint8_t Correct_Front_buf)
{
	if (Correct_Front_buf == CorrectX)
	{
		Correct_Front = X;//1
		Ago_Direct = Correct_Front;//之前的方向
		//Serial.print("-X-");
		//Send_InfoData_To_Fifo("X", 1);
	}
	else if (Correct_Front_buf == CorrectY)
	{
		Correct_Front = Y;//4
		Ago_Direct = Correct_Front;
		//Serial.print("-Y-");
		//Send_InfoData_To_Fifo("Y", 1);
	}
	else if (Correct_Front_buf == Correct_X)
	{
		Correct_Front = _X;//3
		Ago_Direct = Correct_Front;
		//Serial.print("-_X-");
		//Send_InfoData_To_Fifo("_X", 2);
	}
	else if (Correct_Front_buf == Correct_Y)
	{
		Correct_Front = _Y;//2
		Ago_Direct = Correct_Front;
		//Serial.print("-_Y-");
		//Send_InfoData_To_Fifo("_Y", 2);
	}
}
//初始坐标                   //行进坐标数组
void Route_Plan(uint8_t Initial_Coordinate, uint8_t* Route_Coordinate)
{
	uint8_t len = 0;
	len = strlen((char*)Route_Coordinate);     //计算行进坐标数组的长度
					//初始坐标          //行进坐标数组第一位
	Calculate_Act(Initial_Coordinate, Route_Coordinate[0]);

	for (i_Coordinate = 0; i_Coordinate < len - 1; i_Coordinate++)
	{
		Calculate_Act(Route_Coordinate[i_Coordinate], Route_Coordinate[i_Coordinate + 1]);
	}
}
uint8_t SpecialLetters_Flag = 0;
uint8_t SpecialNumber_Flag = 0;
uint8_t JG_Start_GarageFlag;		//开始为车库标志位
uint8_t temp_ago;
//	uint8_t JG_After_GarageFlag;		//最后为车库标志位
void Calculate_Act(uint8_t ago, uint8_t after)
{
	if (ago == 0)
	{
		ago = temp_ago;
	}
	temp_ago = after;
	AgoBit4 = ago >> 4 & 0x0F;		//字母
	AfterBit4 = ago & 0x0F;			//数字
	AgoBit4_buf = after >> 4 & 0x0F;//字母
	AfterBit4_buf = after & 0x0F;	//数字
	//Serial.print("ago:");
	//Serial.print(ago, HEX);
	//Serial.print("after:");
	//Serial.print(after, HEX);

	//Letter_Gap = abs(AgoBit4 - AgoBit4_buf);		//字母差
	//digital_Gap = abs(AfterBit4 - AfterBit4_buf);	//数字差

	if (AgoBit4 == 0x06)		//如果开始点在在G点
	{
		AgoBit4 = 0x10;
	}
	if (AgoBit4_buf == 0x06)		//如果结束点在G点
	{
		AgoBit4_buf = 0x10;
	}
	if (AfterBit4 == 0x07)		   //初始坐标在下库
	{
		//JG_Start_GarageFlag = 1;
		AfterBit4 = AfterBit4 - 1;
		Act_Assign(Judge_Direct(Y));
		Act_Assign(4);
	}
	else if (AfterBit4 == 0x01)		//初始坐标在上库
	{
		AfterBit4 = AfterBit4 + 1;
		Act_Assign(Judge_Direct(_Y));
		Act_Assign(4);
	}
	else if (AgoBit4 == 0x0A)		//初始坐标在左库
	{
		AgoBit4 = AgoBit4 + 1;
		Act_Assign(Judge_Direct(X));
		Act_Assign(5);
	}
	else if (AgoBit4 == 0x10)		//初始坐标在右库
	{
		AgoBit4 = AgoBit4 - 1;
		Act_Assign(Judge_Direct(_X));
		Act_Assign(5);
	}
	else
	{
	}
	//		{
		/* 判断特殊点位 */
	while (1)
	{
		Letter_Gap = abs(AgoBit4 - AgoBit4_buf);
		digital_Gap = abs(AfterBit4 - AfterBit4_buf);
		//			AgoBit4 = ago >> 4 & 0x0F;
		//			AfterBit4 = ago & 0x0F;
				//必须要先判断出自己是不是在特殊的点位

		if (AgoBit4 == 0x0E || AgoBit4 == 0x0C)
		{
			//Printf_Debug("-S1-",4);
			SpecialLetters_Flag = 1;
		}
		else if (AfterBit4 == 0x05 || AfterBit4 == 0x03)
		{
			//Printf_Debug("-S2-",4);
			SpecialNumber_Flag = 1;
		}
		//判断最后点位 在上下
		if ((AgoBit4 == AgoBit4_buf) && (abs(AfterBit4 - AfterBit4_buf)) == 1)
		{
			//Printf_Debug("-1-",3);
			if (AfterBit4_buf == 0x07)		//判断最后点位是否为车库
			{
				//最后点位为下库
				//Printf_Debug("-11-",4);
				AfterBit4 = AfterBit4 + 1;
				Act_Assign(Judge_Direct(Y));
				Act_Assign(0x0B);
				break;
			}
			else if (AfterBit4_buf == 0x01)
			{	//在上库
				//Printf_Debug("-12-",4);
				AfterBit4 = AfterBit4 - 1;
				Act_Assign(Judge_Direct(_Y));
				Act_Assign(0x0B);
				break;
			}

			//数字位相差为1
			if (AfterBit4 > AfterBit4_buf)								//为短的两十字路口中间
			{
				//朝上
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				//Printf_Debug("-13-",4);
				AfterBit4 = AfterBit4 - 1;
				if (SpecialNumber_Flag == 1)
				{
					SpecialNumber_Flag = 0;
					Act_Assign(Judge_Direct(Y));
					Act_Assign(1);
				}
				else
				{
					Act_Assign(Judge_Direct(Y));
					Act_Assign(3);
				}
				break;
			}
			else if (AfterBit4 <= AfterBit4_buf)
			{
				//朝下
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				//Printf_Debug("-14-",4);
				AfterBit4 = AfterBit4 + 1;
				//					Act_Assign(Judge_Direct(_Y));
				//					Act_Assign(3);
				if (SpecialNumber_Flag == 1)
				{
					SpecialNumber_Flag = 0;
					Act_Assign(Judge_Direct(_Y));
					Act_Assign(1);
				}
				else
				{
					Act_Assign(Judge_Direct(_Y));
					Act_Assign(3);
				}
				break;
			}
		}
		//判断最后点位在左右
		else if ((AfterBit4 == AfterBit4_buf) && (abs(AgoBit4 - AgoBit4_buf)) == 1)
		{
			//Printf_Debug("-2-",3);
			if (AgoBit4_buf == 0x0A)		//判断最后点位是否为车库
			{
				//最后点位为左库
				//Printf_Debug("-23-",4);
				AgoBit4 = AgoBit4 - 1;
				Act_Assign(Judge_Direct(X));
				Act_Assign(0x0C);
				break;
			}
			else if (AgoBit4_buf == 0x10)		//右
			{
				//Printf_Debug("-24-",4);
				AgoBit4 = AgoBit4 + 1;
				Act_Assign(Judge_Direct(_X));
				Act_Assign(0x0C);
				break;
			}
			if (AgoBit4 > AgoBit4_buf)						//为长的两十字路口中间
			{
				//朝左
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				//Printf_Debug("-25-",4);
				AgoBit4 = AgoBit4 - 1;
				if (SpecialLetters_Flag == 1)
				{
					SpecialLetters_Flag = 0;
					Act_Assign(Judge_Direct(_X));
					Act_Assign(1);
				}
				else
				{
					Act_Assign(Judge_Direct(_X));
					Act_Assign(2);
				}
				break;
			}
			else if ((AgoBit4 <= AgoBit4_buf))
			{
				//朝右
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				//Printf_Debug("-26-",4);
				AgoBit4 = AgoBit4 + 1;
				if (SpecialLetters_Flag == 1)
				{
					SpecialLetters_Flag = 0;
					Act_Assign(Judge_Direct(X));
					Act_Assign(1);
				}
				else
				{
					Act_Assign(Judge_Direct(X));
					Act_Assign(2);
				}
				break;

				//break;
			}
		}
		if ((AfterBit4 == AfterBit4_buf) && (AgoBit4 == AgoBit4_buf))
		{
			//Printf_Debug("-B-",4);
			break;
		}
		/* 行进规划 */
		if (((Letter_Gap <= digital_Gap) || (SpecialNumber_Flag == 1)) && SpecialLetters_Flag != 1)		//数字差值比字母差值大  先判断数字
		{
			//Printf_Debug("-3-",3);
			if (AfterBit4 > AfterBit4_buf)
			{
				//朝上
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				//Printf_Debug("-31-",4);
				if (SpecialNumber_Flag == 1)
				{
					SpecialNumber_Flag = 0;
					AfterBit4 = AfterBit4 - 1;
				}
				else
				{
					AfterBit4 = AfterBit4 - 2;
				}
				Act_Assign(Judge_Direct(Y));
				Act_Assign(1);
			}
			else if (AfterBit4 <= AfterBit4_buf)
			{
				//Printf_Debug("-32-",4);
				//朝下
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				if (SpecialNumber_Flag == 1)
				{
					SpecialNumber_Flag = 0;
					AfterBit4 = AfterBit4 + 1;
				}
				else
				{
					AfterBit4 = AfterBit4 + 2;
				}
				Act_Assign(Judge_Direct(_Y));
				Act_Assign(1);
			}
		}
		else if (((Letter_Gap > digital_Gap) || (SpecialLetters_Flag == 1)) && SpecialNumber_Flag != 1)		//字母差值比数字差值大	先判断字母
		{
			//Printf_Debug("-4-",3);
			if (AgoBit4 > AgoBit4_buf)
			{
				//Printf_Debug("-41-",4);
				//朝左
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				if (SpecialLetters_Flag == 1)
				{
					SpecialLetters_Flag = 0;
					//Printf_Debug("-411-",5);
					AgoBit4 = AgoBit4 - 1;
				}
				else
				{
					//Printf_Debug("-412-",5);
					AgoBit4 = AgoBit4 - 2;
				}
				Act_Assign(Judge_Direct(_X));
				Act_Assign(1);
			}
			else if ((AgoBit4 <= AgoBit4_buf))
			{
				//Printf_Debug("-42-",4);
				//朝右
				//TGO();
				//Run_Flag = Judge_Direct(Run_Direct);
				if (SpecialLetters_Flag == 1)
				{
					SpecialLetters_Flag = 0;
					//Printf_Debug("-43-",4);
					AgoBit4 = AgoBit4 + 1;
				}
				else
				{
					//Printf_Debug("-44-",4);
					AgoBit4 = AgoBit4 + 2;
				}
				Act_Assign(Judge_Direct(X));
				Act_Assign(1);
			}
		}
	}
}

void Act_Assign(uint8_t Act_flag)
{
	if (Act_flag != 0)
	{
		Act_Array[i_Act] = Act_flag;
		i_Act++;
	}
}

void Take_Action(uint8_t* Action_Array)
{
	uint8_t len = 0;
	len = strlen((char*)Action_Array);
	for (i_Coordinate = 0; i_Coordinate < len; i_Coordinate++)
	{
		switch (Action_Array[i_Coordinate])
		{
		case 1:
			//tarck();
			//go();
			//Car_Track(AGV_GO_sp);
			//Car_Go(AGV_GO_sp, AGV_GO_mp);
			Judge_terrain();
			break;
		case 2:		// 2 3 主要用到两个路口中间的mp循迹
			//Mp_tarck(1);		//1100MP
			Car_TrackMP(55, 1100);
			break;
		case 3:
			//Mp_tarck(2);		//850MP
			Car_TrackMP(55, 850);
			break;
		case 4:						//4 5 用在出车库
			//Out_Garage(1);	//1150
			Car_Go(AGV_GO_sp, 1150);
			break;
		case 5:
			//Out_Garage(2);	//850
			Car_Go(AGV_GO_sp, 850);
			break;
		case 8:
			//Left();	//8
			Car_RorL(1, AGV_wheel_sp, LM);	/*左转90*/
			break;
		case 9:
			//Right();	//9
			//Right();

			Car_RorL(2, AGV_wheel_sp, RM);	/*右转90*/
			Car_RorL(2, AGV_wheel_sp, RM);	/*右转90*/
			break;
		case 10:
			//Right();		//0x0A
			Car_RorL(2, AGV_wheel_sp, RM);	/*右转90*/
			break;
		case 0x0B:
			//dcrk(700);
			Back_ruku(1);
			break;
		case 0x0C:
			//dcrk(450);
			Back_ruku(2);
			break;
		default:
			break;
			// 0x0B		长入库
			//0x0C  短入库
		}
	}
	for (i_Coordinate = 0; i_Coordinate < len; i_Coordinate++)
	{
		Action_Array[i_Coordinate] = 0x00;
	}
	i_Act = 0;
}
uint8_t Judge_CrsrdOrBTTCrsrd(uint8_t JC_ago, uint8_t JC_aft)		//判断是否是十字路口或者是两个十字路口中间
{
	uint8_t CrsrdOrBTTCrsrd = 0;
	if (JC_ago == 0x0C || JC_ago == 0x0E)		//1100
	{
		CrsrdOrBTTCrsrd = 2;
		return CrsrdOrBTTCrsrd;
	}
	else if (JC_aft == 0x05 || JC_aft == 0x03)		//850
	{
		CrsrdOrBTTCrsrd = 3;
		return CrsrdOrBTTCrsrd;
	}
	else if (JC_ago == 0x0B || JC_ago == 0x0D || 		//TGO
		JC_ago == 0x0F || JC_ago == 0x02 || JC_ago == 0x04 || JC_ago == 0x06)
	{
		CrsrdOrBTTCrsrd = 1;
		return CrsrdOrBTTCrsrd;
	}
	else
	{
		return 0;
	}
	//return 0;
}

uint8_t Judge_Direct(uint8_t Run_D)//
{
	/* 小算法 */
	uint8_t Direct = 0;
	//Run_Direct

	if ((Ago_Direct - Run_D) == 1 || (Run_D - Ago_Direct) == 3)
	{
		//Left();	//8
		Direct = 8;
		Ago_Direct = Run_D;		//下一次进来的时候这一次的车头朝向就是下一次的Ago_Direct
		return Direct;
	}
	else if (abs(Run_D - Ago_Direct) == 2)
	{
		//		Right();	//9
		//		Right();
//		Printf_Debug("-R-",3);
//		 Num_Debug_Info(Run_D);
//		Printf_Debug("-A-",3);
//		Num_Debug_Info(Ago_Direct);
		Direct = 9;
		Ago_Direct = Run_D;
		return Direct;
	}
	else if ((Ago_Direct - Run_D) == 3 || (Run_D - Ago_Direct) == 1)
	{
		//Right();		//10
		Direct = 10;
		Ago_Direct = Run_D;
		return Direct;
	}
	else if (Run_D == Run_D)
	{
		Ago_Direct = Run_D;
		return 0;
	}
	else
	{
	}
	return 0;
}

void RouteToHex(uint8_t* RouteArray, char* RouteStr)//
{
	uint8_t i = 0, j = 0;
	uint8_t len = 0;
	len = strlen((char*)RouteStr);
	for (i = 0; i < len; i++)
	{
		if (RouteStr[i] == 'G')
		{
			RouteStr[i] = 6;
		}
		else
		{
			if (RouteStr[i] >= 65 && RouteStr[i] <= 90)	//大写字母
			{
				RouteStr[i] = RouteStr[i] - 0x37;
			}
			else if (RouteStr[i] >= 97 && RouteStr[i] <= 122)		//小写字母
			{
				RouteStr[i] = RouteStr[i] - 0x57;
			}
			else if (RouteStr[i] > 48 && RouteStr[i] < 57)		//数字
			{
				RouteStr[i] = RouteStr[i] - 0x30;
			}
		}
	}
	for (i = 0; i < len; i += 2)
	{
		RouteArray[j] = (uint8_t)((RouteStr[i] & 0x0F) << 4) | (RouteStr[i + 1] & 0x0F);
		j++;
	}
	RouteArray[j] = '\0';
	Printf_HEX_Info(RouteArray, len / 2);
}

void Straighten_Front(uint8_t toward_Front)
{
	if (toward_Front == CorrectX)
	{
		toward_Front = X;
	}
	else if (toward_Front == CorrectY)
	{
		toward_Front = Y;
	}
	else if (toward_Front == Correct_X)
	{
		toward_Front = _X;
	}
	else if (toward_Front == Correct_Y)
	{
		toward_Front = _Y;
	}
	switch (Judge_Direct(toward_Front))
	{
	case 8:
		//Left();	//8
		Car_RorL(1, AGV_wheel_sp, LM);	/*左转90*/
		break;
	case 9:
		Car_RorL(2, AGV_wheel_sp, RM);	/*右转90*/
		Car_RorL(2, AGV_wheel_sp, RM);	/*右转90*/
		break;
	case 10:
		//Right();		//0x0A
		Car_RorL(2, AGV_wheel_sp, RM);	/*右转90*/
		break;
	default:
		//Send_InfoData_To_Fifo("ERROE", 5);
		break;
	}
}