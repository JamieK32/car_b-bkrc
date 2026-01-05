//#include <stdint.h>
#include "ExtSRAMInterface.h"
#include "My_Lib.h"
#include "Command.h"
#include <string.h>
#include "Decode.h"
#include<math.h>

uint8_t Data_Type;
uint8_t Data_Flag;
uint8_t Data_Length;
uint8_t QR_Data_OTABuf[40];			//二维码中间数组(发给主车数组)
uint8_t QR_Data_OTABuf_1[40];	    //二维码中间数组(备用)
uint8_t TL_Data_OTABuf[40];			//交通灯中间数组
uint8_t Data_OTABuf[40];			//交通灯中间数组
uint8_t OpenMv_AGVData_Flag = 1;
uint8_t QRcode_Flag = 0;





uint8_t Formula_length = 0;//二维码内有效数据长度 
uint8_t length = 0;

//uint8_t LUXIAN[12];

uint8_t Traffic_light_Flag = 0;

uint8_t QR_DataLength = 0;			   //二维码数据的长度

uint8_t OpenMV_QRcode_Start[8] = { 0x55, 0x02, 0x92, 0x01, 0x00, 0x00, 0x93, 0xBB };    //上传给openmv识别二维码
uint8_t OpenMV_trafficlight_Start[8] = { 0x55,0x02,0x91,0x01,0x00,0x00,0x92,0xBB };		//上传给openmv识别交通灯

uint8_t OpenMV_QRcode_Stop[8] = { 0x55, 0x02, 0x92, 0x02, 0x00, 0x00, 0x94, 0xBB };
uint8_t OpenMV_trafficlight_Stop[8] = { 0x55, 0x02, 0x91, 0x02, 0x00, 0x00, 0x93, 0xBB };
uint8_t QRcode_content[50];          //二维码数组

uint8_t Data_Enmpty[20];

uint8_t trafficLight_Color = 0;






int sum = 0;
/*
* tim 需要的二维码张数
*
* PT  需要指定位置二维码(1左 2右)
*
* UD  需要指定位置二维码(3上 4下)
*/


void QRcode_Recognition(uint8_t tim, uint8_t PT, uint8_t UD)
{

	if (tim == 1)
	{
		OpenMV_QRcode_Start[5] = 0;
	}
	else
	{
		OpenMV_QRcode_Start[5] = 0x1F; //1 1111
	}

	for (int num = 0; num < tim; num++)
	{

		OpenMV_QRcode_Start[6] = (OpenMV_QRcode_Start[2] + OpenMV_QRcode_Start[3] + OpenMV_QRcode_Start[4] + OpenMV_QRcode_Start[5]) % 256;

		uint8_t Data_Erasure = 0x00;
		uint8_t i = 0;
		uint8_t break_time = 0;
		uint8_t wheel_Flag = 0;

		int mpp = 0;

		uint8_t Rm = 0;
		uint8_t Lm = 0;

		uint8_t tis = 0;
		uint8_t du = 0;

		if (tim == 1 && PT == 1)
		{
			OpenMV_QRcode_Start[4] = UD;
			Car_RorL(1, 80,190);
			mpp -= 190;
		}
		else if (tim == 1 && PT == 2)
		{
			OpenMV_QRcode_Start[4] = UD;
			Car_RorL(2, 80, 170);
			mpp += 170;
		}
		ExtSRAMInterface.ExMem_Write_Bytes(0x6038, Data_Enmpty, 20);		//清空数据缓冲区
		delay_s(1);
		ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Start, 8);  // OpenMV 开启识别
		delay_s(1);
		delay_ms(500);
		Break_time = millis();

		do {

			Cba_Beep(50, 50, 1);

			if (ExtSRAMInterface.ExMem_Read(0x6038) != 0x00)              //检测OpenMV识别结果   最多只能接收43位数据
			{

				Data_Type = ExtSRAMInterface.ExMem_Read(0x603A);
				Data_Flag = ExtSRAMInterface.ExMem_Read(0x603B);
				Data_Length = ExtSRAMInterface.ExMem_Read(0x603C);

				Data_Length = Data_Length + 6;
				QR_DataLength = Data_Length;
				ExtSRAMInterface.ExMem_Read_Bytes(0x6038, QR_Data_OTABuf, Data_Length);

				Printf_HEX_Info((uint8_t*)0x6038, 50);

				if ((QR_Data_OTABuf[0] == 0x55) && (QR_Data_OTABuf[1] == 0x02))
				{
					if (Data_Type == 0x92)
					{
						QRcode_Flag = 1;

						for (i = 0; i < (Data_Length - 6); i++)
						{
							QRcode_content[i] = QR_Data_OTABuf[i + 5];
							QR_Data_OTABuf[i + 5] = 0;
						}
						length = strlen(QRcode_content);//
						Serial.println("\nchar:");
						for (int i = 0; i < length; i++)
						{
							Serial.print((char)QRcode_content[i]); //显示字符
							Serial.print(" ");
						}
						Serial.println("");

						/*************-  算法开始  -***************/
						
						if (Data_Flag == 0x01)
						{
							OpenMV_QRcode_Start[5] &= 0x17;
							Serial.println("QRcode: 1 !");
					
						
						
							
						
						
						
							
						}

					
						if (Data_Flag == 0x02)
						{
							OpenMV_QRcode_Start[5] &= 0x1B;
							Serial.println("QRcode: 2 !");
						
							
						}
						if (Data_Flag == 0x03)
						{
							OpenMV_QRcode_Start[5] &= 0x1D;
							Serial.println("QRcode: 3 !");


						}
						if (Data_Flag == 0x04)
						{
							OpenMV_QRcode_Start[5] &= 0x1E;
							Serial.println("QRcode: 4 !");


						}



						for (int i = 0; i < length; i++)
						{
							QRcode_content[i] = 0;
						}

						/*************-  算法结束  -***************/

						Cba_Beep(300, 50, 1);

						ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Stop, 8);

						ExtSRAMInterface.ExMem_Write_Bytes(0x6038, &Data_Erasure, 1);
						delay(10);
						ExtSRAMInterface.ExMem_Write_Bytes(0x6038, &Data_Erasure, 1);
						delay(10);
						ExtSRAMInterface.ExMem_Write_Bytes(0x6038, &Data_Erasure, 1);

						break;

					}
				}
			}
			else
			{
				tis++;
				QRcode_Flag = 0;
				Lm += 8;
				Rm += 8;
				if (tis % 2 == 0)
				{
					if (PT == 1)
					{
						Car_RorL(2, 80, 10); //从左往右扫
						mpp += 23;
					}
					else if (PT == 2)
					{
						Car_RorL(1, 80, 10); //从右往左扫
						mpp -= 23;
					}
					else
					{
						if (Lm > 60)
						{
							Lm = 60;
							Rm = 73;
						}

						switch (wheel_Flag)
						{
						case 0:
							Car_RorL(1, 80, Lm);
							mpp -= Lm;

							wheel_Flag = 2;
							break;
						case 1:
							Car_RorL(1, 80, Lm * 2);
							mpp -= Lm * 2;

							wheel_Flag = 2;
							break;
						case 2:
							Rm += 2;
							Car_RorL(2, 80, Rm * 2);
							mpp += Rm * 2 - 16;

							wheel_Flag = 1;
							break;
						}
					}
				}

				if (tim > 2)
				{

					if (OpenMV_QRcode_Start[4] == 4 || OpenMV_QRcode_Start[4] == 0)
					{
						OpenMV_QRcode_Start[4] = 3;
						Serial.println("UP");
					}
					else if (OpenMV_QRcode_Start[4] == 3)
					{
						OpenMV_QRcode_Start[4] = 4;
						Serial.println("DW");
					}

				}



				OpenMV_QRcode_Start[6] = (OpenMV_QRcode_Start[2] + OpenMV_QRcode_Start[3] + OpenMV_QRcode_Start[4] + OpenMV_QRcode_Start[5]) % 256;
				ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Start, 8);  // OpenMV 开启识别
				delay_s(1);
				delay_ms(500);
			}
		} while ((millis() - Break_time) < 30000);


		Serial.print("\nmpp = ");
		Serial.println(mpp);
		if (mpp > 0)
		{
			Car_RorL(1, 90, mpp);
		}
		else if (mpp < 0)
		{
			Car_RorL(2, 90, -mpp);
		}
	}
}
void Traffic_light_recognition(uint8_t choice)//交通灯识别
{
	
	if (choice == 1)
	{
		Zigbee_Send_degree(Command.TrafficA_Open, 3, 300);
	}
	else
	{
		Zigbee_Send_degree(Command.TrafficB_Open, 3, 300);
	}
	uint8_t break_time = 0;
	uint8_t Data_Erasure = 0x00;
	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Start, 8);  // OpenMV 开启识别
	delay(200);
	//ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Start, 8);  // OpenMV 开启识别########
	//delay(200);
	//ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Start, 8);  // OpenMV 开启识别
	//delay(200);
	Break_time = millis();
	do {
		//Cba_Beep(50, 50, 1);
		//          Serial.println(millis());
		if (ExtSRAMInterface.ExMem_Read(0x6038) != 0x00)              //检测OpenMV识别结果
		{
			//Serial.print(" RXD ");
			//	for (int i = 0; i < 50; i++)
			//	{
				//	Serial.print(ExtSRAMInterface.ExMem_Read(0x6038 + i), HEX);
			//		Serial.print(" ");
			//	}
			//Serial.println(" ");
			Data_Type = ExtSRAMInterface.ExMem_Read(0x603A);
			Data_Flag = ExtSRAMInterface.ExMem_Read(0x603B);
			Data_Length = ExtSRAMInterface.ExMem_Read(0x603C);
			//Data_Length = Data_Length + 6;
			ExtSRAMInterface.ExMem_Read_Bytes(0x6038, TL_Data_OTABuf, Data_Length);
			if ((TL_Data_OTABuf[0] == 0x55) && (TL_Data_OTABuf[1] == 0x02))
			{
				Serial.print(" ENSURE ");
				if (Data_Type == 0x91)
				{
					Serial.print(" light ");
					if (Data_Flag == 0x01)
					{
						Serial.print(" OK ");
						//SYN7318.VSPTest(Debug[1], 0);
						trafficLight_Color = Data_Length;
						Serial.print(trafficLight_Color);
						//ExtSRAMInterface.ExMem_Write_Bytes(0x6180, Data_OTABuf, Data_Length); //使用自定义数据区上传OpenMV识别结果
						//ExtSRAMInterface.ExMem_Write_Bytes(0x6180, Data_OTABuf, Data_Length); //使用自定义数据区上传OpenMV识别结果
						//ExtSRAMInterface.ExMem_Write_Bytes(0x6180, Data_OTABuf, Data_Length); //使用自定义数据区上传OpenMV识别结果
						Traffic_light_Flag = 1;
						if (choice == 1)
						{
							//trafficLightA_Color = trafficLight_Color;
							//switch (trafficLightA_Color)
							//{
							//case 1:
							//	Zigbee_Send_degree(Command.TrafficA_Red, 3, 300);
							//	break;
							//case 2:
							//	Zigbee_Send_degree(Command.TrafficA_Green, 3, 300);
							//	break;
							//case 3:
							//	Zigbee_Send_degree(Command.TrafficA_Yellow, 3, 300);
							//	break;
							//default:
							//	Zigbee_Send_degree(Command.TrafficA_Red, 3, 300);
							//	break;
							//}
						}
						else
						{
							//trafficLightB_Color = trafficLight_Color;
							//switch (trafficLightB_Color)
							//{
							//case 1:
							//	Zigbee_Send_degree(Command.TrafficB_Red, 3, 300);
							//	break;
							//case 2:
							//	Zigbee_Send_degree(Command.TrafficB_Green, 3, 300);
							//	break;
							//case 3:
							//	Zigbee_Send_degree(Command.TrafficB_Yellow, 3, 300);
							//	break;
							//default:
							//	Zigbee_Send_degree(Command.TrafficB_Red, 3, 300);
							//	break;
							//}
						}
						//              Serial.write(TFT_HEX, Data_Length - 6);
						//              Serial.println("");
						//
						//SYN7318.VSPTest(YY_trafficLight[trafficLight_Color - 1], 1);
						ExtSRAMInterface.ExMem_Write_Bytes(0x6038, &Data_Erasure, 1);
						delay(10);
						ExtSRAMInterface.ExMem_Write_Bytes(0x6038, &Data_Erasure, 1);
						delay(10);
						ExtSRAMInterface.ExMem_Write_Bytes(0x6038, &Data_Erasure, 1);
						break;
					}
					else if (Data_Flag == 0x02)
					{
						//Serial.print(" ERROR ");
						ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Start, 8);  // OpenMV 开启识别
						//delay_s(1);
					//	delay(300);
					//	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Start, 8);  // OpenMV 开启识别#####
					////delay_s(1);
					//	delay(300);
					//	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Start, 8);  // OpenMV 开启识别#####
					////delay_s(1);
					//	delay(300);
					}
					else if (Data_Flag == 0x03)
					{
						//Serial.print(" ING ");
					}
				}
			}
		}
	} while ((millis() - Break_time) < 8000);
	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_trafficlight_Stop, 8);
	if (Traffic_light_Flag == 0)
	{
		Tba_BEEP(50, 50, 3);
		trafficLight_Color = 1;
		if (choice == 1)
		{
			Zigbee_Send_degree(Command.TrafficA_Yellow, 3, 300);
		}
		else
		{
			Zigbee_Send_degree(Command.TrafficB_Green, 3, 300);
		}
		//SYN7318.VSPTest(YY_trafficLight[trafficLight_Color - 1], 1);
		//SYN7318.VSPTest(YY_trafficLight[trafficLight_Color - 3], 1); #############
	}
	Traffic_light_Flag = 0;
	//TTS(Debug[0]);
	
}

void LicensePlate_Pattern_recognition(uint8_t model)//TFT执行翻页操作
{
	if (model == 1)
	{
		
			Tba_BEEP(50, 50, 1);
			TFT_Test_Zigbee('A', 0x10, 0x01, 0x00, 0x00, 2);/*	 0x00  由第二副指令（0x01~0x20）指定显示那张图片   0x01 图片向上翻页											0x02  图片向下翻页 0x03   图片自动向下翻页显示，间隔时间 10S */
			delay_s(2);
	
	}
	else
	{
		
			Tba_BEEP(50, 50, 1);
			TFT_Test_Zigbee('B', 0x10, 0x01, 0x00, 0x00, 2);
			delay_s(2);
		
	}
}

void QR(uint8_t size) 
{
	uint8_t i = 0;
	uint8_t j = 0;
	Break_time = millis();
	/*Serial.print("Break_time: ");
	Serial.println(Break_time);*/
	OpenMV_QRcode_Start[5] = size;
	OpenMV_QRcode_Start[6] = (OpenMV_QRcode_Start[2] + OpenMV_QRcode_Start[3] + OpenMV_QRcode_Start[4] + OpenMV_QRcode_Start[5]) % 256;
	ExtSRAMInterface.ExMem_Write_Bytes(0x6038, Data_Enmpty, 20);		//清空数据缓冲区
	delay_s(1);
	ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Start, 8);
	delay_s(1);
	while (true)
	{
		// OpenMV 开启识别
	  //Cba_Beep(50, 50, 1);
		//Serial.println("1");
		delay_ms(300);
		if (ExtSRAMInterface.ExMem_Read(0x6038) != 0x00)              //检测OpenMV识别结果   最多只能接收43位数据
		{
			Data_Type = ExtSRAMInterface.ExMem_Read(0x603A);
			Data_Flag = ExtSRAMInterface.ExMem_Read(0x603B);
			Data_Length = ExtSRAMInterface.ExMem_Read(0x603C);


			Data_Length = Data_Length + 6;
			QR_DataLength = Data_Length;
			ExtSRAMInterface.ExMem_Read_Bytes(0x6038, QR_Data_OTABuf, Data_Length);

			//Printf_HEX_Info((uint8_t*)0x6038, 50);

			if ((QR_Data_OTABuf[0] == 0x55) && (QR_Data_OTABuf[1] == 0x02))
			{
				if (Data_Type == 0x92)
				{
					QRcode_Flag = 1;

					for (int i = 0; i < (Data_Length - 6); i++)
					{
						QRcode_content[i] = QR_Data_OTABuf[i + 5];
						QR_Data_OTABuf[i + 5] = 0;
						QR_Data_OTABuf[0] = 0;
					}
					Formula_length = strlen(QRcode_content);

					Serial.println("\nchar:");
					for (int i = 0; i < Formula_length; i++)
					{
						Serial.print((char)QRcode_content[i]); //显示字符
						Serial.print(" ");
					}
					Serial.println("");

					/*************-  算法开始  -***************/

					if (Data_Flag == 0x01)
					{
						OpenMV_QRcode_Start[5] &= 0x17;
						Serial.println("QRcode: 1 !");

					}
					if (Data_Flag == 0x02)
					{
						OpenMV_QRcode_Start[5] &= 0x1B;
						Serial.println("QRcode: 2 !");
					}
					if (Data_Flag == 0x03)
					{
						OpenMV_QRcode_Start[5] &= 0x1D;
						Serial.println("QRcode: 3 !");
					}
					if (Data_Flag == 0x04)
					{
						OpenMV_QRcode_Start[5] &= 0x1E;
						Serial.println("QRcode: 4 !");
					}

					for (int i = 0; i < length; i++)
					{
						QRcode_content[i] = 0;
					}

					/*************-  算法结束  -***************/

					Cba_Beep(300, 50, 1);
					ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Stop, 8);
					delay_ms(300);
					break;
				}
			}
		}
		else {
			ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Start, 8);
			delay_ms(300);
		}
		//Serial.println(millis());
		if (millis() - Break_time > 30000) {
			ExtSRAMInterface.ExMem_Write_Bytes(0x6008, OpenMV_QRcode_Stop, 8);
			Serial.print("ooeu");
			break;
		}
		else if (millis() - Break_time > 10000 && i == 0) {
			Car_RorL(1, 80, 80);
			i = 1;
		}
		else if (millis() - Break_time > 20000 && j == 0) {
			Car_RorL(2, 80, 160);
			j = 1;
		}
	}
	if (i == 1 && j != 1) {
		Car_RorL(2, 80, 80);
	}
	else if (j == 1) {
		Car_RorL(1, 80, 80);
	}
	else if (i == 1 && j == 1) {
		Car_RorL(1, 80, 80);
	}
	Serial.print("break");

}