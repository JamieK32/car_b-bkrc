#include "BKRC_Voice.h"
#include "ExtSRAMInterface.h"

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


 
/*语音播报字母数字*/
void _BKRC_Voice::Voice_Broadcast_Text(char text[]) {
    for (int i = 0; i < strlen(text); i++)
    {
        Serial2.write(text[i]);
        delay(10);
    }
}

void _BKRC_Voice::Voice_Broadcast_Cmd(uint8_t cmd) {
    Serial2.write(cmd);
    delay(10);

}

bool _BKRC_Voice::Voice_WaitFor(UartCb cb, uint16_t timeout_ms = 5000)
{
    if (!cb) return;

    uint8_t buf[VOICE_FRAME_LEN];

    // 关键：把 readBytes 的内部等待时间压到很小（比如 1ms）
    // 否则 readBytes 默认可能阻塞很久，外层 timeout_ms 就不准了
    Serial2.setTimeout(1);

    while (timeout_ms--)
    {
        size_t n = Serial2.readBytes(buf, VOICE_FRAME_LEN);  // 读到多少返回多少

        if (n == VOICE_FRAME_LEN &&
            buf[0] == VOICE_HDR0 &&
            buf[1] == VOICE_HDR1)
        {
            cb(buf[VOICE_PAYLOAD_IDX]);
            return true; // 命中后通常就退出；如果你想继续等多个命中，就删掉 return
        }

        delay(1);
    }
    return false;
}
