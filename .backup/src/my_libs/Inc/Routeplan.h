#ifndef _ROUTEPLAN_H
#define _ROUTEPLAN_H
#include <stdint.h>
#define X	1
#define Y	4
#define _X	3
#define _Y	2

extern uint8_t Ago_Direct;		//之前的方向
extern uint8_t Run_Direct;		//要动作的方向

extern void Calculate_Act(uint8_t ago, uint8_t after);     
extern uint8_t Judge_CrsrdOrBTTCrsrd(uint8_t JC_ago, uint8_t JC_aft);  //判断是否是十字路口或者是两个十字路口中间
extern uint8_t Judge_Direct(uint8_t Run_D);
extern void Act_Assign(uint8_t Act_flag);
extern void Correct_direction(uint8_t X_buf, uint8_t _X_buf, uint8_t Y_buf, uint8_t _Y_buf);//坐标
extern void Correct_Front_Orienta(uint8_t Correct_Front_buf); //朝向
extern void Route_Plan(uint8_t Initial_Coordinate, uint8_t* Route_Coordinate);
extern void Take_Action(uint8_t* Action_Array);
extern uint8_t Act_Array[30];
extern void Straighten_Front(uint8_t toward_Front);
extern void RouteToHex(uint8_t* RouteArray, char* RouteStr);

#endif // !_ROUTEPLAN_H
