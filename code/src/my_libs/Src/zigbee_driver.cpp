#include "zigbee_driver.hpp"
#include "Command.h"
#include "ExtSRAMInterface.h"
#include "log.hpp" // 确保包含日志头文件
#include "Drive.h"

/*----------------------------------------------------------------------------*/

/**
 * @brief 发送一条 Zigbee 原始指令（带重发）
 * * @param cmd        指向 Zigbee 指令帧（固定 8 字节）
 * @param retry_cnt  重发次数（>=1）
 * @param interval_ms 重发间隔（毫秒）
 */
void zigbee_send(uint8_t *cmd, uint8_t retry_cnt = 3U, uint16_t interval_ms = 300U)
{
    if (cmd == NULL || retry_cnt == 0) return;

    for (uint8_t i = 0; i < retry_cnt; i++)
    {
        /* 向 Zigbee 通信窗口写入一帧 */
        ExtSRAMInterface.ExMem_Write_Bytes(
            ZIGBEE_TX_ADDR,   // 建议宏定义，不要裸 0x6008
            cmd,
            8
        );

        if (i + 1 < retry_cnt)
        {
            delay(interval_ms);
        }
    }
}


static inline bool zigbee_recv(uint8_t out[ZIGBEE_FRAME_LEN])
{
    if (!out) return false;

    // RX地址非0表示有数据（沿用你现有约定）
    if (ExtSRAMInterface.ExMem_Read(ZIGBEE_RX_ADDR) == 0x00)
        return false;

    // 读一帧（你现有实现会在读完后把BaseAdd写0x00）
    ExtSRAMInterface.ExMem_Read_Bytes(out, ZIGBEE_FRAME_LEN);
    return true;
}


/* 打开交通灯 */
void Zigbee_Traffic_Open(TrafficID id)
{
    static uint8_t* open_cmd[] = {
        Command.TrafficA_Open,
        Command.TrafficB_Open,
        Command.TrafficC_Open,
        Command.TrafficD_Open,
    };

    zigbee_send(open_cmd[id]);
}

/* 设置交通灯颜色 */
void Zigbee_Traffic_SetColor(TrafficID id, TrafficColor color)
{
    static uint8_t* table[4][4] = {
        {Command.TrafficA_Yellow, Command.TrafficA_Red, Command.TrafficA_Green, Command.TrafficA_Yellow},
        {Command.TrafficB_Yellow, Command.TrafficB_Red, Command.TrafficB_Green, Command.TrafficB_Yellow},
        {Command.TrafficC_Yellow, Command.TrafficC_Red, Command.TrafficC_Green, Command.TrafficC_Yellow},
        {Command.TrafficD_Yellow, Command.TrafficD_Red, Command.TrafficD_Green, Command.TrafficD_Yellow},
    };
    zigbee_send(table[id][color]);
}

/**
 * @brief  Zigbee 车库控制重构 
 * @param  garage_type : 0 -> 车库A (0x0D), 1 -> 车库B (0x05)
 * @param  floor      : 0 -> 停止, 1-5 -> 前往对应楼层
 * @param  is_wait    : 是否原地等待到位
 */
void Zigbee_Garage_Ctrl(GarageType garage_type, int floor, bool is_wait = false, uint16_t wait_time = 8000)  
{
    static const uint8_t ID_TABLE[] = { 0x0D, 0x05 }; 
    uint8_t cmd[8] = { 0x55, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xBB };
    if (garage_type > 1) return;
    cmd[1] = ID_TABLE[garage_type];
    cmd[3] = (uint8_t)floor;
    uint16_t sum = 0;
    for (int i = 2; i < 6; i++) { 
        sum += cmd[i]; 
    }
    cmd[6] = (uint8_t)(sum % 256);
    zigbee_send(cmd); 
    // --- 后续到位等待检测 ---
    if (is_wait) {
        uint8_t feedback[ZIGBEE_FRAME_LEN];
        unsigned long start = millis();

        while (millis() - start < wait_time) {
            if (zigbee_recv(feedback)) {
                if (feedback[1] == ID_TABLE[garage_type] && feedback[3] == (uint8_t)floor) {
                    break;
                }
            }
        }
    }
}

uint8_t Zigbee_Garage_Get(GarageType type, uint16_t wait_time = 8000) {
    if (type == GARAGE_TYPE_A) {
        zigbee_send(Drive.GarageA_Get_Floor);
    } else if (type == GARAGE_TYPE_B) {
        zigbee_send(Drive.GarageB_Get_Floor);
    }
    unsigned long start = millis();
    uint8_t feedback[ZIGBEE_FRAME_LEN];
    while (millis() - start < wait_time) {
        if (zigbee_recv(feedback)) {
            if ((feedback[0] == 0x55) && ((feedback[1] == 0x0D) || (feedback[1] == 0x05))) {
                 if((feedback[2] == 0x03) && (feedback[3] == 0x01)) {
                    return feedback[4];
                 }
            }
        }
    }
    return 0;
}

void Zigbee_Gate_SetState(bool state) {
    if (state) {
        zigbee_send(Drive.Gate_Open);
    } else {
        zigbee_send(Drive.Gate_Close);
    }
}

/**
 * @brief  道闸系统显示车牌 
 * 注意：licence_str只有6位
 */
void Zigbee_Barrier_Display_LicensePlate(const char* licence_str) 
{
    // 1. 发送车牌显示指令 (ID: 0x03, 段: 0x10, 0x11)
    if (licence_str != 0) 
    {
        static const uint8_t IDS[] = { 0x10, 0x11 };
        for (int p = 0; p < 2; p++) 
        {
            uint8_t cmd[8] = { 0x55, 0x03, IDS[p], 0x00, 0x00, 0x00, 0x00, 0xBB };
            int offset = p * 3;
            cmd[3] = (uint8_t)licence_str[offset + 0];
            cmd[4] = (uint8_t)licence_str[offset + 1];
            cmd[5] = (uint8_t)licence_str[offset + 2];
            
            // 校验计算
            uint16_t sum = (uint16_t)cmd[2] + cmd[3] + cmd[4] + cmd[5];
            cmd[6] = (uint8_t)(sum % 256);

            zigbee_send(cmd); // 使用 3 次重发确保送达
        }
    }
}

static uint8_t zigbee_led_ctrl_frame_template[8] = { 0x55, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB };

void Zigbee_LED_Display_Hex(uint8_t rank, uint8_t val) {
    uint8_t cmd[8];
    memcpy(cmd, zigbee_led_ctrl_frame_template, 8);
    cmd[2] = rank; // 第一行或第二行
    cmd[3] = 0x00;
    cmd[4] = val;
    cmd[5] = 0x00; 
    cmd[6] = (cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF;
    zigbee_send(cmd);
}

void Zigbee_LED_Display_Number(uint8_t rank, const char* str) {
    uint8_t cmd[8];
    memcpy(cmd, zigbee_led_ctrl_frame_template, 8);
    cmd[2] = rank;
    uint8_t nibbles[6];
    memset(nibbles, 0, sizeof(nibbles)); 
    for (int i = 0; i < 6; i++) {
    // 【关键修复 2】：检测字符串结束符。如果字符串不到6位，后续位保持 0
        if (str[i] == '\0') break; 

        // ASCII 转半字节逻辑 (0-9, A-Z)
        if (str[i] >= 'A' && str[i] <= 'Z')      nibbles[i] = str[i] - 'A' + 10;
        else if (str[i] >= 'a' && str[i] <= 'z') nibbles[i] = str[i] - 'a' + 10;
        else if (str[i] >= '0' && str[i] <= '9') nibbles[i] = str[i] - '0';
        else nibbles[i] = 0; // 非法字符或空格填充为 0
    }
    cmd[3] = (uint8_t)((nibbles[0] << 4) | (nibbles[1] & 0x0F));
    cmd[4] = (uint8_t)((nibbles[2] << 4) | (nibbles[3] & 0x0F));
    cmd[5] = (uint8_t)((nibbles[4] << 4) | (nibbles[5] & 0x0F));
    cmd[6] = (cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF;
    zigbee_send(cmd);
}

void Zigbee_LED_Display_Distance(uint8_t val){
    uint8_t cmd[8];
    memcpy(cmd, zigbee_led_ctrl_frame_template, 8);
    cmd[2] = 0x04; // 距离模式地址位通常是 0x04
    cmd[3] = 0x00;
    // 分拆百/千位 和 十/个位 (模拟 BCD 显示效果)
    cmd[4] = (uint8_t)(val / 100); 
    cmd[5] = (uint8_t)((((val % 100) / 10) << 4) | (val % 10));
    cmd[6] = (cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF;
    zigbee_send(cmd);
}


void Zigbee_LED_TimerControl(LedTimerControl state) {
    uint8_t cmd[8];
    memcpy(cmd, zigbee_led_ctrl_frame_template, 8);
    cmd[2] = 0x03;
    cmd[3] = state;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = (cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF;
    zigbee_send(cmd);
}


// ========================== 内部工具：BCD/校验 ==========================
static inline uint8_t to_bcd(uint8_t val)
{
    return (uint8_t)(((val / 10) << 4) | (val % 10));
}

// 按你协议：校验=cmd[2]+cmd[3]+cmd[4]+cmd[5] 取低8位
static inline uint8_t checksum_2_5(const uint8_t cmd[8])
{
    uint16_t sum = (uint16_t)cmd[2] + cmd[3] + cmd[4] + cmd[5];
    return (uint8_t)(sum & 0xFF);
}

/**
 * @brief  [2026最终整合版] 公交车站全方位配置驱动 (ID 0x06)
 * @param  info : 同步载荷（日期时间自动转BCD，天气温度按原生HEX）
 * 
 * weather: (0:大风, 1:多云, 2:晴, 3:小雪, 4:小雨, 5:阴)
 * temp   : 温度数值（10进制输入，直接HEX显示，不带负号）
 */
// 调用示例
#if 0
ZigbeeBusSystemSyncPayload data;

// 日期时间：2026-03-21 14:30:45
data.yy   = 26;
data.mon  = 3;
data.day  = 21;
data.hour = 14;
data.min  = 30;
data.sec  = 45;

// 天气 & 温度
data.weather = ZB_WEATHER_CLOUDY; // 多云
data.temp    = 3;                 // 3℃

Zigbee_Bus_FullSync_System(&data);
#endif

void Zigbee_Bus_FullSync_System(const ZigbeeBusSystemSyncPayload *info)
{
    if (!info) return;

    // 1) 构造日期包 (0x30) -> BCD
    uint8_t pkg_date[8] = {
        0x55, 0x06, 0x30,
        to_bcd(info->yy),
        to_bcd(info->mon),
        to_bcd(info->day),
        0x00,
        0xBB
    };
    pkg_date[6] = checksum_2_5(pkg_date);

    // 2) 构造时间包 (0x40) -> BCD
    uint8_t pkg_time[8] = {
        0x55, 0x06, 0x40,
        to_bcd(info->hour),
        to_bcd(info->min),
        to_bcd(info->sec),
        0x00,
        0xBB
    };
    pkg_time[6] = checksum_2_5(pkg_time);

    // 3) 构造天气温度包 (0x42) -> 原生 HEX（你原逻辑保留）
    uint8_t pkg_env[8]  = {
        0x55, 0x06, 0x42,
        info->weather,
        (uint8_t)info->temp, // temp是int8_t，这里按原生字节发送
        0x01,
        0x00,
        0xBB
    };
    pkg_env[6] = checksum_2_5(pkg_env);

    // 4) 同步查询包 (0x31) -> 物理触发信号（原样）
    uint8_t sync_sig[8] = { 0x55, 0x06, 0x31, 0x01, 0x00, 0x00, 0x32, 0xBB };

    zigbee_send(pkg_date, 3, 50);
    zigbee_send(pkg_time, 3, 50);
    zigbee_send(pkg_env , 3, 50);
    zigbee_send(sync_sig, 3, 50);
}

// 播报“富强路站”(id=0x01) ... “友善路站”(id=0x07)
void Zigbee_Bus_SpeakPreset(uint8_t voice_id /*1..7*/) {
    if (voice_id < 1 || voice_id > 7) return;

    uint8_t pkg[8] = {
        0x55, 0x06, 0x10,
        voice_id, 0x00, 0x00,
        0x00, 0xBB
    };
    pkg[6] = checksum_2_5(pkg);

    // ⚠️ 注意：公交站终端节点编号是 0x06（别发错目标）
    zigbee_send(pkg, /*dst=*/0x06, 50);
}

/**
 * [ZIGBEE TFT DRIVER 2026 - FINAL MERGE]
 * 设备：TFT-A (0x0B), TFT-B (0x08)
 */

/* --- 2. 核心中枢：计算校验码并执行 SRAM 可靠写入 (带重发) --- */
static inline void _TFT_Send(TftId id, uint8_t cmd, uint8_t d1, uint8_t d2, uint8_t d3) {
    uint8_t buf[8] = { 0x55, id, cmd, d1, d2, d3, 0, 0xBB };
    buf[6] = (uint8_t)(buf[2] + buf[3] + buf[4] + buf[5]);
    zigbee_send(buf, 3, 50);
}


/**
 * @brief 翻页控制 (模版主程序建议使用这个)
 * mode: 0(跳页), 1(上翻), 2(下翻), 3(轮播)
 */
void Zigbee_TFT_Page_Turn(TftId id, uint8_t mode) {
    _TFT_Send(id, 0x10, mode, 0x00, 0x00);
}

/**
 * @brief 跳到指定页数
 */
void Zigbee_TFT_Goto_Page(TftId id, uint8_t page) {
    _TFT_Send(id, 0x10, 0x00, page, 0x00);
}

/**页
 * @brief 显示 6 位车牌 (分段自动处理)
 */
void Zigbee_TFT_Display_Plate(TftId id, const char* plate) {
    if(!plate || strlen(plate) < 6) return;
    _TFT_Send(id, 0x20, plate[0], plate[1], plate[2]);
    delay(450); // 段间延迟确保硬件刷屏
    _TFT_Send(id, 0x21, plate[3], plate[4], plate[5]);
}

/**
 * @brief 显示距离 (BCD格式修正)
 */
void Zigbee_TFT_Display_Distance(TftId id, uint16_t dist) {
    uint8_t b4 = (uint8_t)(dist / 100);             // 百位
    uint8_t b5 = (uint8_t)((dist/10%10)*16 + dist%10); // BCD解析
    _TFT_Send(id, 0x50, 0x00, b4, b5);
}

/**
 * @brief 计时器操作
 * action: 0(关), 1(开), 2(重置)
 */
void Zigbee_TFT_Timer_Control(TftId id, TftTimerAction action)
{
    _TFT_Send(id, 0x30, (uint8_t)action, 0x00, 0x00);
}

/**
 * @brief 交通标志物图标显示
 * @param sign : 交通标志类型（枚举，取值 1-6）
 */
void Zigbee_TFT_Display_Traffic_Sign(TftId id, TrafficSignType sign)
{
    // 可选：防御式检查，避免传入非法值
    if (sign < TFT_SIGN_STRAIGHT || sign > TFT_SIGN_NO_ENTRY) return;

    _TFT_Send(id, 0x60, (uint8_t)sign, 0x00, 0x00);
}

/**
 * @brief  激活无线电
 */
void Zigbee_Wireless_Activate(uint8_t* keys)
{
    if(keys == NULL) return;

    uint8_t cmd[8];
    cmd[0] = 0x55;          // 帧头
    cmd[1] = 0x0A;          // ID: 无线充电模块
    
    // 将数组中的数据填入 Data 位 (Byte 2-5)
    cmd[2] = keys[0];
    cmd[3] = keys[1];
    cmd[4] = keys[2];
    cmd[5] = keys[3];
    
    // 校验逻辑：仅累加 B2+B3+B4+B5 (完全还原可用代码逻辑)
    uint16_t sum = (uint16_t)cmd[2] + cmd[3] + cmd[4] + cmd[5];
    cmd[6] = (uint8_t)(sum % 256);
    cmd[7] = 0xBB;          // 帧尾

    zigbee_send(cmd);
}