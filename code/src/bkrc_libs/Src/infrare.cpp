// 
// 
// 

#include "infrare.h"

/* 如果你电路是“低电平点亮红外LED”（你代码注释就是这种），打开这个宏 */
#define USE_ACTIVE_LOW_OUTPUT_FOR_SEND_PIN

/* 可选：如果你想在 AVR 上更省程序/更稳 timing，可以把发送脚设成宏（需在 include 前） */
#define IR_SEND_PIN RITX_PIN

#include <IRremote.hpp>

_Infrare Infrare;

_Infrare::_Infrare(uint8_t _pin)
{
	pinMode(_pin, OUTPUT);
	pin = _pin;
	digitalWrite(_pin, TXD_OFF);
}
_Infrare::_Infrare()
{

}

_Infrare::~_Infrare()
{
}

/***************************************************************
** 功能：	红外发射端口初始化
** 参数：	无参数
** 返回值：	无
****************************************************************/
void _Infrare::Initialization(void)
{
	pinMode(RITX_PIN, OUTPUT);
	digitalWrite(RITX_PIN, TXD_OFF);
	//pin = _pin;

}

/***************************************************************
** 功能：	红外发射子程序
** 参数：	*s：指向要发送的数据
**          n：数据长度
** 返回值：	无
****************************************************************/
void _Infrare::Transmition(uint8_t *s, int n)
{
	u8 i, j, temp;

	digitalWrite(RITX_PIN, TXD_ON);    //13    HIGH  低电平表示:发射打开
	delayMicroseconds(9000);
	digitalWrite(RITX_PIN, TXD_OFF);   //13    LOW   高电平表示:发射关闭
	delayMicroseconds(4560);
	

	for (i = 0; i < n; i++)
	{
		for (j = 0; j<8; j++)
		{
			temp = (s[ i ] >> j) & 0x01;
			if (temp == 0)						//发射0
			{
				digitalWrite(RITX_PIN, TXD_ON);
				delayMicroseconds(500);			//延时0.5ms
				digitalWrite(RITX_PIN,TXD_OFF);
				delayMicroseconds(500);			//延时0.5ms
			}
			if (temp == 1)						//发射1
			{
				digitalWrite(RITX_PIN, TXD_ON);
				delayMicroseconds(500);			//延时0.5ms
				digitalWrite(RITX_PIN, TXD_OFF);
				delayMicroseconds(1000);
				delayMicroseconds(800);			//延时1.69ms  690

			}
		}
	}
	digitalWrite(RITX_PIN, TXD_ON);				//结束
	delayMicroseconds(560);						//延时0.56ms
	digitalWrite(RITX_PIN, TXD_OFF);			//关闭红外发射
}


void _Infrare::Transmition_New(uint8_t *s, int n)
{
    if (!s || n <= 0) return;

    // 你原来的时序参数（单位：微秒）
    const uint16_t HDR_MARK   = 9000;
    const uint16_t HDR_SPACE  = 4560;
    const uint16_t BIT_MARK   = 500;
    const uint16_t ZERO_SPACE = 500;
    const uint16_t ONE_SPACE  = 1800; // 你原来是 1000+800
    const uint16_t STOP_MARK  = 560;

    // 为了避免 AVR 上 malloc，给个上限（按你实际最大 n 调）
    const int MAX_BYTES = 32;
    if (n > MAX_BYTES) n = MAX_BYTES;

    // raw 数组长度：头(2) + 每bit(2)*8*n + 结尾mark(1) = 3 + 16*n
    static uint16_t raw[3 + 16 * MAX_BYTES];
    int rawLen = 0;

    raw[rawLen++] = HDR_MARK;
    raw[rawLen++] = HDR_SPACE;

    for (int i = 0; i < n; i++) {
        for (uint8_t j = 0; j < 8; j++) {           // LSB first：和你原来一致
            uint8_t bit = (s[i] >> j) & 0x01;
            raw[rawLen++] = BIT_MARK;
            raw[rawLen++] = bit ? ONE_SPACE : ZERO_SPACE;
        }
    }

    raw[rawLen++] = STOP_MARK;

    // 38kHz 载波（大多数红外接收头/家电遥控都是 38k）
    IrSender.sendRaw(raw, rawLen, 38);
}
