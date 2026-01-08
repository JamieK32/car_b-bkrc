#include "k210_protocol.hpp"
#include "ExtSRAMInterface.h"
#include "Command.h"
#include <string.h>
#include <math.h>
#include "DCMotor.h"

#include "BKRC_Voice.h"
#include "zigbee_driver.hpp"
#include "log.hpp"
#include "k210_app.hpp"

#define LOG_K210_EN  0

#if LOG_K210_EN
  #define log_k210(fmt, ...)  LOG_P("[K210] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define log_k210(...)  do {} while(0)
#endif

uint8_t g_qr_raw_slots[QRCODE_SLOTS][QRCODE_SLOT_SIZE];

static inline void handleQrFrame(K210_Frame *frame);

#define QRCODE_TIMEOUT_S 25

void Servo_SetAngle(int angle)
{
    K210_CMD cmd;
    uint8_t abs_angle;

    if (angle >= 0)
    {
        cmd = CMD_SET_SERVO_POSITIVE;
        abs_angle = angle;
    }
    else
    {
        cmd = CMD_SET_SERVO_NEGATIVE;
        abs_angle = abs(angle);
    }

    K210_SendCmd(cmd, abs_angle, 0);
}


uint8_t identifyTraffic_New(TrafficID id)
{
    uint8_t result = 0;
    K210_Frame frame;

    Servo_SetAngle(SERVO_TRAFFIC_IDENTIFY_ANGLE);

    delay(1700);
    
    /* 2. Zigbee 打开对应交通灯 */
    Zigbee_Traffic_Open(id);

    /* 4. 启动识别 */
    K210_SendCmd(CMD_TRAFFIC_START, 0, 0);

    unsigned long start = millis();
    const unsigned long TIMEOUT_MS = 6000;

    while (millis() - start < TIMEOUT_MS)
    {
        if (!K210_ReadFrame(&frame, K210_READ_FIXED)) {
            continue;
        }
        K210_ClearRxByFrame(&frame);
        if (frame.cmd == CMD_TRAFFIC_RESULT)
        {
            log_k210("cmd ok %d", frame.cmd);
            result = frame.fixed.data1;
            Zigbee_Traffic_SetColor(id, (TrafficColor)result);
            K210_SendCmd(CMD_TRAFFIC_STOP, 0, 0);
            break;
        }
    }

    if (result == 0)
    {
        result = LIGHT_YELLOW;
        Zigbee_Traffic_SetColor(id, LIGHT_YELLOW);
    }

    Servo_SetAngle(SERVO_DEFAULT_ANGLE);
    log_k210("result = %d", result);
    return result;
}


bool identifyQrCode_New(uint8_t slot_count)
{
    K210_Frame frame;
    bool res = false;
    Servo_SetAngle(SERVO_QRCODE_IDENTIFY_ANGLE);
    memset(g_qr_raw_slots, 0, sizeof(g_qr_raw_slots));
    K210_SendCmd(CMD_QR_CODE_START, slot_count, QRCODE_TIMEOUT_S);
    unsigned long deadline = millis() + (QRCODE_TIMEOUT_S * 1000U);
    while ((long)(millis() - deadline) < 0)
    {
        if (!K210_ReadFrame(&frame, K210_READ_VAR_LEN)) continue;
        handleQrFrame(&frame);
        K210_ClearRxByFrame(&frame);
        log_k210("QrCode Slot Index: %d", frame.var.var_id);
        if (frame.var.var_id >= slot_count - 1) {
            res = true;
            K210_SendCmd(CMD_QR_CODE_STOP, 0, 0);
            break;
        }
    }

    K210_SendCmd(CMD_QR_CODE_STOP, 0, 0);
    Servo_SetAngle(SERVO_DEFAULT_ANGLE);
    return res;
}


static inline void handleQrFrame(K210_Frame *frame)
{
    uint8_t id = frame->var.var_id;
    if (id >= QRCODE_SLOTS) return;

    uint16_t n = frame->var.data_len;
    if (n > (QRCODE_SLOT_SIZE - 1)) n = QRCODE_SLOT_SIZE - 1;

    memcpy(g_qr_raw_slots[id], frame->var.data, n);
    g_qr_raw_slots[id][n] = '\0';
}
