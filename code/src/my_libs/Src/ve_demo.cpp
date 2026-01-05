#include "car_controller.hpp"
#include "log.hpp"
#include "k210_protocol.hpp"
#include "k210_app.hpp"
#include "lsm6dsv16x.h"
#include "qrcode_datamap.hpp"
#include "algorithm.hpp"
#include "vehicle_exchange.h"
#include "ve_demo.h"

#define WAIT_TIMEOUT_MS        3000
#define SEND_TIMEOUT_MS        3000

static void log_ve(const char *fmt, ...);

void Car_Wait_Start(void) {
    WaitData(DATA_TYPE_CMD, WAIT_TIMEOUT_MS);
}

void Car_Receive_Demo(void) {
    log_ve("Receive\n");
    // 阻塞等待最多 2000ms，接收类型为 DATA_TYPE_SENSOR 的数据
    uint8_t *data = WaitData(DATA_TYPE_CUSTOM, WAIT_TIMEOUT_MS);

    if (data != NULL) {
        uint8_t len = GetDataLen();
		log_ve("Data receive OK, len = %d\n", len);

    } else {
        log_ve("Data receive Timeout\n");
    }
}

void Car_Send_Demo(void) {
    log_ve("Send\n");
    uint8_t my_sensor_data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x55, 0xAA, 0x43, 0x88}; 
    uint16_t data_len = sizeof(my_sensor_data);
    bool success = SendArray(DATA_TYPE_CUSTOM, my_sensor_data, data_len, SEND_TIMEOUT_MS);
    if (success) {
        log_ve("Send OK\n");
    } else {
        log_ve("Send Timeout\n");
    }
}


void Car_Send_Custom_Pkt(void) {
    log_ve("Send Custom\n");
    #pragma pack(push, 1)
    typedef struct {
        uint8_t cmd;
        uint8_t a;
        uint8_t b;
        uint8_t c;
    } MyPkt;
    #pragma pack(pop)
    MyPkt p = { .cmd = 1, .a = 2, .b = 3, .c = 4 };
    bool success = SendArray(DATA_TYPE_CUSTOM, (const uint8_t*)&p, sizeof(p), SEND_TIMEOUT_MS);
    if (success) {
        log_ve("Send OK\n");
    } else {
        log_ve("Send Timeout\n");
    }
}

void Car_Receive_Custom_Pkt(void) {
    log_ve("Receive Custom\n");
    #pragma pack(push, 1)
    typedef struct {
        uint8_t cmd;
        uint8_t a;
        uint8_t b;
        uint8_t c;
    } MyPkt;
    #pragma pack(pop)
    uint8_t *data = WaitData(DATA_TYPE_CUSTOM, WAIT_TIMEOUT_MS);
    if (data != NULL && GetDataLen() >= sizeof(MyPkt)) {
        MyPkt *p = (MyPkt *)data; 
        log_ve("cmd = %d, a = %d, b = %d, c = %d\n", p->cmd, p->a, p->b, p->c);
    } else {
        log_ve("Receive Timeout\n"); 
    }
}


static void log_ve(const char *fmt, ...)
{
    if (!fmt) return;
    char buf[256];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

#if CURRENT_CAR == CAR_A

    #ifdef SCREEN_PRINTF_TAKES_STRING_ONLY
        /* screen_printf 只收字符串 */
        screen_printf("%s", buf);  /* 或者 screen_puts(buf); 取决于你实际 API */
    #else
        /* screen_printf 是 printf 风格 */
        screen_printf("%s", buf);
    #endif

#elif CURRENT_CAR == CAR_B

    /* LOG_P 常见是宏/函数：LOG_P("xxx")，很多实现只收字符串 */
    LOG_P(buf);

#else
    /* 未定义车型：可以选择丢弃，或者输出到默认通道 */
    (void)buf;
#endif
}
