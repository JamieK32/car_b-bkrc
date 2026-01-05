#include "BKRC_Voice.h"
#include "ExtSRAMInterface.h"
#include "My_Lib.h"
#include "my_libs/Inc/openmv2.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>


  _BKRC_Voice BKRC_Voice;


_BKRC_Voice::_BKRC_Voice()
{

}

_BKRC_Voice::~_BKRC_Voice()
{
}



/*语音播报字母数字*/
void YY_Text(char text[]) {
    for (int i = 0; i < strlen(text); i++)
    {
        Serial2.write(text[i]);
        /*if (i!=0&& text[i]== text[i+1]) {
            delay_ms(200);
        }
        else {
            
        }*/
        delay_ms(10);
       
    }
}



//语音播报车牌
void YY_CP(char text[]) {
    Serial2.write(0x10);
    delay_ms(800);
    YY_Text(text);
}

/*小创语音播报
参数详情    mainOrder   num     播报内容
             6/7        -1      A到B/B到A
             08         层数     车库层数
             09         档位     路灯档位
             13         颜色     交通灯颜色               
*/
void YY_BZW(uint8_t mainOrder,uint8_t num) {
    Serial2.write(mainOrder);
    Serial2.write(num);
}

/*
* 小创播报当前时间  
* 全部需要十六进制数 0x20 => 20年
*/
void YY_Time(uint8_t ali_year, uint8_t ali_month, uint8_t ali_day, uint8_t ali_hour, uint8_t ali_minute, uint8_t ali_second) {
   /* delay_ms(500);
    YY_aNum_Hex(ali_year);
    delay_ms(500);
    Serial2.write(0x14);
    Serial2.write(0x02);*/
    delay_ms(500);
    YY_Num_Hex(ali_month);
    delay_ms(500);
    Serial2.write(0x14);
    Serial2.write(0x03);
    delay_ms(500);
    YY_Num_Hex(ali_day);
    delay_ms(500);
    Serial2.write(0x14);
    Serial2.write(0x04);
    delay_ms(500);
    YY_Num_Hex(ali_hour);
    delay_ms(500);
    Serial2.write(0x14);
    Serial2.write(0x05);
    delay_ms(500);
    YY_Num_Hex(ali_minute);
    delay_ms(500);
    Serial2.write(0x14);
    Serial2.write(0x06);
   /* delay_ms(500);
    YY_Num_Hex(ali_second);
    delay_ms(500);
    Serial2.write(0x14);
    Serial2.write(0x07);*/
}


void YY_Year(uint8_t ali_year, uint8_t ali_month, uint8_t ali_day) {
    delay_ms(500);
    YY_aNum_Hex(ali_year);
    delay_ms(1);
    Serial2.write(0x14);
    Serial2.write(0x02);
    delay_ms(1);
    YY_Num_Hex(ali_month);
    delay_ms(1);
    Serial2.write(0x14);
    Serial2.write(0x03);
    delay_ms(1);
    YY_Num_Hex(ali_day);
    delay_ms(1);
    Serial2.write(0x14);
    Serial2.write(0x04);
}

void fh(int num) {
    switch (num) {
    case 1:

        break;
    case 2://十
        Serial2.write(0x16);
        Serial2.write(0x2);
        break;
    case 3:
        Serial2.write(0x16);
        Serial2.write(0x3);
        break;
    case 4:
        Serial2.write(0x16);
        Serial2.write(0x4);
        break;
    case 5:
        Serial2.write(0x16);
        Serial2.write(0x5);
        break;
    }

}
void YY_aNum_Hex(uint8_t num) {
    int tempNum = num;
    int len = 0;
    tempNum = num;
    while (tempNum != 0) {
        tempNum /= 0x10;
        len++;
    }
    int mo = 0x10;
    int chu = 0x01;
    for (int i = 0; i < len - 1; i++) {
        mo *= 0x10;
        chu *= 0x10;
    }
    int ti = 0;
    for (int i = len; i > 0; i--) {
        int num2 = (num % mo) / chu;
        u8 numnum = 0x30 + num2;      
        if (ti == numnum) {
            delay_ms(800);
        }
        else {
            delay_ms(10);
        }
        Serial2.write(numnum);
        ti = numnum;
        
        mo /= 0x10;
        chu /= 0x10;
    }
}


void YY_aNum(uint16_t num) {
    int tempNum = num;
    int len = 0;
    tempNum = num;
    while (tempNum != 0) {
        tempNum /= 10;
        len++;
    }

    int mo = 10;
    int chu = 1;
    for (int i = 0; i < len - 1; i++) {
        mo *= 10;
        chu *= 10;
    }
    int ti = 0;
    for (int i = len; i > 0; i--) {
        int num2 = (num % mo) / chu;

        u8 numnum = 0x30 + num2;
        if (ti == numnum) {
            delay_ms(800);
        }
        else {
            delay_ms(10);
        }
        Serial2.write(numnum);
        ti = numnum;

        mo /= 10;
        chu /= 10;
    }
}



void YY_Num(uint16_t num)
{


    int tempNum = num;
    int len = 0;

    while (tempNum != 0) {
        tempNum /= 10;
        len++;
    }

    int mo = 10;
    int chu = 1;
    for (int i = 0; i < len - 1; ++i) {
        mo *= 10;
        chu *= 10;
    }

    for (int i = len; i > 0; i--) {
        int num2 = (num % mo) / chu;
        if (i == 1 && num2 == 0) {
            break;
        }
        u8 numnum = 0x30 + num2;
        
        Serial2.write(numnum);
        delay_ms(100);
        if (num2 != 0) {
            fh(i);
        }
        delay_ms(100);
        mo /= 10;
        chu /= 10;
    }
}

//语音播报天气1为大风（0x01）
void  YY_weather(uint8_t weather) {
    delay_ms(1000);
    Serial2.write(0x19);
    Serial2.write(0x11);
    delay_ms(1000);
    Serial2.write(0x19);
    Serial2.write(weather);
}
//语音播报温度（十进制）
void YY_temperature(uint8_t temperature) {
    Serial2.write(0x19);
    Serial2.write(0x07);
    delay_ms(1000);
    YY_Num(temperature);
    delay_ms(1000);
    Serial2.write(0x19);
    Serial2.write(0x10);
}

void YY(uint8_t yy1, uint8_t yy2) {
    Serial2.write(yy1);
    Serial2.write(yy2);
}

void YY_sj(char c) {
    if (c == 'l' || c == 'L') {
        YY(0x06, 0x11);
    }
    else if (c == 'r' || c == 'R') {
        YY(0x06, 0x12);
    }
    else {
        YY(0x06, 0x13);
    }
}

void YY_Num_Hex(uint8_t num)  
{
    int tempNum = num;
    int len = 0;

    while (tempNum != 0) {
        tempNum /= 0x10;
        len++;
    }
    
    int mo = 0x10;
    int chu = 0x01;
    for (int i = 0; i < len - 1; ++i) {
        mo *= 0x10;
        chu *= 0x10;
    }
    
    for (int i = len; i > 0; i--) {
        int num2 = (num % mo) / chu;
        if (i == 1&&num2==0) {
            break;
        }
        u8 numnum = 0x30 + num2;
        if (i != 2 || num2!=1) {//习惯问题，当十位为一时把一省略
            Serial2.write(numnum);
        }
        
        delay_ms(1);
        if (num2 != 0) {
            fh(i);
        }
        delay_ms(1);
        mo /= 0x10;
        chu /= 0x10;

    }
    return 0;
}

void YY_CJ(uint16_t dis,uint8_t dw) {
    Serial2.write(0x15);
    Serial2.write(0x1);
    delay_ms(500);
    YY_Num(dis);
    delay_ms(500);
    if (dw==1) {
       Serial2.write(0x15);
       Serial2.write(0x2);
       
    }
    else if(dw==2) {
        Serial2.write(0x15);
        Serial2.write(0x3);
    }
    else {
        Serial2.write(0x15);
        Serial2.write(0x4);
    }
    
}

int decimal_to_hexadecimal(int x)

{

    int hexadecimal_number = 0, remainder, count = 0;

    for (count = 0; x > 0; count++)

    {

        remainder = x % 16;

        hexadecimal_number = hexadecimal_number + remainder * pow(10, count);

        x = x / 16;

    }

    return hexadecimal_number;

}
//数字10进制转16进制
int numToHex_1(int num) {
    int hex = 0x00;
    while (num > 16) {
        num -= 16;
        hex += 0x10;
    }
    if (num <= 9)
    {
        hex += num;
    }
    else {
        switch (num)
        {
        case 10:hex += 0x0A;
        case 11:hex += 0x0B;
        case 12:hex += 0x0C;
        case 13:hex += 0x0D;
        case 14:hex += 0x0E;
        case 15:hex += 0x0F;
        default:
            break;
        }
    }
    //printf("%X", hex);
    return hex;
}

void TTS(char* text) {
    int len = strlen(text);
    char temp[3];
    uint16_t GBK2 = 0;
    uint16_t GBK3 = 0;
    /*Serial.print("len:");
    Serial.println(len);*/
    Serial.print("TTS:");
    for (int i = 0; i < len; i++)
    {
        if (text[i] <0)  
        {
            temp[0] = text[i];
            temp[1] = text[i + 1];
            temp[2] = text[i + 2];
            GBK2 = getGBK(temp);
            if (GBK2 == GBK3) {
                delay_ms(800);
            }
            else {
                delay_ms(10);
            }
            Serial.print(temp);
            i += 2;
            /*Serial.println(GBK2>>8, HEX);
            Serial.println(GBK2 & 0xFF, HEX);*/
            Serial2.write(GBK2 >> 8);
            Serial2.write(GBK2 & 0xFF);
            GBK3 = GBK2;
        }
        else {//不是汉字
            if (text[i] == GBK3) {
                delay_ms(800);
            }
            else {
                delay_ms(10);
            }
            Serial.print(text[i]);
            Serial2.write(text[i]);
            GBK3 = text[i];
        }
       
    }
    Serial.println("");
}

//初始化
/************************************************************************************************************
【函 数 名】：	Initialization		初始化函数
【参数说明】：	无
【返 回 值】：	无
【简    例】：	Initialization();	初始化相关接口及变量
************************************************************************************************************/
void _BKRC_Voice::Initialization(void)
{
  Serial2.begin(115200);
  while(Serial2.read() >=0);
  Serial.begin(115200);
  while(Serial.read() >=0);
}




/**************************************************
  功  能：语音识别函数
  参  数：  无
  返回值：  语音词条ID    词条内容
            富强路站::富强路站 :55020501
            民主路站::民主路站 :55020502
            文明路站::文明路站 :55020503
            和谐路站::和谐路站 :55020504
            爱国路站::爱国路站 :55020505
            敬业路站::敬业路站 :55020506
            友善路站::友善路站 :55020507

            秀丽山河::秀丽山河 :55020601
            美好生活::美好生活 :55020602
            齐头并进::齐头并进 :55020603
            扬帆起航::扬帆起航 :55020604

            技能成才::技能成才 :55020701
            匠心筑梦::匠心筑梦 :55020702
            逐梦扬威::逐梦扬威:55020703
            技行天下::技行天下 :55020704
            展行业百技::展行业百技 :55020705
            树人才新观::树人才新观 :55020706
*/
void YYSB(void) {
    uint8_t YY_Random[8] = { 0x55, 0x06, 0x20, 0x01, 0x00, 0x00, 0x21, 0xBB };   // 语音播报随机指令
    while (Serial.read() >= 0); //清空串口数据
    //delay(800);
    uint8_t yy_Falg=0xFF;
    uint16_t time = millis();
    uint16_t timeNow = 0;
    uint8_t zl=99;
    uint8_t buf[4];
    for(int i=0;i<6;){
        if (timeNow>=3000*i) {
            YY_Comm_Zigbee(0x20, 0x01);
            time = timeNow;
            i++;
        }
        delay(100);
        Serial2.readBytes(buf, 4);
        if (buf[0] == 0x55 && buf[1] == 0x02) {
            Serial.print("buf[3]:");
            Serial.println(buf[3]);
            yy_Falg = buf[3];
            ali_yybb = yy_Falg;
            Serial.print("ali_yybb:");
            Serial.println(ali_yybb);
            return;
        }
        timeNow= millis()- time;
        
    }
}

/**************************************************
  功  能：语音识别回传命令解析函数
  参  数： 无
  返回值：  语音词条ID /小创语音识别模块状态
**************************************************/
uint8_t _BKRC_Voice::Voice_Drive(void)
{
    uint8_t  status = 0;
    if (Serial2.available() > 0)
    {
        numdata = Serial2.readBytes(buffer, 4);
        if (buffer[0] == 0x55 && buffer[1] == 0x02 )
        {
            status = buffer[3];
        }
        else
        {
            status = 0;
        }
    }
    return status;
}




//小创识别并执行对应指令
void control(void) {
    if (Serial2.available() > 0)
    {
        uint8_t buf[4];
        Serial2.readBytes(buf, 4);
        if (buf[0] == 0x55 && buf[1] == 0x02) {
            switch (buf[2])
            {
            case 0x01:
                Garage_Test_Zigbee('A', 0x01, buf[3]);
                break;
            case 0x02:
                Garage_Test_Zigbee('B', 0x01, buf[3]);
                break;
            case 0x03:
                if (buf[3] == 0x01) {//循迹角度
                    servoControl(-55);
                }
                else if (buf[3] == 0x02) {
                    servoControl(15);
                }
                else if (buf[3] == 0x03) {
                    servoControl(4);
                }
                else if (buf[3] == 0x04) {
                    initPhotosensitive(1);
                }
                else if (buf[3] == 0x05) {
                    LED_time_Zigbee(2, 2);
                }
                else if (buf[3] == 0x06) {
                    LED_time_Zigbee(1, 2);
                }else if (buf[3] == 0x07) {
                    if (ali_falg == 0) {
                        Serial2.write(0x18);
                        Serial2.write(0x01);
                    }
                    else {
                        Serial2.write(0x18);
                        Serial2.write(0x02);
                    }
                }
                break;
            case 0x04:/****************************  摄像头部分  ***************************/
                switch (buf[3])
                {
                case 0x00:
                    yyzz_falg = 1;
                    break;
                case 0x01://前进
                    if (yyzz_falg == 1) {
                        highSpeed(80);                             //高速循迹到路口
                    }
                    break;
                case 0x02:
                    if (yyzz_falg == 1) {
                        left(2);
                    }
                    break; 
                case 0x03:
                    if (yyzz_falg == 1) {
                        right(2);
                    }
                    break;
                case 0x04:
                   Identify_QR_2(1);
                    break;
                case 0x05:
                      Identify_QR_2(2);
                    break;
                case 0x06:
                     Identify_QR_2(3);
                    break;
                case 0x07:
                     Identify_QR_2(1);
                    break;
                case 0x08:
                     Identify_QR_2(1);
                    break;
                case 0x09:
                    identifyTraffic(1);			/*交通灯*/
                    break;
                case 0x0A:
                    identifyTraffic(2);			/*交通灯*/
                    break;
                case 0x0B:
                    ali_back(2, 0);
                    break;
                case 0x0C:
                    ali_back(1, 0);
                    break;
                case 0x0D:
                    highSpeed(80);                             //高速循迹到路口
                    highSpeed(80);                             //高速循迹到路口
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }

        }
    }
}



