#ifndef VE_DEMO_H__
#define VE_DEMO_H__


/* 等待启动命令（阻塞） */
void Car_Wait_Start(void);

/* 接收 demo：等待 DATA_TYPE_CUSTOM，打印长度 */
void Car_Receive_Demo(void);

/* 发送 demo：发送一段固定字节数组，等待 ACK */
void Car_Send_Demo(void);

/* 发送自定义结构体包（1+3 bytes），等待 ACK */
void Car_Send_Custom_Pkt(void);

/* 接收自定义结构体包并打印字段 */
void Car_Receive_Custom_Pkt(void);



#endif 
