#include "FHT.hpp"
#include "infrare.h"
#include "Drive.h"
#include "BEEP.h"
#include "BH1750.h"
#include "ExtSRAMInterface.h"
#include "log.hpp"
#include "zigbee_driver.hpp"

#define FHT_DATA_SIZE 6
#define FHT_TX_GAP_MS   30

static const uint8_t FHT_DEFAULT_OPEN_CODE[6] = {0x03, 0x05, 0x14, 0x45, 0xDE, 0x92};

// ============================================================================
//                              烽火台激活
// ============================================================================
void Infrared_Activate_FHT(uint8_t* code, uint8_t repeat) {
    if (!code) return;
    for (uint8_t i = 0; i < repeat; i++) {
        Infrare.Transmition(code, FHT_DATA_SIZE);
        delay(FHT_TX_GAP_MS);
    }
}

static void FHT_Zigbee_SendFrame(uint8_t cmd, uint8_t s1, uint8_t s2, uint8_t s3) {

    uint8_t buf[8];
    buf[0] = 0x55;      // 帧头
    buf[1] = 0x07;      // 烽火台设备ID
    buf[2] = cmd;       // 主指令
    buf[3] = s1;        // 副指令1
    buf[4] = s2;        // 副指令2
    buf[5] = s3;        // 副指令3
    buf[6] = (uint8_t)(cmd + s1 + s2 + s3); // 校验和：主+副1+副2+副3
    buf[7] = 0xBB;      // 帧尾

    zigbee_send(buf, 8); // 执行回调发送
}

// 3. [ZigBee] 激活烽火台
void FHT_Zigbee_RemoteActive(uint8_t* custom_code) {
    // 指向正确代码 (传入为0则用默认码)
    const uint8_t* code = (custom_code == 0) ? FHT_DEFAULT_OPEN_CODE : custom_code;

    // 协议 7.2.1：开启报警台需分两段发送密码
    // 第一帧：主指令 0x10，载荷为前3字节密码
    FHT_Zigbee_SendFrame(0x10, code[0], code[1], code[2]);
    
    delay(50); // 必须有间隔，否则 Zigbee 模块处理不过来

    // 第二帧：主指令 0x11，载荷为后3字节密码
    FHT_Zigbee_SendFrame(0x11, code[3], code[4], code[5]);
}
