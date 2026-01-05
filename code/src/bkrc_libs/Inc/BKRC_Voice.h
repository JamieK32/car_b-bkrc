// BKRC_Voice.h

#ifndef _BKRC_VOICE_h
#define _BKRC_VOICE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif


#define VOICE_HDR0        (0x55)
#define VOICE_HDR1        (0x02)
#define VOICE_FRAME_LEN   (4)
#define VOICE_PAYLOAD_IDX (2)


typedef void (*UartCb)(uint8_t byte);


class _BKRC_Voice
{
public:
	_BKRC_Voice();
	~_BKRC_Voice();
	void Voice_Broadcast_Text(char text[]);
	bool Voice_WaitFor(UartCb cb, uint16_t timeout_ms = 5000);
	void Initialization(void);							
	void Voice_Broadcast_Cmd(uint8_t cmd);
private:



};

extern _BKRC_Voice BKRC_Voice;



#endif

