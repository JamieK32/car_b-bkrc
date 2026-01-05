// BKRC_Voice.h

#ifndef _BKRC_VOICE_h
#define _BKRC_VOICE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

extern int decimal_to_hexadecimal(int x);
extern void YY_Text(char text[]);
extern void YY_CP(char text[]);
extern void YY_Num_Hex(uint8_t num);
extern void YY_weather(uint8_t weather);
extern void YY_Num(uint16_t num);
extern void YY_CJ(uint16_t dis, uint8_t dw);
extern void YY_temperature(uint8_t temperature);
extern void YY_BZW(uint8_t mainOrder, uint8_t num);
extern void YY_aNum_Hex(uint8_t num);
extern void YY_aNum(uint16_t num);
extern void YY_Time(uint8_t ali_year, uint8_t ali_month, uint8_t ali_day, uint8_t ali_hour, uint8_t ali_minute, uint8_t ali_second);
extern void YY_Year(uint8_t ali_year, uint8_t ali_month, uint8_t ali_day);
extern void control(void);
extern void YYSB(void);
extern void YY_Calculator();
extern void YY(uint8_t yy1, uint8_t yy2);
extern void YY_sj(char c);
extern void TTS(char* text);
extern void fh(int num);
extern int numToHex_1(int num);

class _BKRC_Voice
{
public:
	_BKRC_Voice();
	~_BKRC_Voice();

	void Initialization(void);							
	uint8_t BKRC_Voice_Extern(void);
	uint8_t TTS_1(uint8_t command);
	uint8_t YY_Speech_Recognition_1(uint8_t number);
	//void YY_Comm_Zigbee(uint8_t Primary, uint8_t Secondary);			
	
	char buffer[18]={0};
	int numdata = 0;
private:

	uint8_t start_voice_dis_1[1];
	uint8_t start_voice_dis[5] = {0xFA, 0xFA, 0xFA, 0xFA, 0xA1};
	uint8_t Voice_Drive(void);
};

extern _BKRC_Voice BKRC_Voice;



#endif

