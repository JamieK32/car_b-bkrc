/**
 * ============================================================================
 * K210 视觉系统通信模块 - 重构版
 * ============================================================================
 *
 * 功能模块：
 * 1. 协议通信：与 K210 端通信协议实现
 * 2. 舵机控制：摄像头角度控制
 * 3. 交通灯识别：识别交通灯颜色
 * 4. 二维码识别：识别并解析二维码数据
 * 5. 循迹功能：视频循迹相关功能
 *
 * 协议版本：匹配 K210 端重构后的协议
 * ============================================================================
 */

#include "ExtSRAMInterface.h"
#include "My_Lib.h"
#include "Command.h"
#include <string.h>
#include "Decode.h"
#include <math.h>
#include "DCMotor.h"
#include "openmv2.h"
#include "BKRC_Voice.h"

// ============================================================================
//                                  协议常量定义（匹配 K210 端）
// ============================================================================
#define PKT_HEAD 0x55 // 数据包起始字节
#define PKT_VER 0x02  // 协议版本号
#define PKT_TAIL 0xBB // 数据包结束字节

#define CMD_SERVO 0x91	 // 舵机/循迹控制命令
#define CMD_TRAFFIC 0x92 // 交通灯/二维码控制命令
#define CMD_TRACE 0x93	 // 循迹数据命令
#define CMD_VAR 0x95	 // 变量传输命令（二维码识别结果）

// 子命令定义
#define SUB_SERVO_SET 0x01		// 设置舵机角度
#define SUB_TRACE_START 0x06	// 启动循迹模式
#define SUB_TRACE_STOP 0x07		// 停止循迹模式
#define SUB_TRACE_DATA 0x01		// 循迹数据
#define SUB_TRAFFIC_START 0x03	// 启动交通灯识别
#define SUB_TRAFFIC_STOP 0x04	// 停止交通灯识别
#define SUB_TRAFFIC_RESULT 0x01 // 交通灯识别结果
#define SUB_QR_START 0x01		// 启动二维码识别

#define SERVO_POSITIVE_FLAG 0x2B // 舵机正角度标志

// 变量 ID 定义（匹配 K210 端 VarID，用于二维码识别结果分类）
#define VAR_WXD 0x10  // 无线电数据
#define VAR_FHT 0x11  // 烽火台数据
#define VAR_CKCS 0x12 // 车库层数
#define VAR_LDC 0x13  // 路灯数据
#define VAR_LDZ 0x14  // 路灯数据2（备用）
#define VAR_CP 0x15	  // 车牌数据
#define VAR_SJ 0x16   // 时间数据
#define VAR_WD 0x17   // 温度数据
#define VAR_LS1 0x18
#define VAR_XY 0X19
#define VAR_AF 0x20
#define VAR_MY 0x21
#define VAR_IQ 0x22
#define VAR_WXD2 0x23
#define VAR_WXD3 0x24
// ============================================================================
//                                  全局变量定义
// ============================================================================

// 二维码识别结果存储
uint8_t k1[2]; // 卡一扇区块（无线电数据前2字节）
uint8_t k2[2]; // 卡二扇区块（无线电数据后2字节）
uint8_t k3[2]; // 卡三扇区块（备用）

// 交通灯颜色存储
uint8_t trafficLightA_Color = 3; // 交通灯A颜色（1=红, 2=绿, 3=黄）
uint8_t trafficLightB_Color = 3; // 交通灯B颜色
uint8_t trafficLightC_Color = 3; // 交通灯C颜色
uint8_t trafficLightD_Color = 3; // 交通灯D颜色

// 其他数据存储
uint8_t GateStr_buf[12];	  // 车牌数据缓冲区
uint8_t valid_data[100] = {}; // 二维码数据缓冲区（烽火台等）
int garage = 0;				  // 车库层数
uint8_t light_goal = 0;		  // 光强度
uint8_t lx2[20] = {};		  // 路线数据
uint8_t qr_lr = 1;			  // 二维码左右标志
uint16_t QR_Num = 0;		  // 二维码编号
int flage = 0; //自定义MY标志位


////新增变量///
char list_cp[6];
// 其他模块使用的变量
int data1 = 0; // 摆正标志（用于 ali_back 等函数）

// 内部使用的缓冲区
static uint8_t Enmpty[20] = {}; // 清空数据缓冲区

// ============================================================================
//                                  底层通信函数
// ============================================================================

/**
 * 清空 ExtSRAM 数据缓冲区
 * @param directives 缓冲区地址（通常为 0x6038）
 */
void emptyData(uint16_t directives)
{
	while (ExtSRAMInterface.ExMem_Read(directives) != 0x00)
	{
		ExtSRAMInterface.ExMem_Read_Bytes(directives, Enmpty, 1);
	}
	delay(100);
	while (ExtSRAMInterface.ExMem_Read(directives) != 0x00)
	{
		ExtSRAMInterface.ExMem_Read_Bytes(directives, Enmpty, 1);
	}
	delay(10);
}

/**
 * 发送 CMD_SERVO 命令（舵机/循迹控制）
 * @param sub 子命令（SUB_SERVO_SET, SUB_TRACE_START, SUB_TRACE_STOP）
 * @param p1 参数1（舵机角度标志或循迹模式参数）
 * @param p2 参数2（舵机角度值或循迹模式参数）
 */
void openMv91(uint8_t sub, uint8_t p1, uint8_t p2)
{
	uint8_t cmd[8] = {PKT_HEAD, PKT_VER, CMD_SERVO, sub, p1, p2, 0x00, PKT_TAIL};
	Command.Judgment(cmd); // 计算校验和
	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, cmd, 8);
}

/**
 * 发送 CMD_TRAFFIC 命令（交通灯/二维码控制）
 * @param sub 子命令（SUB_TRAFFIC_START, SUB_TRAFFIC_STOP, SUB_QR_START）
 * @param p1 参数1（保留，通常为0）
 * @param p2 参数2（保留，通常为0）
 */
void openMv92(uint8_t sub, uint8_t p1, uint8_t p2)
{
	uint8_t cmd[8] = {PKT_HEAD, PKT_VER, CMD_TRAFFIC, sub, p1, p2, 0x00, PKT_TAIL};
	Command.Judgment(cmd); // 计算校验和
	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, cmd, 8);
}

// ============================================================================
//                                  舵机控制
// ============================================================================

/**
 * 舵机角度控制
 * @param angle 角度值，范围 -80° ~ +40°，0° 垂直于车身
 *              典型值：交通灯=5°, 二维码=-10°, 循迹=-55°
 */
void servoControl(int8_t angle)
{
	uint8_t flag = (angle >= 0) ? SERVO_POSITIVE_FLAG : 0x2D; // 正数用 0x2B，负数用 0x2D
	uint8_t absAngle = abs(angle);
	openMv91(SUB_SERVO_SET, flag, absAngle);
	delay(800); // 等待舵机到位
}

// ============================================================================
//                                  交通灯识别
// ============================================================================

/**
 * 发送交通灯识别结果到 Zigbee（使用查表法）
 * @param choice 交通灯编号（1=A, 2=B, 3=C, 4=D）
 * @param colorCode 颜色代码（1=红, 2=绿, 3=黄, 其他=黄(默认)）
 */
static void sendTrafficResult(uint8_t choice, uint8_t colorCode)
{
	// 命令查找表：[交通灯编号][颜色] -> 对应 Zigbee 命令
	// 颜色索引: 0=默认(黄), 1=红, 2=绿, 3=黄
	uint8_t *cmdTable[4][4] = {
		{Command.TrafficA_Yellow, Command.TrafficA_Red, Command.TrafficA_Green, Command.TrafficA_Yellow},
		{Command.TrafficB_Yellow, Command.TrafficB_Red, Command.TrafficB_Green, Command.TrafficB_Yellow},
		{Command.TrafficC_Yellow, Command.TrafficC_Red, Command.TrafficC_Green, Command.TrafficC_Yellow},
		{Command.TrafficD_Yellow, Command.TrafficD_Red, Command.TrafficD_Green, Command.TrafficD_Yellow}};

	// 参数范围检查
	uint8_t idx = (choice >= 1 && choice <= 4) ? (choice - 1) : 0;
	uint8_t colorIdx = (colorCode >= 1 && colorCode <= 3) ? colorCode : 0;
	Zigbee_Send_degree(cmdTable[idx][colorIdx], 3, 300);
}

/**
 * 获取交通灯开启命令
 * @param choice 交通灯编号（1=A, 2=B, 3=C, 4=D）
 * @return 对应的 Zigbee 开启命令指针
 */
static uint8_t *getTrafficOpenCmd(uint8_t choice)
{
	uint8_t *openCmds[4] = {
		Command.TrafficA_Open,
		Command.TrafficB_Open,
		Command.TrafficC_Open,
		Command.TrafficD_Open};
	uint8_t idx = (choice >= 1 && choice <= 4) ? (choice - 1) : 0;
	return openCmds[idx];
}

/**
 * 保存交通灯颜色到对应全局变量
 * @param choice 交通灯编号（1=A, 2=B, 3=C, 4=D）
 * @param colorCode 颜色代码（1=红, 2=绿, 3=黄）
 */
static void saveTrafficColor(uint8_t choice, uint8_t colorCode)
{
	switch (choice)
	{
	case 1:
		trafficLightA_Color = colorCode;
	   Serial.print("trafficLightA_Color:");
	    Serial.println(trafficLightA_Color);
		break;
	case 2:
		trafficLightB_Color = colorCode;
		break;
	case 3:
		trafficLightC_Color = colorCode;
		break;
	default:
		trafficLightD_Color = colorCode;
		break;
	}
}

/**
 * 识别交通灯颜色
 * @param choice 交通灯编号（1=A, 2=B, 3=C, 4=D）
 * @return 识别到的颜色（1=红, 2=绿, 3=黄, 0=超时/未识别）
 *
 * 工作流程：
 * 1. 调整舵机到识别角度（5°）
 * 2. 通知 Zigbee 开启对应交通灯
 * 3. 启动 K210 识别
 * 4. 等待识别结果（超时6秒）
 * 5. 保存并发送结果
 */
uint8_t identifyTraffic(uint8_t choice)
{
	uint8_t result = 0;
	uint8_t rxData[8] = {};

	// 1. 调整舵机到识别角度
	servoControl(5);

	// 2. 通知 Zigbee 开启对应交通灯
	Zigbee_Send_degree(getTrafficOpenCmd(choice), 3, 300);

	// 3. 清空缓冲区并启动 K210 识别
	emptyData(SIXZEROTHREEEIGHT);
	openMv92(SUB_TRAFFIC_START, 0, 0);
	delay(200);

	// 4. 等待识别结果（带超时）
	unsigned long startTime = millis();
	const unsigned long TIMEOUT_MS = 6000; // 6秒超时

	while (millis() - startTime < TIMEOUT_MS)
	{
		// 检查是否有数据
		if (ExtSRAMInterface.ExMem_Read(0x6038) == 0x00)
			continue;

		// 读取数据
		ExtSRAMInterface.ExMem_Read_Bytes(0x6038, rxData, 8);

		// 验证数据包格式: 0x55 0x02 0x92 0x01 [color] ... 0xBB
		if (rxData[0] != PKT_HEAD || rxData[1] != PKT_VER ||
			rxData[7] != PKT_TAIL || rxData[2] != CMD_TRAFFIC || rxData[3] != SUB_TRAFFIC_RESULT)
			continue;

		// 获取颜色结果
		result = rxData[4]; // 1=红, 2=绿, 3=黄

		// 打印结果
		Serial.print("Traffic ");
		Serial.print((char)('A' + choice - 1));
		Serial.print(": ");
		Serial.println(result == 1 ? "Red" : (result == 2 ? "Green" : "Yellow"));
		Serial2.write(0x30 + result);

		// 保存并发送结果
		saveTrafficColor(choice, result);
		sendTrafficResult(choice, result);
		break;
	}

	// 5. 超时处理：默认发送黄色
	if (result == 0)
	{
		Serial.println("Traffic: Timeout -> Yellow");
		result = 3;
		saveTrafficColor(choice, 3);   // ★★★关键
		sendTrafficResult(choice, 3);
	}


	// 6. 停止识别并清理
	openMv92(SUB_TRAFFIC_STOP, 0, 0);
	delay(20);
	emptyData(SIXZEROTHREEEIGHT);

	return result;
}
// ============================================================================
//                                  二维码识别 (已修改：纯字符串打印)
// ============================================================================

/**
 * 接收并处理变量数据包（二维码识别结果）
 * @param var_id 变量ID（VAR_WXD, VAR_FHT, VAR_CP 等）
 * @param data 数据缓冲区
 * @param data_len 数据长度
 * @return 是否成功处理
 */
static bool processVarData(uint8_t var_id, const uint8_t *data, uint8_t data_len)
{
    switch (var_id)
    {
		for (int i = 0;i < 4;i++)
		{
			Serial.print("data[");
			Serial.print(i);
			Serial.print("]:");
			Serial.print(data[i], HEX);
			Serial.println(" ");
		}

	case VAR_MY:
	if (data_len == 6)
	{
		Serial.println("--------------------- VAR_MY 开始 --------------------");
		if (flage == 0)
		{
         for (int i = 0;i < 6;i++)
		{
			ali_my3[i] = data[i];
		}
		Serial.print("ali_my3:");
		for (int i = 0;i < 6;i++)
		{
			Serial.println(ali_my3[i],HEX);
		}
		flage=flage+1;
		}
	    if (flage==1)
		{
		    for (int i = 0;i < 6;i++)
		{
			ali_my2[i] = data[i];
		}
		   Serial.print("ali_my2:");
		   for (int i = 0;i < 6;i++)
		{
			Serial.println(ali_my2[i],HEX);
		}
		}
		get_middle_six(ali_my3, ali_my2, ali_fht);
		Serial.println("ali_fht:");
		for (uint8_t i = 0;i < 6;i++)
		{
			Serial.println(ali_fht[i],HEX);
		}
		Serial.println("--------------------- VAR_MY 结束 --------------------");
	}
    case VAR_WXD:    // WXD:0x10
        // K210发送的是ASCII字符串，直接按字符打印
        if (data_len == 4 && var_id == VAR_WXD) 
        {
			Serial.println("--------------------- VAR_WXD 开始 --------------------");
			for (int i = 0; i < 4; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i],HEX);
				Serial.println(" ");
			}
            // 注意：请确保 ali_fht 或对应接收buffer 足够大
            // memcpy(ali_cp, data, data_len); //把内存里的“原始字节”直接复制一份,memcpy(目标地址, 源地址, 拷贝字节数);
			Serial.println("ali_wxd:");
			for (uint8_t i = 0;i < 4;i++)
			{
				ali_wxd[i] = data[i];
				Serial.println(ali_wxd[i]); // 【修改】使用 write 直接打印字符
			}
			Serial.println("--------------------- VAR_WXD 结束 --------------------");
            Serial.println();
            return true;
        }
        return false;
	case VAR_WXD2:    // WXD:0x23
        // K210发送的是ASCII字符串，直接按字符打印
        if (data_len == 4) 
        {
			Serial.println("--------------------- VAR_WXD2 开始 --------------------");
			for (int i = 0; i < 4; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i]);
				Serial.println(" ");
			}
            // 注意：请确保 ali_fht 或对应接收buffer 足够大
            // memcpy(ali_cp, data, data_len); //把内存里的“原始字节”直接复制一份,memcpy(目标地址, 源地址, 拷贝字节数);
			Serial.println("ali_ltxs3:");
			for (uint8_t i = 0;i < 4;i++)
			{
				ali_ltxs3[i] = data[i];
				Serial.println(ali_ltxs3[i] - '0'); // 【修改】使用 write 直接打印字符
			}
			Serial.println("--------------------- VAR_WXD2 结束 --------------------");
            Serial.println();
            return true;
        }
        return false;
	case VAR_WXD3:    // WXD:0x24
        // K210发送的是ASCII字符串，直接按字符打印
        if (data_len == 4) 
        {
			Serial.println("--------------------- VAR_WXD3 开始 --------------------");
			for (int i = 0; i < 4; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
            // 注意：请确保 ali_fht 或对应接收buffer 足够大
            // memcpy(ali_cp, data, data_len); //把内存里的“原始字节”直接复制一份,memcpy(目标地址, 源地址, 拷贝字节数);
			Serial.println("VAR_WXD3:");
			for (uint8_t i = 0;i < 4;i++)
			{
				ali_ltxs2[i] = data[i];

				Serial.println(ali_ltxs2[i],HEX); // 【修改】使用 write 直接打印字符
			}
			Serial.println("--------------------- VAR_WXD3 结束 --------------------");
            Serial.println();
            return true;
        }
        return false;
    case VAR_FHT:    // FHT:0x11
        if (data_len == 6)
        {
			Serial.println("--------------------- VAR_FHT 开始 --------------------");
			for (int i = 0; i < 6; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
            Serial.print("FHT: ");
            for (uint8_t i = 0; i < 6; i++)
            {   
				ali_fht[i] = data[i];
                Serial.println(ali_fht[i],HEX); // 【修改】使用 write 直接打印字符
            }
			Serial.println("--------------------- VAR_FHT 结束 --------------------");
            Serial.println();
            return true;
        }
        return false;
	case VAR_AF:	
	    if (data_len == 6)
		{
			Serial.println("--------------------- VAR_AF 开始 --------------------");
		   	for (int i = 0; i < 6; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
			Serial.print("AF: ");
		   for (int i = 0; i < 6; i++)
			{
				temp_array_1[i + 3] = data[i] - '0';
			}
			Serial.println("temp_array_1[i]:");
			for (int i = 0;i < 6;i++)
			{
				Serial.print((char)temp_array_1[i + 3]);
			}
			Serial.println("--------------------- VAR_AF 结束 --------------------");
            return true;
		}
		return false;
    case VAR_CP:     // CP:0x15
        if (data_len == 6)
        {
		   Serial.println("--------------------- VAR_CP 开始 --------------------");
		   	for (int i = 0; i < 6; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
           Serial.print("CP: ");
		   for (int i = 0; i < 6; i++)
			{
				ali_cp[i] = data[i];
			}
            for (uint8_t i = 0; i < 6; i++)
            {
                Serial.write(ali_cp[i]); // 【修改】使用 write 直接打印字符
            }
			Serial.println("--------------------- VAR_CP 结束 --------------------");
            return true;
        }
        return false;
    case VAR_CKCS:   // 1 字节 (K210发送的是数值 0x01, 0x02等，不是字符 '1')
        if (data_len == 1)
        {
            ali_ckcs = data[0];
            Serial.print("CKCS: ");
            Serial.println(ali_ckcs); // 打印数值即可
            return true;
        }
        return false;
	case VAR_XY:
		for (int i = 0; i < 2; i++)
			{
				Serial.print("data1[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
	    if (data_len == 2)
		{
			Serial.println("--------------------- VAR_XY结束 --------------------");
			for (int i = 0;i < 8;i++)
			{
				ali_Mainlx[i] = data[i];
			}
			temp_st[0] = ali_Mainlx[0];
			temp_st[1] = ali_Mainlx[1];
			temp_st[2] = '\0';
			sscanf(temp_st, "%2hhX", &ali_ckcs);
			Serial.println("XY拼接后数据:");
			Serial.print(ali_ckcs,HEX);
			Serial.println("XY未拼接数据:");
			Serial.print(temp_st[0]);
			Serial.print(temp_st[1]);
			if (ali_Mainlx[0] == 'B')
			{
				ali_fx2 = '1';
			}
			else if (ali_Mainlx[0] == 'D')
			{
				ali_fx2 = '2';
			}
			else
			{
				ali_fx2 = '3';
			}
			return true;
		}
		return false;
    case VAR_LDC:    // 1~2 字节 (数值)
        if (data_len >= 1)
        {
            ali_ldC = data[0];
            light_goal = data[0];
            Serial.print("LDC: ");
            Serial.println(ali_ldC); // 打印数值即可
            return true;
        }
        return false;
	case VAR_LS1:
	    if (data_len == 1,isdigit(data[0]))
		{
			Serial.println("--------------------- VAR_LS1 开始 --------------------");
			for (int i = 0; i < 1; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
			Serial.println("ali_LS1:");
			for (uint8_t i = 0;i < 1;i++)
			{
				ali_wd = data[i];
			}
			return true;
			Serial.print("ali_wd:");
			Serial.print(ali_wd);
			Serial.println("--------------------- VAR_LS1 结束 --------------------");
		}
		return false;
    case VAR_LDZ:    // (数值)
        if (data_len >= 1)
        {
			Serial.println("--------------------- VAR_LDZ 开始 --------------------");
			for (int i = 0; i < 2; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
            ali_ldZ = data[0];
            Serial.print("LDZ: ");
            Serial.println(ali_ldZ); // 打印数值即可
			Serial.println("--------------------- VAR_LDZ 结束 --------------------");
            return true;
        }
        return false;
	case VAR_SJ:    // 时间数据 (数值)
		if (data_len == 10)
		{
			Serial.println("--------------------- VAR_SJ 开始 --------------------");
			for (int i = 0; i < 10; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
			  Serial.print("SJ: ");
			for (int i = 0; i < 10; i++)
			{
				
				temp_array_1[i] = data[i];
				//Serial.print(temp_array_1[i]);
			}
			char hex_y[3] = {data[2], data[3], '\0'}; // 得到字符串 "20"
            sscanf(hex_y, "%2hhx", &ali_year);
			ali_month = (temp_array_1[5] - '0') * 10 + (temp_array_1[6] - '0');
			ali_day = (temp_array_1[8] - '0') * 10 + (temp_array_1[9] - '0');
			Serial.println(ali_day,HEX);
			Serial.println(ali_month,HEX);
			Serial.println(ali_year);
			Serial.println(" ");
			Serial.println("--------------------- VAR_SJ 结束 --------------------");
			return true;
		}
		return false;
	case VAR_IQ:
	    if (data_len == 1)
		{
			Serial.println("--------------------- VAR_LD 开始 --------------------");
		    for (int i = 0; i < 3; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
			ali_ldZ = data[0];
			Serial.print(ali_ldZ);
			return true;
		}
		return false;
	case VAR_WD:    // 温度数据 (数值)
		if (data_len == 2)
		{
			Serial.println("--------------------- VAR_WD 开始 --------------------");
		    for (int i = 0; i < 3; i++)
			{
				Serial.print("data[");
				Serial.print(i);
				Serial.print("]:");
				Serial.print(data[i], HEX);
				Serial.println(" ");
			}
			char hex_wd[3] = {data[0], data[1], '\0'}; // 得到字符串 "20"
            sscanf(hex_wd, "%2hhx", &ali_wd);
		    ali_wd = (data[0]-'0') * 10 + (data[1] - '0');
			Serial.print("WD:");
			Serial.println(ali_wd);
			Serial.println("--------------------- VAR_WD 结束 --------------------");
			return true;
		}
		return false;
    default:
        return false;
    }
}

/**
 * 验证变量数据包格式（二维码识别结果）
 * @param data 数据包缓冲区
 * @param len 数据包长度
 * @return 是否为有效的数据包
 */
static bool isValidVarPacket(const uint8_t *data, uint8_t len)
{
    if (len < 5)
        return false;
    return (data[0] == PKT_HEAD && data[1] == len - 2 &&
            data[2] == CMD_VAR && data[len - 1] == PKT_TAIL);
}

// 建议放在文件前面某处，通用的十六进制打印函数
void printHexBuf(const char *tag, const uint8_t *buf, uint8_t len)
{
    if (tag)
    {
        Serial.print(tag);
    }
    for (uint8_t i = 0; i < len; i++)
    {
        if (buf[i] < 0x10) Serial.print('0'); // 对齐
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

/**
 * 二维码识别函数
 */
void Identify_QR_2(uint8_t sheet)
{
    // 1. 调整舵机并清空缓冲区
    servoControl(-10);
    delay(200);
    
    // 清空固定指令帧
    emptyData(SIXZEROTHREEEIGHT); 
    // 【修改】确保开始前变量缓冲区也是干净的
    ExtSRAMInterface.ExMem_Write(0x6039, 0x00); 

    // 2. 发送开始扫描指令
    openMv92(SUB_QR_START, sheet, 0);
    delay(500);

    unsigned long startTime = millis();
    const unsigned long SCAN_TIMEOUT_MS = 22000;
    const unsigned long DRAIN_TIMEOUT_MS = 1200;
    unsigned long lastRxTime = startTime;
    bool stopSeen = false;
    bool timeoutReached = false;
    uint8_t rxBuf[120] = {0};
    uint8_t stopCmdBuf[8] = {0};
    Serial.println(">> STM32: 开始接收二维码数据...");

    while (true)
    {
        uint8_t pktLen = ExtSRAMInterface.ExMem_Read(0x6039);
        if (pktLen > 0 && pktLen <= 100)
        {
            uint8_t totalLen = pktLen + 3; // 估算读取长度
            if (totalLen > 119) totalLen = 119;
            ExtSRAMInterface.ExMem_Read_Bytes(0x6039, rxBuf, totalLen);
		    ExtSRAMInterface.ExMem_Write(0x6039, 0x00);
            lastRxTime = millis();
            uint8_t cmd = rxBuf[1];

            if (cmd == CMD_VAR)
            {
                uint8_t var_id   = rxBuf[2];
                uint8_t data_len = pktLen - 2;
                uint8_t* data    = &rxBuf[3];
                bool ret = processVarData(var_id, data, data_len);
                if (ret){
                      Serial.print(">> 收到有效变量: 0x");
                      Serial.println(var_id, HEX);
					  Serial.println( );
                }
            }
        }
        if (ExtSRAMInterface.ExMem_Read(0x6038) == PKT_HEAD)
        {
            ExtSRAMInterface.ExMem_Read_Bytes(0x6038, stopCmdBuf, 8);

            if (stopCmdBuf[0] == PKT_HEAD &&
                stopCmdBuf[1] == PKT_VER &&
                stopCmdBuf[2] == CMD_TRAFFIC &&
                stopCmdBuf[3] == SUB_TRAFFIC_STOP &&
                stopCmdBuf[7] == PKT_TAIL)
            {
                Serial.println(">> STM32: 收到 K210 退出指令，结束扫码。");
                emptyData(SIXZEROTHREEEIGHT);
                stopSeen = true;
                lastRxTime = millis();
            }
        }

        unsigned long now = millis();
        if (!timeoutReached && (now - startTime >= SCAN_TIMEOUT_MS))
        {
            timeoutReached = true;
            Serial.println(">> STM32: QR scan timeout, request stop.");
            openMv92(SUB_TRAFFIC_STOP, 0, 0);
        }
        if ((stopSeen || timeoutReached) && (now - lastRxTime >= DRAIN_TIMEOUT_MS))
        {
            break;
        }
        delay(10);
    }
    Serial.println("二维码任务结束");
    openMv92(SUB_TRAFFIC_STOP, 0, 0); 
}

/**
 * 彩色二维码识别函数（兼容旧接口）
 * @param sheet 需要识别的二维码数量
 * @param size 已废弃，保留兼容性
 * @param around 已废弃，保留兼容性
 */
void Identify_colourQR_2(uint8_t sheet, uint8_t size, uint8_t around)
{
	// 使用普通二维码识别函数
	Identify_QR_2(sheet);
}

/**
 * 初始化摄像头（已废弃，保留兼容性）
 * @param choose 1=循迹初始化, 2=二维码初始化
 *
 * 注意：新协议中不再需要单独的初始化命令，功能已集成到各模式启动命令中
 */
void initPhotosensitive(uint8_t choose)
{
	// 新协议中，传感器初始化已集成到各模式启动命令中
	delay(200);
}


//测距二维码
void rangingQR(uint8_t sheet) {
	UltrasonicRanging_Model(3, 1, 1);
	int num = 340;
	if (dis_size >= num) {//一厘米等于20MP  //400   
		Car_Go(60, (dis_size - num) * 3);
		Identify_QR_2(sheet);
		Car_Back(60, (dis_size - num)* 3);
	}
	else {
		Car_Back(60, (num - dis_size)* 3);
		Identify_QR_2(sheet);
		Car_Go(60, (num - dis_size) * 3);
	}
}

// ============================================================================
//                                  循迹功能
// ============================================================================

/**
 * 启动循迹模式
 */
void OpenMVTrack_Disc_StartUp(void)
{
	Servo_Control(-60);
	openMv91(SUB_TRACE_START, 0, 0);
}

/**
 * 关闭循迹模式
 */
void OpenMVTrack_Disc_CloseUp(void)
{
	openMv91(SUB_TRACE_STOP, 0, 0);
}

/**
 * 验证循迹数据包格式
 * @param data 数据包缓冲区
 * @return 是否为有效的循迹数据包
 */
static bool isValidTracePacket(const uint8_t *data)
{
	return (data[0] == PKT_HEAD && data[1] == PKT_VER &&
			data[2] == CMD_TRACE && data[3] == SUB_TRACE_DATA);
}

/**
 * 通用循迹初始化函数
 * @param mode 模式（0=黑线, 1=白线）
 * @param param1 参数1（白线模式参数）
 * @param param2 参数2（白线模式参数）
 */
static void initLineTracking(uint8_t mode, uint8_t param1, uint8_t param2)
{
	DCMotor.StartUp();
	DCMotor.Roadway_mp_syn();
	emptyData(SIXZEROTHREEEIGHT);
	if (mode == 0)
	{
		openMv91(SUB_TRACE_START, 0, 0);
	}
	else
	{
		openMv91(SUB_TRACE_START, param1, param2);
	}
}

/**
 * 简化初始化（仅同步码盘，不启动电机）
 */
static void initLineTrackingSync()
{
	DCMotor.Roadway_mp_syn();
	emptyData(SIXZEROTHREEEIGHT);
	openMv91(SUB_TRACE_START, 0, 0);
}

/**
 * 通用循迹数据读取函数
 * @param data 数据缓冲区（8字节）
 * @return 是否成功读取到有效数据
 */
static bool readVideoData(uint8_t *data)
{
	if (ExtSRAMInterface.ExMem_Read(0x6038) == 0x00)
	{
		return false;
	}
	ExtSRAMInterface.ExMem_Read_Bytes(0x6038, data, 8);
	return isValidTracePacket(data);
}

/**
 * 通用清理函数（停止循迹并清空缓冲区）
 */
static void cleanupLineTracking()
{
	openMv91(SUB_TRACE_STOP, 0, 0);
	emptyData(SIXZEROTHREEEIGHT);
}

// 循迹查找表结构体
typedef struct
{
	uint8_t input; // 输入：线位置掩码
	float output;  // 输出：偏移量
} lookup_table_t;

// 循迹偏移量查找表（适用于黑色和白色巡线）
static const lookup_table_t lookup_table[] = {
	// 适用于黑色巡线
	// 单个传感器触发
	{0x01, -3.5}, // 00000001b (bit 0) - 最左端
	{0x02, -2.5}, // 00000010b (bit 1)
	{0x04, -1.5}, // 00000100b (bit 2)
	{0x08, -0.5}, // 00001000b (bit 3)
	{0x10, 0.0},  // 00010000b (bit 4) ⭐ 中心
	{0x20, 0.5},  // 00100000b (bit 5)
	{0x40, 1.5},  // 01000000b (bit 6)
	{0x80, 2.5},  // 10000000b (bit 7) - 最右端

	// 两个相邻传感器触发（双线）
	{0x03, -3.0}, // 00000011b (bit 0,1)
	{0x06, -2.0}, // 00000110b (bit 1,2)
	{0x0C, -1.0}, // 00001100b (bit 2,3)
	{0x18, 0.0},  // 00011000b (bit 3,4) ⭐ 双线中心
	{0x30, 1.0},  // 00110000b (bit 4,5)
	{0x60, 2.0},  // 01100000b (bit 5,6)
	{0xC0, 3.0},  // 11000000b (bit 6,7)

	// 三个相邻传感器检测（宽线条）
	{0x07, -2.5}, // 00000111b (bit 0,1,2) - 最左端三个
	{0x0E, -1.5}, // 00001110b (bit 1,2,3)
	{0x1C, -0.3}, // 00011100b (bit 2,3,4) - 左偏中心
	{0x38, 1.5},  // 00111000b (bit 3,4,5) - 右偏中心
	{0x70, 1.5},  // 01110000b (bit 4,5,6)
	{0xE0, 2.5},  // 11100000b (bit 5,6,7) - 最右端三个
	{0x0F, -2.0}, // 00001111b
	{0x1E, -1.5}, // 00011110b
	{0x3C, 0.0},  // 00111100b → 完全居中
	{0x78, 1.5},  // 01111000b
	{0xF0, 2.0},  // 11110000b

	// =====================================================
	//   ★ 五相邻触发（极宽黑线、转弯、贴线压到中心）
	// =====================================================
	{0x1F, -1.5}, // 00011111b
	{0x3E, -0.5}, // 00111110b
	{0x7C, 0.0},  // 01111100b（中心）
	{0xF8, 0.5},  // 11111000b

	// 适用于白色巡线（颜色翻转）
	// 单个传感器触发
	{0xFE, -3.5}, // 11111110
	{0xFD, -2.5}, // 11111101
	{0xFB, -1.5}, // 11111011
	//{0xF7, -0.5}, // 11110111
	{0xEF, 0.0},  // 11101111 ⭐中心
	{0xDF, 0.5},  // 11011111
	{0xBF, 1.5},  // 10111111
	{0x7F, 2.5},  // 01111111 - 最右端(反转后)

	// 两个相邻传感器触发
	{0xFC, -3.0}, // 11111100
	{0xF9, -2.0}, // 11111001
	{0xF3, -1.0}, // 11110011
	{0xE7, 0.0},  // 11100111 ⭐ 双线中心
	{0xCF, 1.0},  // 11001111
	{0x9F, 2.0},  // 10011111
	{0x3F, 3.0},  // 00111111

	// 特殊模式
	{0xDF, -3},
	//{0xFF, -3},
	{0xEF, -3},
	{0xBF, 3.0},
	{0xFB, 3.0},
};


/**
 * 查表获取偏移量（通用函数）
 * @param lineMask 线位置掩码
 * @return 偏移量值
 */
static float getLookupOffset(uint8_t lineMask)
{
	const int table_size = sizeof(lookup_table) / sizeof(lookup_table[0]);
	for (int i = 0; i < table_size; i++)
	{
		if (lookup_table[i].input == lineMask)
		{
			return lookup_table[i].output;
		}
	}
	return 0.0f; // 未找到则返回0
}

/**
 * 检测是否到达路口
 * @param lineMask 线位置掩码
 * @param stability 稳定性标志
 * @return 是否到达路口
 */
static bool isAtCrossroad(uint8_t lineMask, uint8_t stability)
{
	return (lineMask == 0xFF || lineMask == 0x81 || lineMask == 0xC3 || lineMask == 0xE7 ||
			stability == 0b101 || stability == 0b111 || stability == 0b110 || stability == 0b11);
}

/**
 * 检测高速循迹路口（特定模式）
 * @param lineMask 线位置掩码
 * @return 是否到达高速循迹路口
 */
static bool isAtHighSpeedCrossroad(uint8_t lineMask)
{
	return (lineMask == 0xFF || lineMask == 0x81 || lineMask == 0xC3 ||
			lineMask == 0xE7 || lineMask == 0b1111110);
}

/**
 * 高速循迹
 * @param spend 基础速度
 *
 * 功能：高速循迹到路口，使用查表法计算偏移量
 */
void highSpeed(uint8_t spend)
{
	// 常量定义
	const uint16_t MILEAGE_EARLY_CHECK = 1500; // 早期里程检查阈值
	const uint16_t MILEAGE_EXIT = 1800;		   // 退出里程阈值
	const uint16_t MILEAGE_MIN = 600;		   // 最小里程要求
	const float OFFSET_MULTIPLIER = 21.0f;	   // 偏移量倍数
	const uint8_t FINAL_SPEED = 80;			   // 最终前进速度
	const uint16_t FINAL_DISTANCE = 555;	   // 最终前进距离

	uint8_t getVideoData[8] = {};
	initLineTracking(0, 0, 0);

	while (true)
	{
		uint16_t mileage = DCMotor.Roadway_mp_Get();
		bool frameOK = readVideoData(getVideoData);

		// 早期阶段数据无效时继续等待
		if (!frameOK && mileage <= MILEAGE_EARLY_CHECK)
		{
			continue;
		}

		uint8_t lineMask = getVideoData[4];

		// 检测路口或达到退出里程
		if (isAtHighSpeedCrossroad(lineMask) || mileage > MILEAGE_EXIT)
		{
			if (mileage > MILEAGE_MIN)
			{
				DCMotor.Stop();
				DCMotor.Car_Go(FINAL_SPEED, FINAL_DISTANCE);
				cleanupLineTracking();
				break;
			}
		}
		// 查表计算偏移量
		float offset = getLookupOffset(lineMask) * OFFSET_MULTIPLIER;
		DCMotor.SpeedCtr(spend - offset, spend + offset);
	}
}

/**
 * 循迹到十字路口
 * @param spend 基础速度
 *
 * 功能：循迹到路口后前进一段距离并停止
 */
void videoLine(uint8_t spend)
{
	uint8_t getVideoData[8] = {};
	initLineTracking(0, 0, 0);
	delay(200);

	while (true)
	{
		if (!readVideoData(getVideoData))
			continue;

		uint8_t lineMask = getVideoData[4];
		uint8_t stability = getVideoData[5];

		// 检测路口
		if (DCMotor.Roadway_mp_Get() > 0 && isAtCrossroad(lineMask, stability))
		{
			DCMotor.Stop();
			cleanupLineTracking();
			break;
		}

		// 查表计算偏移
		float offset = getLookupOffset(lineMask) * 17;
		DCMotor.SpeedCtr(spend - offset, spend + offset);
	}

	delay(300);
}

/**
 * 后退到黑线
 * @param speed 后退速度
 * 功能：后退循迹直到检测到黑线
 */
void VideoLineBreak(uint8_t speed)
{
	uint8_t getVideoData[8] = {};
	initLineTracking(0, 0, 0);
	delay(200);

	while (true)
	{
		if (!readVideoData(getVideoData))
			continue;

		uint8_t lineMask = getVideoData[4];
		uint8_t stability = getVideoData[5];

		// 检测路口
		if (DCMotor.Roadway_mp_Get() > 0 && isAtCrossroad(lineMask, stability))
		{
			DCMotor.Stop();
			cleanupLineTracking();
			break;
		}

		// 查表计算偏移
		float offset = getLookupOffset(lineMask) * 9;
		DCMotor.SpeedCtr(-speed - offset, -speed +offset);
	}

	delay(300);
}

/**
 * 过地形循迹
 * @param spend 基础速度
 * @param len 地形长度（1=短道, 2=长道）
 *
 * 功能：白线模式循迹过地形，根据长度设置目标距离
 */
void Video_Line_4(uint8_t spend, uint8_t len)
{
	uint8_t getVideoData[8] = {0};
	initLineTracking(1, 0x00, 0); // 白线模式

	uint16_t targetDist = (len == 1) ? 1728 : 1730; //1700

	while (true)
	{
		uint16_t distance = DCMotor.Roadway_mp_Get();
		if (distance >= targetDist)
			break;

		if (!readVideoData(getVideoData))
			continue;

		uint8_t lineMask = getVideoData[4];
		// 查表计算偏移（地形用较小系数）
		Serial.println(getVideoData[4]);
		float offset = getLookupOffset(lineMask) * 12;
		DCMotor.SpeedCtr(spend - offset, spend + offset);
		
	}

	cleanupLineTracking();
	DCMotor.Stop();
}

void carBackTracking(uint8_t baseSpeed, int distance)
{
    uint8_t vd[8];
    float cur_off = 0, last_off = 0, output = 0;
    
    // PID参数：倒车时 Kd 必须大，用于防甩尾
    float Kp = 12.0f; 
    float Kd = 60.0f; 

    // 初始化硬件
    while(DCMotor.ClearCodeDisc());
    DCMotor.Roadway_mp_syn();
    initLineTracking(0, 0, 0);
    delay(200);

    unsigned long st = millis();
    
    // 初始化历史误差
    if(readVideoData(vd)) last_off = getLookupOffset(vd[4]);

    while(1)
    {
        if(!readVideoData(vd)) continue;

        // 检查里程是否到达 或 超时(30s)
        uint16_t cardis = ExtSRAMInterface.ExMem_Read(BASEADDRESS+CODEOFFSET) + 
                          (ExtSRAMInterface.ExMem_Read(BASEADDRESS+CODEOFFSET+1) << 8);
        
        if((cardis > 32768) && (((65536 - cardis) >= distance) || ((millis() - st) > 30000)))
        {
            DCMotor.Stop();
            cleanupLineTracking();
            break;
        }

        // PD 计算
        cur_off = getLookupOffset(vd[4]);
        output = (cur_off * Kp) + ((cur_off - last_off) * Kd); // P项回正 + D项阻尼
        last_off = cur_off;

        // 电机控制 (线在右->车尾右甩->左轮后退快)
        int sp_L = -baseSpeed - (int)output;
        int sp_R = -baseSpeed + (int)output;

        // 速度限幅 [-90, 0] 防止反转或过速
        if (sp_L > 0) sp_L = 0; else if (sp_L < -90) sp_L = -90;
        if (sp_R > 0) sp_R = 0; else if (sp_R < -90) sp_R = -90;

        DCMotor.SpeedCtr(sp_L, sp_R);
    }
}


/**
 * 码盘循迹
 * @param spend 基础速度
 * @param distance 目标距离
 *
 * 功能：循迹到指定距离，使用速度映射表
 */
void videoLineMP(uint8_t spend, int distance)
{

	uint8_t getVideoData[8] = {};
	int16_t LSpeed = 0, RSpeed = 0;

	initLineTracking(0, 0, 0);
	delay(200);

	while (true)
	{
		// 检查是否到达目标距离
		if (DCMotor.Roadway_mp_Get() >= distance)
		{
			DCMotor.Stop();
			delay(100);
			cleanupLineTracking();
			break;
		}

		if (!readVideoData(getVideoData))
			continue;

		// 查表获取速度
		uint8_t lineMask = getVideoData[4];
		float offset = getLookupOffset(lineMask) * 35;
		DCMotor.SpeedCtr(spend - offset, spend + offset);
	}
	delay(200);
}

/**
 * 车头摆正
 * @return 摆正结果（1=成功, 0=失败）
 *
 * 功能：根据循迹数据调整车头方向，使其与黑线对齐
 */
int head()
{
	uint8_t getVideoData[8] = {};
	initLineTrackingSync();
	delay(10);
	int sum = 0;

	while (true)
	{
		if (!readVideoData(getVideoData))
			continue;

		switch (getVideoData[4])
		{
		case 0b1000:
			if (getVideoData[6] == 1)
			{
				sum = 1;
			}
			break;
		// 右边循迹灯黑
		case 0b1100:
			Car_RorL(2, 100, 30); // 方向/速度/角度
			break;
		case 0b100:
			break;
		case 0b1110:
			Car_RorL(2, 100, 50);
			break;
		case 0b110:
			Car_RorL(2, 100, 70);
			break;
		case 0b111:
			Car_RorL(2, 100, 110);
			break;
		case 0b11:
			Car_RorL(2, 100, 130);
			break;
		case 0b10000:
			break;
		case 0x01:
			Car_RorL(2, 100, 160);
			break;
		// 左边循迹灯黑
		case 0b11000:
			Car_RorL(1, 100, 50);
			break;
		case 0b1010000:
			Car_RorL(1, 100, 50);
			break;
		case 0b111000:
			Car_RorL(1, 100, 50);
			break;
		case 0b110000: // 0x30
			Car_RorL(1, 100, 55);
			break;
		case 0b1100000:
			break;
		case 0b1110000: // 0x70
			Car_RorL(1, 100, 80);
			break;
		case 0b1000000:
			Car_RorL(1, 100, 90);
			break;
		case 0b11110000:
			break;
		case 0b11100000:
			break;
		case 0b11000000:
			Car_RorL(1, 100, 137);
			break;
		case 0b10000000:
			Car_RorL(1, 100, 150);
			break;
		default:
			break;
		}
		break;
	}

	return sum;
}

/**
 * 车头摆正（AMD版本）
 *
 * 功能：调用 head() 函数进行车头摆正，并设置 data1 标志
 */
void headAMD()
{
	data1 = head();
}

/**
 * 防止死机（避障功能）
 *
 * 功能：检测前方障碍物，如有障碍则后退并报警
 */
void preventDie()
{
	uint8_t getVideoData[8] = {};
	ExtSRAMInterface.ExMem_Read_Bytes(0x6038, getVideoData, 8);
	while (getVideoData[5] > 1)
	{
		Car_Back(40, 40);
		Tba_BEEP(50, 50, 1);
		delay(100);
		ExtSRAMInterface.ExMem_Read_Bytes(0x6038, getVideoData, 8);
	}
	head();
}

// ============================================================================
//                                  其他功能
// ============================================================================

/**
 * 显示消息（兼容旧接口）
 * @param ch 消息类型（1-6）
 * @param nice 距离值（厘米）
 */
void displayMessage(uint8_t ch, uint16_t nice)
{
	char kak[6] = {};
	kak[0] = (nice / 100) + 0x30;
	kak[1] = ((nice / 10) % 10) + 0x30;
	kak[2] = '\0'; // 终止符

	switch (ch)
	{
	case 1:
		sprintf(temp_array_6, "技能成%sCM", kak);
		Rotate_show_Text(temp_array_6, 2);
		break;
	case 2:
		sprintf(temp_array_6, "匠心筑%sCM", kak);
		Rotate_show_Text(temp_array_6, 2);
		break;
	case 3:
		sprintf(temp_array_6, "逐梦扬%sCM", kak);
		Rotate_show_Text(temp_array_6, 2);
		break;
	case 4:
		sprintf(temp_array_6, "技行天%sCM", kak);
		Rotate_show_Text(temp_array_6, 2);
		break;
	case 5:
		sprintf(temp_array_6, "展行业%sCM", kak);
		Rotate_show_Text(temp_array_6, 2);
		break;
	case 6:
		sprintf(temp_array_6, "树人才%sCM", kak);
		Rotate_show_Text(temp_array_6, 2);
		break;
	default:
		break;
	}
}

/**
 * 码盘前进（封装函数）
 * @param speed 速度
 * @param dis 距离
 */
void Go(uint8_t speed, uint16_t dis)
{
	DCMotor.Go(speed, dis);
}

/**
 * 停车（封装函数）
 */
void Stop()
{
	DCMotor.Stop();
}

/**
 * 开机电机自检程序
 *
 * 功能：启动电机并检查码盘是否正常工作
 */
void self_inspection()
{
	DCMotor.Roadway_mp_syn(); // 码盘同步
	DCMotor.StartUp();		  // 启动
	delay(200);
	DCMotor.Stop();
	if (DCMotor.Roadway_mp_Get() <= 10)
	{
		YY_BZW(0, 0);
	}
}
