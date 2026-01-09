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
}

void _BKRC_Voice::Voice_Broadcast_Text(char text[]) {
    for (int i = 0; i < strlen(text); i++)
    {
        Serial2.write(text[i]);
        delay(10);
    }
}

static const uint16_t VOICE_GAP_MS = 200;

void _BKRC_Voice:: Voice_Broadcast_Cmd(uint16_t cmd)
{
    if (cmd <= 0xff) { 
        Serial2.write((uint8_t)cmd); 
    } else {
        uint8_t hi = (uint8_t)((cmd >> 8) & 0xFF);
        uint8_t lo = (uint8_t)(cmd & 0xFF);
        Serial2.write(hi);
        Serial2.write(lo);
    }
    Serial2.flush();
    delay(VOICE_GAP_MS);
}


uint8_t _BKRC_Voice::Voice_WaitFor(uint16_t timeout_ms = 5000)
{
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
            return buf[VOICE_PAYLOAD_IDX];
        }

        delay(1);
    }
    return 0;
}

// 工具：发一个数字 0~9 的词条
static inline uint8_t vc_digit(uint8_t d) { return (uint8_t)('0' + d); }

// 类方法：播报一个自然数（0~9999）
void _BKRC_Voice::Voice_SpeakNumber(uint16_t num)
{
    if (num == 0) {
        Voice_Broadcast_Cmd('0');
        return;
    }

    // 分解：千百十个
    uint8_t qian = (num / 1000) % 10;
    uint8_t bai  = (num / 100)  % 10;
    uint8_t shi  = (num / 10)   % 10;
    uint8_t ge   = (num % 10);

    bool started = false;   // 是否已经开始播报（前导零不读）
    bool need_zero = false; // 中间是否需要补“零”

    // 千
    if (qian) {
        Voice_Broadcast_Cmd(vc_digit(qian));
        Voice_Broadcast_Cmd(VC_THOU);
        started = true;
    }

    // 百
    if (bai) {
        if (need_zero) { Voice_Broadcast_Cmd('0');  need_zero = false; }
        Voice_Broadcast_Cmd(vc_digit(bai));
        Voice_Broadcast_Cmd(VC_HUND);
        started = true;
    } else {
        if (started && (shi || ge)) need_zero = true;
    }

    // 十
    if (shi) {
        if (need_zero) { Voice_Broadcast_Cmd('0');   need_zero = false; }

        // 10~19：读“十X”，不读“一十X”
        if (!started && shi == 1) {
            Voice_Broadcast_Cmd(VC_TEN);
        } else {
            Voice_Broadcast_Cmd(vc_digit(shi));
            Voice_Broadcast_Cmd(VC_TEN);
        }
        started = true;
    } else {
        if (started && ge) need_zero = true;
    }

    // 个
    if (ge) {
        if (need_zero) { Voice_Broadcast_Cmd('0');   need_zero = false; }
        Voice_Broadcast_Cmd(vc_digit(ge));
    }
}



void _BKRC_Voice::Voice_SpeakWeather(uint16_t weather_code)
{
    Voice_Broadcast_Cmd(VC_TODAY_WEATHER_IS);
    delay(400); //等待说完
    Voice_Broadcast_Cmd(VC_WEATHER_WINDY + weather_code);
}

void _BKRC_Voice::Voice_SpeakTemperature(int16_t temp_c)
{
    Voice_Broadcast_Cmd(VC_TEMPERATURE_IS);
    Voice_SpeakNumber(temp_c);
    Voice_Broadcast_Cmd(VC_CELSIUS);
}
