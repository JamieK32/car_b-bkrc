#include "Command.h"

_Command Command;

_Command::_Command()
{
}

_Command::~_Command()
{
}

/************************************************************************************************************
【函 数 名】：  Judgment		命令校验函数
【参数说明】：	command	：		要发送的命令
【返 回 值】：	无
【简    例】：	Judgment(command01);	校验command01命令
************************************************************************************************************/
void _Command::Judgment(uint8_t *command)
{
	uint16_t tp = command[ 2 ] + command[ 3 ] + command[ 4 ] + command[ 5 ];
	command[ 6 ] = uint8_t(tp % 256);
}


