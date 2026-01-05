#ifndef _MyLib_H
#define _MyLib_H

#define L 1
#define R 2
#define A 1
#define B 2
#define LM 691  //705
//1.720 2.714
#define RM 745//930  792 //左转90
#define LMB 350  //390
#define RMB 375  //375
extern uint16_t GBKNUM ;//编码转换储存的地方法
extern uint8_t SYN_ID;
extern uint8_t RFID_Position[6];//卡坐标点
extern uint8_t trm_buf_t[8];
extern uint8_t Beacon_Tower[6];//烽火台数据
extern uint8_t Locatio;
extern uint8_t Infrared[6];           // 红外发送数据缓存
extern uint8_t Text[9][3];
//随机路线的字母-数字-初始方向-中途的任务
extern char zmlx[20];
extern char szlx[20];
extern char fx ;
extern char rw[20][5];
extern char ali_cp[6];//车牌
//extern uint8_t ali_ltxs[12]; 
extern char ali_ltxs[12];
extern uint8_t ali_wxd[4];
extern uint8_t ali_Mainlx[20];
extern long ali_falg;
extern uint8_t ck_ZZ_A;
extern uint8_t ck_ZZ_B;
extern uint8_t zb[2];
extern uint8_t ali_fht[6];
extern uint8_t ali_sqk[6];
extern uint8_t ali_ltxs2[4];
extern uint8_t ali_ltxs3[4];
extern uint8_t yyzz_falg ;//小创语音控制的falg
extern uint8_t Start_Light_dear;//路灯的初始挡位
extern uint8_t initial;      //灯光的初始挡位(单用)
extern uint8_t SYN7318_ID_buf; //语音识别标志位
extern uint8_t GarageA_Floor;  //获取车库A的层数
extern uint8_t GarageB_Floor;  //获取车库B的层数
extern uint16_t dis;            //
extern uint8_t ali_ckcs;            //
extern uint16_t dis_size;//测距
extern uint8_t ali_my[6];
extern uint8_t ali_my1[6];
extern uint8_t ali_my2[6];
extern uint8_t ali_my3[6];
extern uint8_t ali_my4[6];
extern int ali_sum2;
extern long ali_sum3;

extern uint8_t csewm;//彩色二维码标志位
extern uint8_t ptewm;//普通二维码标志位
extern uint8_t bsyz ;
extern uint8_t sm ;
extern uint8_t jddz;//绝对地址
extern uint16_t ali_dis_size;
extern uint8_t text[10];
extern uint8_t temp_array[6];

extern uint8_t ali_fht_jyd;//烽火台救援点
extern uint8_t ali_tft[6];//TFT数据
extern uint8_t ali_ldC;//路灯初始档位
extern uint8_t ali_ldZ ;//路灯最终档位
extern uint8_t ali_yybb;//公交车站识别的车站
extern uint8_t ali_year ;//年
extern uint8_t ali_month ;//月
extern uint8_t ali_day;//日
extern uint8_t ali_hour;//时
extern uint8_t ali_minute;//分
extern uint8_t ali_second;//秒
extern uint8_t ali_dx;//地形位置
extern uint8_t ali_tq ;//天气
extern uint8_t ali_wd ;//温度
extern uint8_t ali_gjz;//公交车站
extern uint8_t falg;
extern uint8_t cs_ck ;
extern char  temp_cp[6];
extern char temp_st[3];
extern char ali_fx1;
extern char ali_fx2;
extern double ali_sum ;
extern unsigned long Break_time;

//extern uint8_t AGV_GO_Position;
extern uint8_t WaitTerrain_Flag;
void suiji_FZ(uint8_t* Alx, char Afx,uint8_t len);
void printArr(uint8_t* arr);
void printHexArr(uint8_t* arr);
void wxdDemo();
void TTS(uint8_t text);
void Printf_HEX_Info(uint8_t* p, uint8_t len);
void Send_ZigbeeData_To_Fifo(uint8_t* p, uint8_t len);
void Zigbee_Send_degree(uint8_t* buf, uint8_t number, uint16_t ms);
extern void delay_ms(uint16_t ms);
extern int getGBK(char* text);
//void Rotate_ZigBeeText_u8long(const uint8_t* Sec1);
/* 循迹板小车行进 */
void TGO(void);
void TGO_1(void);
void TGO_2(void);
void Car_Go(uint8_t speed, uint16_t temp);   						// 主车前进 参数：速度/码盘
void Car_Back(uint8_t speed, uint16_t temp); 						// 主车后退 参数：速度/码盘
void Car_BackBlackLine(uint8_t speed, uint16_t _distance);
void Car_Track(uint8_t speed);  								    // 主车循迹 参数：速度
void Car_TrackMP(uint8_t speed, uint16_t MP);                       // 主车循迹 参数：速度/码盘
void Car_RorL(uint8_t sp, uint8_t LorR, uint16_t angle);            // 小车转弯 参数：方向/速度/角度
void right(uint8_t model);
void left(uint8_t model);
void Back_ruku(uint8_t choice);
extern void openRandomPath(uint8_t* dw, char cfx, int len);


void setRW(uint8_t zb, char rwdh, char rwfx, char ifxh);
void Back_Garage(uint8_t choice);
void Back_Garage_2(uint8_t choice);
void Back_Garage_3(uint8_t choice);
void Back_Garage_4(uint8_t choice);//码盘入库

void Out_Garage(void);
void Out_Garage_2(uint8_t choice);//码盘出库
void Reverse_exit(void);//倒车出库

void Dis_QianJing(uint8_t i);

extern void CarBody_Amendment(int _mp);		/* 一般在2200码盘的地方可以加300，在1700码盘的地方输入0 */
extern void Amendment_Front();
extern void Amendment_Front_2();
void Amendment_Front_AXLT(void);
void Amendment_Front_AXLT_2(void);
extern void Car_Track_Terrain(uint8_t Spend, uint16_t temp_mp);
/* 地形 */
void Fixed_Terrain();
void Fixed_Terrain_2();
void Fixed();
void Fixed_Terrain_3();//码盘过地形
void Fixed_Terrain_4();//码盘过短地形
void Random_Terrain();
void Terrain_And_Card_Offset_easy();
void Terrain_And_Card_Offset_difficul();
void Terrain_And_Card_AXLT();
void TerrainBack_Zigbee(uint8_t number);
void Judge_terrain(); //过地形, 无论是有地形还是没有地形都照样过
void Judge_terrain_2();
void Judge_terrain_3();
void Judge_terrain_4();
/* 道闸 */
void Gate_Open_Zigbee(uint8_t number);						// 道闸闸门开启
void Gate_Show_Zigbee(char* Licence, uint8_t number);		// 道闸系统标志物显示车牌
void Get_state();
/* LED标志物 */
void LED_Data_Zigbee(uint8_t One, uint8_t Two, uint8_t Three, uint8_t rank);	// LED显示标志物显示数据
void LED_Data2_Zigbee(uint8_t* Data, uint8_t rank, uint8_t toHEX, uint8_t number);
void LED_Dis_Zigbee(uint16_t dis, uint8_t choice, uint8_t number);	            // LED显示标志物显示测距信息
void LED_time_Zigbee(uint8_t choice, uint8_t number);                       	//LED标志物显示时间信息
/* 无线充电 */
extern void Wireless_Charging(uint8_t number);
extern void Wireless_Charging_KEY(uint8_t* Charge_buf, uint8_t number);
extern void Wireless_Charging_KEY2(uint8_t buf_1, uint8_t buf_2, uint8_t buf_3, uint8_t buf_4, uint8_t number);
void Wireless_Charging_Close(uint8_t number);
/* 立体显示 */
void Rotate_show_Inf_dis_size(uint16_t num);
void Rotate_show_Inf(char* src, char x, char y, uint8_t number);		// 立体显示标志物显示车牌数据
void Rotate_show_Inf_2(uint8_t* src, char x, char y, uint8_t number);	//立体显示显示车牌
void Rotate_Dis_Inf(uint16_t dis, uint8_t number);    					// 立体显示标志物显示距离信息
void Rotate_roadCondition_Inf(uint8_t road_date, uint8_t number);		//立体显示车况信息
void Rotate_Sign_Inf(uint8_t sign_date, uint8_t number);				//立体显示交通标识
void Rotate_Colour(uint8_t data, uint8_t number);						//显示颜色
void Rotate_Shape(uint8_t shape, uint8_t number);						//立体显示形状
void Rotate_FontColor_Inf(uint8_t red_buf, uint8_t green_buf, uint8_t blue_buf, uint8_t number);//设置文本颜色
void Rotate_FontColor_Inf_2(uint8_t colour, uint8_t number);			//设置文本颜色
void Rotate_Text(uint8_t * Sec1, uint8_t number, uint8_t select);		//立体显示一个字符
void Rotate_Empty(uint8_t Sec1, uint8_t number);						//清空立体显示
void Rotate_Infrared_Text(uint8_t* Sec1, uint8_t number, uint8_t select);//红外发送一个自定义文本
void Rotate_show_Text2(char* text, uint8_t ifZigBee);
void dis_sizeShow(int value, char dw);//立体显示显示距离
extern void Rotate_show_Text(char* text, uint8_t ifZigBee);				//立体显示自定义文本
/* 路灯 */
uint8_t Light_Inf(uint8_t gear);
uint8_t Light_Inf_1(uint8_t gear);

/* 单点激活烽火台 */
void single_point(uint8_t* beacon_buf);
void single(uint8_t* beacon_buf, uint8_t* beacon);
void single_point_beacon(uint8_t* beacon_buf);
void single_point_beacon_2(uint8_t* beacon_buf);
void Car_RorL_Alarm(uint8_t* Alarm_buf, uint8_t choice);
void single_Location();
void Infrared_Send_degree(uint8_t* s, uint8_t number, uint16_t ms);
void Smoke_Towers(uint8_t* beacon_buf, uint8_t number);
/* TFT */

void TFT_Zigbee_A();//车牌A
void TFT_Zigbee_B();//车牌B
void TDT_HEX_A();
void TDT_HEX_B();
void LED_tow();

void TFT_Test_Zigbee(char Device, uint8_t Pri, uint8_t Sec1, uint8_t Sec2, uint8_t Sec3, uint8_t number);	// TFT显示标志物控制指令
void TFT_Dis_Zigbee(char Device, uint16_t dis, uint8_t number);		// 智能TFT显示器显示距离信息
void TFT_Disdate_Zigbee(char Device, uint8_t date_1, uint8_t date_2, uint8_t number);
void TFT_Show_Zigbee(char Device, char* Licence, uint8_t number);	// TFT显示器显示车牌
void TFT_Hex_Zigbee(char Device, char* Licence, uint8_t transition, uint8_t number);
void TFT_WhichPic_Zigbee(char Device, uint8_t which, uint8_t number);
void TFT_show_Hex2(char Device, uint8_t one, uint8_t two, uint8_t three, uint8_t number);
void TFT_Sign_Zigbee(char Device, uint8_t which, uint8_t number);

/* 立体车库 */
void Garage_GetFloor_Zigbee(char Device);
void Garage_Cont_Zigbee(char Device, uint8_t Floor);
void Garage_convenien_Zigbee(char Device, uint8_t Floor);
void Garage_AB_Zigbee(uint8_t A_Floor, uint8_t B_Floor);
void Garage_Test_Zigbee(char Device, uint8_t Pri, uint8_t Sec1);

/* 超声波 */
void Ultrasonic_Ranging();
void UltrasonicRanging_Model( uint8_t model, uint8_t choice, uint8_t wheel);
void UltrasonicRanging_Model_2(void); //后退到黑线再超声波测距

/* 蜂鸣器 */
extern void Cba_Beep(uint16_t HT, uint16_t LT, uint16_t number);
extern void Tba_BEEP(uint16_t HT, uint16_t LT, uint16_t number);
void delay_s(uint8_t s);

/* 语音 */
//void Voice_Broadcast_light(uint8_t Luden_Danwei, uint8_t maker);//语音播报路灯的挡位
//void Voice_broadcast_digital(uint16_t num);//语音播报数字
//void Voice_broadcast_digital_2(uint8_t num);  //语音播报标志物播报数字
//extern void Bus_tq(uint8_t cs);
//void Voice_Broadcast_Busstation(uint8_t num);  //TTS播报公交站
//void Voice_Broadcast_dis(uint16_t dis_buf, uint8_t model, uint8_t maker);
////void Voice_Broadcast_light(uint8_t light_Gears, uint8_t maker);
//void Voice_Broadcast_MainCar_YY(uint8_t YY_Nub, uint8_t maker);



void Uploading(uint8_t num);				//上传评分系统
void Bus_Dat_Weather_Data(uint8_t weather, uint8_t temp, uint8_t number);
void Bus_Dat_InquireTQ_Data(uint8_t number);
void YY_Play_Zigbee(uint8_t* s_data);
void suijiluxian();//随机路线！！！！
void CX_Bus_Time_Inquire_Data(uint8_t number);
//uint8_t YY_Speech_Recognition(uint8_t number);
uint8_t Start_ASR_WEN(void);
void YY_Comm_Zigbee(uint8_t Primary, uint8_t Secondary);
void Bus_Dat_Inquire_Data(uint8_t number);

void Bus_Dat_Control_Data(uint8_t year, uint8_t month, uint8_t day, uint8_t number);
void Bus_Time_Control_Data(uint8_t hour, uint8_t minute, uint8_t second, uint8_t number);

/*主从交互*/
void Main_SendData(uint8_t Pri, uint8_t Sec1, uint8_t Sec2, uint8_t Sec3, uint8_t degree);	// 给主车发送数据
void Main_SendLongData(uint8_t* text, uint8_t Pri, uint8_t len, uint8_t degree);
/*ETC*/
void ETCabc(uint8_t zss, uint8_t yss, uint8_t number);

/*双闪*/
void Double_Dodge();     //开
void Double_Flash_off(); //关
#if 0
extern
coordinates, towards, G_zb;
/* 初始坐标 车头朝向 要去的坐标 是否判断朝向 */
void Auto_kong(u8 c, u8 t, u8 g, u8 m);
/* 初始坐标 车头朝向 要去的地方的数组 HEX进制 */
void Auto_Array(u8 c, u8 t, u8* data);
/* 车头朝向 要去的地方的数组字符 */
uint8_t auto_act(uint8_t tw, uint8_t* way);
#endif
extern uint32_t gt_get_sub(uint32_t c);
extern uint32_t gt_get();
extern uint8_t H8_HEX(void);
extern uint8_t Q7_HEX(void);
extern void Send_InfoData_To_Fifo(uint8_t* Data_Array, uint8_t len);
extern void ZeroAry(uint8_t* date, uint8_t len);
extern void lx(int n, int szorzm);
#endif