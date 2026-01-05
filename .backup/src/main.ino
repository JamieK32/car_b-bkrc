#include "DCMotor.h"
#include "CoreLED.h"
#include "CoreKEY.h"
#include "CoreBeep.h"
#include "My_Lib.h"
#include "ExtSRAMInterface.h"
#include "LED.h"
#include "BH1750.h"
#include "Command.h"
#include "BEEP.h"
#include "Ultrasonic.h"
#include "Drive.h"
#include "infrare.h"
#include "OpenMV.h"
#include "my_libs/Inc/openmv2.h"
#include "Decode.h"
#include "Routeplan.h"
#include "string.h"
#include "math.h"
#include "BKRC_Voice.h"
#include "Chinese.h"


/****************************************************** 重要变量（最好别动）****************************************************************************/
#define TSendCycle  300
#define ATM_Data_Length 48
uint8_t ATM_Data[ATM_Data_Length];
uint8_t ZigBee_back[16] = { 0x55, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };//用于清空缓存数组
uint8_t ZigBee_command[8];//储存ZigBee接受的数组
uint8_t ZigBee_judge;//校验和
uint8_t Zigbee_repetition[8];//判断标志位
uint8_t sendflag;//接受标志位
uint8_t led_1;//LED

uint8_t mark = 0;//AGV全自动标志位
uint8_t mark_flag = 0;//AGV全自动标志位
uint8_t AGV = 0;
uint8_t ali_Iidex = 1;
uint8_t ali_Iidex2 = 0;
uint8_t ali_Index3 = 0;
uint8_t ali_falg2 = 0;
uint8_t Empty_Array[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };//清空缓冲区的
unsigned long frisrtime;//开机后的运行时间
unsigned long nowTime;//此刻时间
unsigned long Tcount;
unsigned long time_led;//记录LED4闪烁的时间
unsigned long Difference_time;
unsigned long Forcedentry_time = 0;//60000;//设置超时进入请设置这里       60000等于一分钟    
unsigned long tim_flag = 0;



void Analyze_Handle(uint8_t com);//ZigBee控制处理函数
void KEY_Handler(uint8_t k_value);//按键检测函数

static uint8_t asciiHexToValue(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	return 0;
}
void(*resetFunc) (void) = 0;
void setup()/*只执行一次， 适合初始化函数*/
{
	CoreLED.Initialization();   //核心板led灯初始化
	CoreKEY.Initialization();   //核心板按键初始化
	CoreBeep.Initialization();  //核心板蜂鸣器初始化
	ExtSRAMInterface.Initialization();  //底层通讯初始化
	LED.Initialization();  // 任务板LED初始化
	BH1750.Initialization();// 光强
	BEEP.Initialization();//  任务板蜂鸣器
	Infrare.Initialization();// 红外
	Ultrasonic.Initialization();//超声波
	DCMotor.Initialization();//电机
	BKRC_Voice.Initialization();//小创初始化
	Serial.begin(115200);//串口初始化
	while (!Serial);
	sendflag = 0;
	mark = 0;
	mark_flag = 0;
	ali_falg = Forcedentry_time;
	frisrtime = 0;
	Tcount = 0;
	tim_flag = millis();
	time_led = millis();
}

void loop()
{
	frisrtime = millis();//获取每次循环开始的时间//1秒 ＝ 1000毫秒//millis函数可以用来获取Arduino开机后运行的时间长度
	if ((millis() - time_led) >= 100)
	{
		led_1 = (~led_1) & 0x01;
		CoreLED.TurnOnOff(led_1);
		time_led = millis();
	}

	if (ExtSRAMInterface.ExMem_Read(0x6100) != 0x00)              //从车接收ZigBee数据
	{
		ExtSRAMInterface.ExMem_Read_Bytes(ZigBee_command, 8);      //读取八个字节数据

		ZigBee_judge = ZigBee_command[6];                          //获取校验和 主车发过来的校验和
		if ((ZigBee_judge == ZigBee_command[6]) && (ZigBee_command[0] == 0x55) && (ZigBee_command[7] == 0xBB))
		{
			if (Zigbee_repetition[2] != ZigBee_command[2])     //防重复标志位切记不可删除
			{
				Zigbee_repetition[2] = ZigBee_command[2];
				if (ZigBee_command[1] == 0x02)
				{
					Analyze_Handle(ZigBee_command[2]);       //主车命令相应函数
					//ExtSRAMInterface.ExMem_Write_Bytes(0x6100, Empty_Array, 8);     //绝对绝对不可以删掉
					//ExtSRAMInterface.ExMem_Write_Bytes(0x6100, Empty_Array, 8);     //绝对绝对不可以删掉
				}
			}
			else if (ZigBee_command[2] == 0xAA || ZigBee_command[2] == 0xEA|| ZigBee_command[2] == 0xEE) {
				Analyze_Handle(ZigBee_command[2]);       //主车命令相应函数
				//ExtSRAMInterface.ExMem_Write_Bytes(0x6100, Empty_Array, 8);     //绝对绝对不可以删掉
			}
			else {
				Tba_BEEP(50, 50, 1);
				Zigbee_repetition[2] = 0;
				ExtSRAMInterface.ExMem_Write_Bytes(0x6100, Empty_Array, 8);     //绝对绝对不可以删掉
				//ExtSRAMInterface.ExMem_Write_Bytes(0x6100, Empty_Array, 8);     //绝对绝对不可以删掉
			}
		}
	}

	if ((((millis() - Difference_time) >= Forcedentry_time) && Forcedentry_time != 0) || mark != 0)         //全自动触发点    || mark != 0
	{
		if (mark == 0) {
			mark_flag++;
			mark = mark_flag;
		}
		else {
			mark_flag = mark;
		}
		Serial2.write(0x30 + mark);
		AGV_Thread();
	}
	if (mark == 0 && mark_flag ==0) {
		control();
	}
	CoreKEY.Kwhile(KEY_Handler);                                  //按键检测
}


/******************************************************************************************************************************************************/
									  /** 控制指令处理函数 */
void Analyze_Handle(uint8_t com)
{
	switch (com)
	{
		/*********************************** 主车控制从车  *****************************************************/
	case 0x01://前进
		highSpeed(80);                             //高速循迹到路口
		break;
	case 0x02://左转
		left(2);
		break;
	case 0x03://右转
		right(2);
		break;
	case 0x04://后退
		Car_Go(50,1000);
		break;
	case 0x05://LED关闭计时
		Light_Inf_1(3);			
		break;
	case 0x06://TFTA关闭计时
		identifyTraffic(1);			                //交通灯
		servoControl(-55);                          //摄像头角度
		initPhotosensitive(1);                      //循迹初始化
		break;
	case 0x07://TFTA关闭计时
		identifyTraffic(2);			                //交通灯
		servoControl(-55);                          //摄像头角度
		initPhotosensitive(1);                      //循迹初始化
		break;
	case 0x08://TFTB关闭计时
		identifyTraffic(3);			                //交通灯
		servoControl(-55);                          //摄像头角度
		initPhotosensitive(1);                      //循迹初始化
		break;
	case 0x09://TFTB关闭计时
		left(1);
		break;
	case 0x0A://开启无线电
		right(1);

		break;
		/**************************************  从车接受数据  *************************************************/
	case 0xA0:  // 全自动任务调用，进入全自动函数  
		mark = ZigBee_command[3];
		Main_SendData(0xAA, 0x00, 0x00, 0x00, 5);
		Serial.print("markAGV:");
		Serial.println(mark);

		break;
	case 0xA1: //接受道闸车牌前三位（ZIgBee）
		ali_cp[0] = (char)ZigBee_command[3];
		ali_cp[1] = (char)ZigBee_command[4];
		ali_cp[2] = (char)ZigBee_command[5];
		Serial.print("ali_cp[0-2]:");
		Serial.print((char)ali_cp[0]);
		Serial.print(",");
		Serial.print((char)ali_cp[1]);
		Serial.print(",");
		Serial.println((char)ali_cp[2]);
		break;
	case 0xA2://接收道闸车牌后三位（ZIgBee）
		ali_cp[3] = (char)ZigBee_command[3];
		ali_cp[4] = (char)ZigBee_command[4];
		ali_cp[5] = (char)ZigBee_command[5];
		Serial.print("ali_cp[3-5]:");
		Serial.print((char)ali_cp[3]);
		Serial.print(",");
		Serial.print((char)ali_cp[4]);
		Serial.print(",");
		Serial.println((char)ali_cp[5]);

		break;
	case 0xA3://获取年月日
		ali_year = ZigBee_command[3];
		ali_month = ZigBee_command[4];
		ali_day = ZigBee_command[5];
		Serial.print("ali_year:");
		Serial.println(ali_year);
		Serial.print("ali_month:");
		Serial.println(ali_month);
		Serial.print("ali_day");
		Serial.println(ali_day);
		break;
	case 0xA4://获取时分秒
		ali_hour = ZigBee_command[3];
		ali_minute = ZigBee_command[4];
		ali_second = ZigBee_command[5];
		Serial.print("ali_hour:");
		Serial.println(ali_hour);
		Serial.print("ali_minute:");
		Serial.println(ali_minute);
		Serial.print("ali_second");
		Serial.println(ali_second);
		break;
	case 0xA5://接收车库最终层数

		ali_ckcs = ZigBee_command[3];
		Serial.print("ali_ckcs:");
		Serial.println(ali_ckcs);
		break;
	case 0xA6:
		qr_lr = ZigBee_command[3];
		Serial.print("qr_lr:");
		Serial.println(qr_lr);
		break;
	case 0xA7://地形状态
		if (ZigBee_command[3] == 0xAB) {
			YY_BZW(6, 1);
			ali_dx = 0xAB;
		}
		else if (ZigBee_command[3] == 0xBA) {
			YY_BZW(6, 2);
			ali_dx = 0xBA;
		}
		Serial.print("ali_dx:");
		Serial.println(ali_dx,HEX);
		break;
	case 0xA8://语音播报
		ali_yybb = ZigBee_command[3];
		Serial.print("ali_yybb1:");
		Serial.println(ali_yybb);
		Serial.print("ZigBee_command[3]:");
		Serial.println(ZigBee_command[3]);
		break;
	case 0xA9://烽火台激活码
		ali_fht[0] = ZigBee_command[3];
		ali_fht[1] = ZigBee_command[4];
		ali_fht[2] = ZigBee_command[5];
		Serial.print("ali_fht[0]-[2]:");
		Serial.print(ali_fht[0],HEX);
		Serial.print(",");
		Serial.print(ali_fht[1],HEX);
		Serial.print(",");
		Serial.println(ali_fht[2],HEX);
		break;
	case 0xAA:
		ali_fht[3] = ZigBee_command[3];
		ali_fht[4] = ZigBee_command[4];
		ali_fht[5] = ZigBee_command[5];
		Serial.print("ali_fht[3]-[5]:");
		Serial.print(ali_fht[3]);
		Serial.print(",");
		Serial.print(ali_fht[4]);
		Serial.print(",");
		Serial.println(ali_fht[5]);
		break;
	case 0xAB://TFT数据
		ali_tft[0] = ZigBee_command[3];
		ali_tft[1] = ZigBee_command[4];
		ali_tft[2] = ZigBee_command[5];
		Serial.print("ali_tft:");
		Serial.print(ali_tft[0]);
		Serial.print(",");
		Serial.print(ali_tft[1]);
		Serial.print(",");
		Serial.println(ali_tft[2]);
		break;
	case 0xAC:
		ali_tft[3] = ZigBee_command[3];
		ali_tft[4] = ZigBee_command[4];
		ali_tft[5] = ZigBee_command[5];
		Serial.print(ali_tft[3]);
		Serial.print(",");
		Serial.print(ali_tft[4]);
		Serial.print(",");
		Serial.println(ali_tft[5]);
		break;
	case 0xAD://获取路灯初始档位
		ali_ldC = ZigBee_command[3];
		Serial.print("ali_ldC:");
		Serial.println(ali_ldC);
		break;
	case 0xAE://获取路灯最终档位
		ali_ldZ = ZigBee_command[3];
		Serial.print("ali_ldZ:");
		Serial.println(ali_ldZ);
		break;
	case 0xAF://地形的位置
		ali_dx = ZigBee_command[3];
		Serial.print("ali_dx:");
		Serial.println(ali_dx);
		break;
	case 0xB0://交通灯颜色（A）
		trafficLightA_Color = ZigBee_command[3];
		Serial.print("trafficLightA_Color:");
		Serial.println(trafficLightA_Color);
		break;
	case 0xB1://交通灯颜色（B）
		trafficLightB_Color = ZigBee_command[3];
		Serial.print("trafficLightB_Color:");
		Serial.println(trafficLightB_Color);
		break;
	case 0xB2://交通灯颜色（C）
		trafficLightC_Color = ZigBee_command[3];
		Serial.print("trafficLightC_Color:");
		Serial.println(trafficLightC_Color);
		break;
	case 0xB3://立体显示数据
		ali_ltxs[0] = ZigBee_command[3];
		ali_ltxs[1] = ZigBee_command[4];
		ali_ltxs[2] = ZigBee_command[5];
		Serial.print("ali_ltxs[0]-[2]:");
		Serial.print((char)ali_ltxs[0]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[1]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[2]);
		break;
	case 0xB4:
		Serial.print("ali_ltxs[3]-[5]:");
		ali_ltxs[3] = ZigBee_command[3];
		ali_ltxs[4] = ZigBee_command[4];
		ali_ltxs[5] = ZigBee_command[5];
		Serial.print((char)ali_ltxs[3]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[4]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[5]);
		break;
	case 0xB5:
		Serial.print("ali_ltxs[6]-[8]:");
		ali_ltxs[6] = ZigBee_command[3];
		ali_ltxs[7] = ZigBee_command[4];
		ali_ltxs[8] = ZigBee_command[5];
		Serial.print((char)ali_ltxs[6]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[7]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[8]);
		break;
	case 0xB6:
		Serial.print("ali_ltxs[9]-[11]:");
		ali_ltxs[9] = ZigBee_command[3];
		ali_ltxs[10] = ZigBee_command[4];
		ali_ltxs[11] = ZigBee_command[5];
		Serial.print((char)ali_ltxs[9]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[10]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[11]);
		break;
	case 0xB7://无线充电激活码（ZIgBee）
		ali_wxd[0] = ZigBee_command[3];
		ali_wxd[1] = ZigBee_command[4];
		Serial.print("ali_wxd[0]-[1]:");
		Serial.print(ali_wxd[0]);
		Serial.print(",");
		Serial.println(ali_wxd[1]);
		break;
	case 0xB8:
		ali_wxd[2] = ZigBee_command[3];
		ali_wxd[3] = ZigBee_command[4];
		Serial.print("ali_wxd[2]-[3]:");
		Serial.print(ali_wxd[2]);
		Serial.print(",");
		Serial.println(ali_wxd[3]);
		break;
	case 0xB9://公交车站的天气温度
		ali_tq = ZigBee_command[3];
		ali_wd = ZigBee_command[4];
		Serial.print("tqwd:");
		Serial.println(ali_tq);
		Serial.println(ali_wd);
		break;
	case 0xBA://获取车库AB的层数
		GarageA_Floor = ZigBee_command[3];
		GarageB_Floor = ZigBee_command[4];
		Serial.print("GarageA_Floor:");
		Serial.print(GarageA_Floor);
		Serial.print("\tGarageB_Floor:");
		Serial.println(GarageB_Floor);
		break;
	case 0xBB://测距
		ali_dis_size = ZigBee_command[3];
		ali_dis_size = ali_dis_size * 10 + ZigBee_command[4];
		ali_dis_size = ali_dis_size * 10  + ZigBee_command[5];
		
		Serial.print("ali_dis_size:");
		Serial.println(ali_dis_size);
		break;
	case 0xBC://临时交互变量
		sm = ZigBee_command[3]-0x30;
		Serial.print("sm:");
		Serial.print(sm);
		
		break;
	case 0xBD://临时交互变量
		temp[6] = ZigBee_command[3];
		Serial.print("temp[6]:");
		Serial.println(temp[6],HEX);
		break;
	case 0xBE://初始车库坐标
		cs_ck= ZigBee_command[3];
		ali_fx1 = ZigBee_command[4];
		ali_fx2 = ZigBee_command[5];
		Serial.print("cs_ck:");
		Serial.println(cs_ck,HEX);
		Serial.print("ali_fx:");
		Serial.println(ali_fx1);
		Serial.print("ali_fx2:");
		Serial.println(ali_fx2);
		break;
	case 0xC0://随机路线点位
		lx2[0] = ZigBee_command[3];
		lx2[1] = ZigBee_command[4];
		lx2[2] = ZigBee_command[5];
		Serial.print("lx2:0-2:");
		Serial.print(lx2[0], HEX);
		Serial.print(lx2[1], HEX);
		Serial.println(lx2[2], HEX);
		break;
	case 0xC1://随机路线点位
		lx2[3] = ZigBee_command[3];
		lx2[4] = ZigBee_command[4];
		lx2[5] = ZigBee_command[5];
		Serial.print("lx2:3_5:");
		Serial.print(lx2[3], HEX);
		Serial.print(lx2[4], HEX);
		Serial.println(lx2[5], HEX);
		break;
	case 0xC2://随机路线点位
		lx2[6] = ZigBee_command[3];
		lx2[7] = ZigBee_command[4];
		lx2[8] = ZigBee_command[5];
		Serial.print("lx2:6_8:");
		Serial.print(lx2[6], HEX);
		Serial.print(lx2[7], HEX);
		Serial.println(lx2[8], HEX);
		break;
	case 0xC3://随机路线点位
		lx2[9] =  ZigBee_command[3];
		lx2[10] = ZigBee_command[4];
		lx2[11] = ZigBee_command[5];
		Serial.print("lx2:9-11:");
		Serial.print(lx2[9], HEX);
		Serial.print(lx2[10], HEX);
		Serial.println(lx2[11], HEX);
		break;
	
	
	

		/**********************************zigMan用于发送数据给主车*********************************/
	case 0xE9://主车关闭计时
		Main_SendData(0xE9, 0, 0, 0, 3);
		Serial.println("bye!");
		break;
	case 0xEE://主车获取从车任务状态
		Main_SendData(0xEE, 0, 0, 0, 3);
		Serial.println("Main_next!");
		break;
	case 0x70://从车识别的语音
		Main_SendData(0x70, ali_yybb-1, 0, 0, 3);
		Serial.print("Main_ali_yybb:");
		Serial.println(ali_yybb);
		break;
	case 0x71://从车测距
		Main_SendData(0x71, dis_size / 100, dis_size % 100, 0, 3);
		Serial.print("Main_dis_size:");
		Serial.println(dis_size);
		break;
	case 0x72://路灯档位   初始 最终 0x00

		Main_SendData(0x72, ali_ldC, ali_ldZ, 0, 3);
		Serial.print("Main_ali_ldC:");
		Serial.print(ali_ldC);
		Serial.print("\tMain_ali_ldZ:");
		Serial.println(ali_ldZ);
		break;
	case 0x73://从车识别的二维码信息
		Main_SendData(0x73, ali_ldC, ali_ldZ, 0, 3);
		break;
	case 0x74://发送主车车库AB层数   车库A    车库B    0x00
		Main_SendData(0x74, temp_st[0], temp_st[1], 0, 3);  //原来:GarageA_Floor,GarageB_Floor
		Serial.print("Main_GarageA_Floor:");
		Serial.print(GarageA_Floor);
		Serial.print("\tMain_GarageB_Floor:");
		Serial.println(GarageB_Floor);
		break;
	case 0x75://发送主车交通灯颜色   交通灯A    交通灯B  0x00
		Main_SendData(0x75, trafficLightA_Color, trafficLightB_Color, trafficLightC_Color, 3);
		Serial.print("Main_trafficLightA_Color:");
		Serial.print(trafficLightA_Color);
		Serial.print("\tMain_trafficLightB_Color:");
		Serial.println(trafficLightB_Color);
		break;
	case 0x76://获取地形状态
		Main_SendData(0x76, ali_dx, 0, 0, 3);
		Serial.print("Main_ali_dx:");
		Serial.println(ali_dx,HEX);
		break;
	case 0x77://路灯
		Main_SendData(0x77, ali_ldC, ali_ldZ, 0, 3);
		break;
	case 0x78://烽火台前三位
		Main_SendData(0x78, ali_fht[0], ali_fht[1], ali_fht[2], 3);
		Serial.print("Main_ali_fht[0]-[2]:");
		Serial.print((char)ali_fht[0]);
		Serial.print(",");
		Serial.print((char)ali_fht[1]);
		Serial.print(",");
		Serial.println((char)ali_fht[2]);
		break;
	case 0x79://烽火台后三位
		Main_SendData(0x79, ali_fht[3], ali_fht[4], ali_fht[5], 3);
		Serial.print("Main_ali_fht[3]-[5]:");
		Serial.print((char)ali_fht[3]);
		Serial.print(",");
		Serial.print((char)ali_fht[4]);
		Serial.print(",");
		Serial.println((char)ali_fht[5]);
		break;
	case 0x80://车牌前三位
	    ali_cp[0] = temp_array_1[6];
		ali_cp[1] = temp_array_1[7];
		ali_cp[2] = temp_array_1[8];
		Main_SendData(0x80, ali_cp[0], ali_cp[1], ali_cp[2], 3);
		Serial.print("Main_ali_cp[0-2]:");
		Serial.print((char)ali_cp[0]);
		Serial.print(",");
		Serial.print((char)ali_cp[1]);
		Serial.print(",");
		Serial.println((char)ali_cp[2]);
		break;
	case 0x81://车牌后三位
		Main_SendData(0x81, ali_cp[3], ali_cp[4], ali_cp[5], 3);
		Serial.print("Main_ali_cp[3-5]:");
		Serial.print((char)ali_cp[3]);
		Serial.print(",");
		Serial.print((char)ali_cp[4]);
		Serial.print(",");
		Serial.println((char)ali_cp[5]);
		break;
	case 0x82://无线电前两位
		Main_SendData(0x82, ali_wxd[0], ali_wxd[1], 0, 3);
		Serial.print("Main_ali_wxd[0-1]:");
		Serial.print(ali_wxd[0],HEX);
		Serial.print(",");
		Serial.println(ali_wxd[1],HEX);
		break;
	case 0x83://无线电后两位
		Main_SendData(0x83, ali_wxd[2], ali_wxd[3], 0, 3);
		Serial.print("Main_ali_wxd[2-3]:");
		Serial.print(ali_wxd[2],HEX);
		Serial.print(",");
		Serial.println(ali_wxd[3],HEX);
		break;
	case 0x84://卡一扇区块
		Main_SendData(0x84, qr_lr, k1[1], 0, 3);
		Serial.print("Main_k1[0-1]:");
		Serial.print(qr_lr);
		Serial.print(",");
		Serial.println(k1[1]);
		break;
	case 0x85://卡二扇区块
		Main_SendData(0x85, k2[0], k2[1], 0, 3);
		Serial.print("Main_k2[0-1]:");
		Serial.print(k2[0]);
		Serial.print(",");
		Serial.println(k2[1]);
		break;
	case 0x86://卡三扇区块
		Main_SendData(0x86, k3[0], k3[1], 0, 3);
		Serial.print("Main_k3[0-1]:");
		Serial.print(k3[0]);
		Serial.print(",");
		Serial.println(k3[1]);
		break;
	case 0x87://计算结果
		Main_SendData(0x87, ali_sum/100, ali_sum/10, ali_sum, 3);
		Serial.print("Main_ali_sum:");
		Serial.print(ali_sum/100);
		Serial.print(ali_sum/10);
		Serial.println(ali_sum);
		break;
	case 0x88://主车获取立体显示
		Main_SendData(0x88, ali_ltxs[0], ali_ltxs[1], ali_ltxs[2], 3);
		Serial.print("Main_ali_ltxs[0-2]:");
		Serial.print((char)ali_ltxs[0]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[1]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[2]);
		break;
	case 0x89://主车获取立体显示
		Main_SendData(0x89, ali_ltxs[3], ali_ltxs[4], ali_ltxs[5], 3);
		Serial.print("Main_ali_ltxs[3-5]:");
		Serial.print((char)ali_ltxs[3]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[4]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[5]);
		break;
	case 0x90://主车获取立体显示
		Main_SendData(0x90, ali_ltxs[6], ali_ltxs[7], ali_ltxs[8], 3);
		Serial.print("Main_ali_ltxs[6-8]:");
		Serial.print((char)ali_ltxs[6]);
		Serial.print(",");
		Serial.print((char)ali_ltxs[7]);
		Serial.print(",");
		Serial.println((char)ali_ltxs[8]);
		break;
	case 0x91:
		Main_SendData(0x91, temp_array_1[0], temp_array_1[1], temp_array_1[2], 3);
		Serial.print("temp_array_1[0-2]:");
		Serial.print((char)temp_array_1[0]);
		Serial.print(",");
		Serial.print((char)temp_array_1[1]);
		Serial.print(",");
		Serial.println((char)temp_array_1[2]);
		break;
	case 0x92://临时变量1
		Main_SendData(0x92, temp_array_1[3], temp_array_1[4], temp_array_1[5], 3);
		Serial.print("temp[3-5]:");
		Serial.print(temp_array_1[3]);
		Serial.print(",");
		Serial.println(temp_array_1[4]);
		Serial.print(",");
		Serial.println(temp_array_1[5]);
		break;
	case 0x93://临时变量1
		Main_SendData(0x93, temp_array_1[6], temp_array_1[7], temp_array_1[8], 3);
		Serial.print("temp_array_1[6-8]:");
		Serial.print(temp[6]);
		Serial.print(",");
		Serial.print(temp_array_1[7]);
		Serial.print(",");
		Serial.println(temp_array_1[8]);
		
		break;
	case 0x94://临时变量2
		Main_SendData(0x94, temp_array_1[9],ali_ckcs, temp_array_1[11], 3);
		Serial.print("temp_array_1[9-11]:");
		Serial.print(ali_year);
		Serial.print(",");
		Serial.print( ali_month);
		Serial.print(",");
		Serial.println(ali_day);
		
		break;
	case 0x95://临时变量2
		Main_SendData(0x95, ali_my3[0], ali_my3[1], ali_my3[2], 3);
		Serial.print("ali_my3[0]:");
		Serial.print(ali_my3[0],HEX);
		Serial.print(",");
		Serial.print(ali_my2[4], HEX);
		Serial.print(",");
		Serial.println(ali_my2[5], HEX);
		break;
	case 0x96://密钥
		Main_SendData(0x96, ali_my3[0], ali_my3[1], ali_my3[2], 3);
		Serial.print("ali_my3[0]:");
		Serial.print(ali_my3[0],HEX);
		/*Serial.print(",");
		Serial.print(ali_my3[1], HEX);
		Serial.print(",");
		Serial.println(ali_my3[2], HEX);*/
		break;
	case 0x97:
		Main_SendData(0x97, ali_my3[3], ali_my3[4], ali_my3[5], 3);
		Serial.print("ali_my3[3-5]:");
		Serial.print(ali_my3[3], HEX);
		Serial.print(",");
		Serial.print(ali_my3[4], HEX);
		Serial.print(",");
		Serial.println(ali_my3[5], HEX);
		break;
	case 0xD0://主车随机路线
		Main_SendData(0xD0, ali_my2[0], ali_my2[1], ali_my2[2], 3);
		Serial.print("ali_Mainlx[0-2]:");
		Serial.print( ali_my2[0], HEX);
		Serial.print(",");
		Serial.print(ali_my2[1], HEX);
		Serial.print(",");
		Serial.println(ali_my2[2], HEX);
		break;
	case 0xD1:
		Main_SendData(0xD1, ali_my2[3], ali_my2[4], ali_my2[5], 3); 
		Serial.print("ali_my2:");
		Serial.print(ali_my2[3], HEX);
		Serial.print(",");
		Serial.print(ali_my2[4], HEX);
		Serial.print(",");
		Serial.println(ali_my2[5], HEX);
		break;
	case 0xD2:
		Main_SendData(0xD2, ali_Mainlx[6], ali_Mainlx[7], ali_Mainlx[8], 3);
		Serial.print("ali_Mainlx[6-9]:");
		Serial.print(ali_Mainlx[6], HEX);
		Serial.print(",");
		Serial.print(ali_Mainlx[7], HEX);
		Serial.print(",");
		Serial.println(ali_Mainlx[8], HEX);
		break;
	case 0xD3:
		Main_SendData(0xD3, ali_Mainlx[9], ali_Mainlx[10], ali_Mainlx[11], 3);
		Serial.print("ali_Mainlx[10-11]:");
		Serial.print(ali_Mainlx[9], HEX);
		Serial.print(",");
		Serial.print(ali_Mainlx[10], HEX);
		Serial.print(",");
		Serial.println(ali_Mainlx[11], HEX);
		break;
	case 0xD4:
		Main_SendData(0xD4, ali_Mainlx[12], ali_Mainlx[13], ali_Mainlx[14], 3);
		Serial.print("ali_Mainlx[10-11]:");
		Serial.print(ali_Mainlx[12], HEX);
		Serial.print(",");
		Serial.print(ali_Mainlx[13], HEX);
		Serial.print(",");
		Serial.println(ali_Mainlx[14], HEX);
		break;
	case 0xD5://车库初始位置
		Main_SendData(0xD5, cs_ck,0 , 0, 3); 
		Serial.print("Main_cs_ck:");
		Serial.println(cs_ck, HEX);
		break;
	default:
		break;
	}
}
/******************************************** 按键检测函数 ********************************************************/
uint8_t ke = 0;
void KEY_Handler(uint8_t k_value)
{
	switch (k_value)
	{
		/*l
		Car_Go(82, 1170);                        //短道前进
		right(2);                                //右转
		Identify_QR_2(0);//识别彩色二维码
		left(2);                                 //左转
		Gate_Show_Zigbee(ali_cp, 2);            //车牌开启道闸
		highSpeed(82);                           //循迹段
		identifyTraffic(1);		                 //识别交通灯A
		Identify_QR_2(2);                     //识别二维码
		ali_back(1, 1);	//倒车入库：numOrletter数字库为1，isLandmark有标志物填1
		single_point_beacon(hef); 			  //单点烽火台激活
		Wireless_Charging_KEY(hex, 3);				/*开启码开启无线电
		Serial.println("KEY_Handler");
		servoControl(-55);                      //摄像头角度
		*/
	case 1: 
		// identifyTraffic(1);
		// Serial.println("trafficLightA_Color:");
	    // Serial.print(trafficLightA_Color);
		// Rotate_FontColor_Inf_2(3,3);
	     Identify_QR_2(3);
		//  for (int i = 0;i < 4;i++)
		//  {
		// 	Serial.println("ali_wxd:");
		// 	Serial.println(ali_wxd[i]);
		//  }
		//  for (int i = 0;i < 4;i++)
		//  {
		// 	Serial.println("ali_wxd2:");
		// 	Serial.println(ali_ltxs2[i]);
		//  }
		Wireless_Charging_KEY(ali_ltxs3,3); 			  //单点烽火台激活
		break;
	case 2:
		// Wireless_Charging_KEY(ali_ltxs3,3); 			  //单点烽火台激活
		// Garage_Cont_Zigbee('B', 1);				//立体车库到达指定车库(运行并等待)
		// Car_Go(85, 1130);                        //前进
		// right(2);                                //右转
		// Light_Inf_1(ali_ldZ);            		//自动调节光照强度函数
		// left(2);                                 //左转
  		// left(2);                                 //左转
		// highSpeed(82);                           //循迹段
		// Video_Line_4(65,1);                      //循迹段
		// rangingQR(1);//识别二维码
		// Car_RorL(2, 100, 780);		//右转:930 左转:837                 //右转
		// Gate_Show_Zigbee(ali_cp, 2);            //车牌开启道闸
		// highSpeed(82);                           //循迹段
		// highSpeed(82);                           //循迹段
		// right(2);                                //右转
		// identifyTraffic(1);		                 //识别交通灯A
		// highSpeed(82);                           //循迹段e
		// Car_RorL(2, 100, 780);		//右转:930 左转:837                                    //右转
		// rangingQR(2);                     //识别二维码
		// left(2);                                 //左转
		// identifyTraffic(2);		                 //识别交通灯B
		// highSpeed(82);                           //循迹段
		// right(1);                               //右转
		// single_point_beacon(ali_fht); 			  //单点烽火台激活
		// right(1);                                //右转
		// ali_back(1, 1);		//numOrletter:数字库为1，字母库随便  isLandmark：有标志物就填1
	    Car_Go(85, 1130);                        //前进
		rangingQR(3);                     //识别二维码
		Car_Back(85, 1130);
		// right(2);
		// Video_Line_4(65,1); 
		// left(2);
		// Gate_Show_Zigbee(ali_cp, 2);            //车牌开启道闸
		// highSpeed(82);
		// identifyTraffic(1);		                 //识别交通灯A
		// highSpeed(82);
		// right(1);
		// single_point_beacon(ali_fht);
		// left(1);
		// left(2);
		// identifyTraffic(2);
		// highSpeed(82);
		// left(2);
		// rangingQR(3);                     //识别二维码
		// ali_back(1, 1);
		// Garage_Cont_Zigbee('A',(temp[6] % 4) + 1);
		// Wireless_Charging_KEY(ali_wxd, 3);				/*开启码开启无线电*/
		// Wireless_Charging_KEY(ali_ltxs2, 3);				/*开启码开启无线电*/
		// Wireless_Charging_KEY(ali_ltxs3,3);
		break;
	case 3:
		switch (ke)
		{
		case 0:
		UltrasonicRanging_Model(3, 1, 1);
	     Serial.print(dis_size);
			break;
		}
		break;
	case 4:
	  servoControl(-10);                      //摄像头角度
		break;
	case 5:
	  servoControl(-70);                      //摄像头角度
		break;
	}

}

/**************************************** 部分常用参数 *****************************************************/
/*
车牌：ali_cp[6]						年：ali_year			月：ali_month				日：ali_day
最终车库层数：ali_ckcs				时：ali_hour			分：ali_minute				秒：ali_second
地形状态：ali_dx					语音播报：ali_yybb		烽火台激活码：ali_fht[6]	TFT数据：ali_tft[6]
路灯初始档位：ali_ldC				路灯最终档位：ali_ldZ	地形位置：ali_dx			主车识别到的公交车站：ali_gjz
交通灯(A)：trafficLightA_Color		交通灯(B)：trafficLightB_Color						立体显示数据：ali_ltxs[12]
无线电激活码：ali_wxd[4]			天气：ali_tq			温度：ali_wd				测距：dis_size
车库初始层数(A)：GarageA_Floor		车库初始层数(B)：GarageB_Floor
*/


void AGV_Thread()
{
	switch (mark)
	{
		/*l
		Garage_Cont_Zigbee('B', 1);	                //降车库(等待回传)
		videoLine(50);				                //循迹到路口
		 highSpeed(80);                             //高速循迹到路口
		servoControl(-55);                          //摄像头角度
		videoLineMP(80,500);			            //码盘循迹
		Serial.print("");                           //打印
		Identify_QR_2(2, 0, 2);                     //识别二维码 
		identifyTraffic(1);			                //交通灯
		servoControl(-55);                          //摄像头角度
		initPhotosensitive(1);                      //循迹初始化
		Gate_Show_Zigbee(ali_cp, 2); //车牌开启道闸
		ali_back(int numOrletter, int isLandmark);  //numOrletter:数字库为1，字母库随便  isLandmark：有标志物就填1
		Car_RorL(2, 100, RMB);		//右转:930 左转:837
		delay_ms(200);
		*/
	case 0x01://收到车牌和起点
        Car_Go(85, 1130);                        //前进
		rangingQR(3);                     //识别二维码
		mark = 0;
		break;
	case 0x02://给主车第一段路线后
	    right(2);
		Video_Line_4(65,1); 
		left(2);
		mark = 0;
		break;
	case 0x03:
	    Gate_Show_Zigbee(ali_cp, 2);            //车牌开启道闸
		highSpeed(82);
		identifyTraffic(1);		                 //识别交通灯A
		highSpeed(82);
		right(1);
		single_point_beacon(ali_fht);
		left(1);
		left(2);
		identifyTraffic(2);
		highSpeed(82);
		left(2);
		rangingQR(3);                     //识别二维码
		mark = 0;
		break;
	case 0x04:
		if (temp[6] == 0xD7)
		{
			ali_back(1, 1);
			Garage_Cont_Zigbee('A',(temp[6] % 4) + 1);
		    Wireless_Charging_KEY(ali_wxd, 3);				/*开启码开启无线电*/
		    Wireless_Charging_KEY(ali_ltxs2, 3);				/*开启码开启无线电*/
		    Wireless_Charging_KEY(ali_ltxs3,3);
		}
		else if(temp[6] == 0xB7)
		{
			left(2);
			highSpeed(82);
			right(2);
			ali_back(1, 1);
			Garage_Cont_Zigbee('A',(temp[6] % 4) + 1);
			Wireless_Charging_KEY(ali_wxd, 3);				/*开启码开启无线电*/
		    Wireless_Charging_KEY(ali_ltxs2, 3);				/*开启码开启无线电*/
		    Wireless_Charging_KEY(ali_ltxs3,3);
		}
		else
		{
			right(2);
			highSpeed(82);
			left(2);
			ali_back(1, 1);
			Garage_Cont_Zigbee('A',(temp[6] % 4)+ 1);
			Wireless_Charging_KEY(ali_wxd, 3);				/*开启码开启无线电*/
		    Wireless_Charging_KEY(ali_ltxs2, 3);				/*开启码开启无线电*/
		    Wireless_Charging_KEY(ali_ltxs3,3);
		}
		mark = 0;
		break;
	case 0x05:
		// Wireless_Charging_KEY(ali_wxd, 3);				/*开启码开启无线电*/
		Serial.print((temp[6]%4)+1,HEX);
		Garage_Cont_Zigbee('A',(temp[6] % 4) + 1);
		mark = 0;
		break;
	case 0x06:
		
		mark = 0;
		break;
	case 0x07:
		temp_array_3[0] = 'n';
		temp_array_3[1] = '+';
		temp_array_3[2] = 'x';
		temp_array_3[3] = '*';
		temp_array_3[4] = '1';
		temp_array_3[5] = '0';
		temp_array_3[6] = '/';
		temp_array_3[7] = 'y';
		for (int i = 0; i < strlen(temp_array_3); i++)
		{
			if (temp_array_3[i] == 'n') {
				temp_array_3[i] = 2 + 0x30;
			}
			else if (temp_array_3[i] == 'x') {
				temp_array_3[i] = ali_my3[5] + 0x30;
			}
			else if (temp_array_3[i] == 'y') {
				temp_array_3[i] = ali_my4[5] + 0x30;
			}
		}
		ali_sum = calculator(temp_array_3);
		temp_array_7[0] = (int)ali_sum / 100000;
		temp_array_7[1] = (int)ali_sum / 10000 % 10;
		temp_array_7[2] = (int)ali_sum / 1000 % 10;
		temp_array_7[3] = (int)ali_sum / 100 % 10;
		temp_array_7[4] = (int)ali_sum / 10 % 10;
		temp_array_7[5] = (int)ali_sum % 10;
		TFT_Hex_Zigbee('A', temp_array_7, 1, 3);

		mark = 0;
		break;
	}
	Difference_time = millis();
}
