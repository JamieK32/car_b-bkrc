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

/* ========================================================================== */
/*                                立体车库控制 (修正版以匹配 main.ino)         */
/* ========================================================================== */

// 内部辅助：ID转换
static inline uint8_t _garage_get_id(GarageType type) {
    return (type == GARAGE_TYPE_A) ? 0x0D : 0x05;
}

/**
 * @brief 获取车库当前层数
 * 注意：保留原名 Zigbee_Get_Floor 以兼容 main.ino
 */
int Zigbee_Get_Floor(GarageType type)
{
    uint8_t id = _garage_get_id(type);
    
    // 构造查询指令
    uint8_t cmd[8] = { 0x55, id, 0x02, 0x01, 0x00, 0x00, 0x00, 0xBB };
    cmd[6] = (uint8_t)((cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF);

    // 清空缓存
    uint8_t dummy[8];
    while (zigbee_recv(dummy));

    zigbee_send(cmd, 2, 100);

    unsigned long start = millis();
    while (millis() - start < 500)
    {
        uint8_t fb[8];
        if (zigbee_recv(fb))
        {
            if (fb[0] == 0x55 && fb[1] == id && fb[2] == 0x03 && fb[7] == 0xBB)
            {
                if (fb[4] >= 1 && fb[4] <= 4) return fb[4];
                if (fb[3] >= 1 && fb[3] <= 4) return fb[3];
            }
        }
    }
    return -1;
}

void Zigbee_Garage_Ctrl(GarageType type, int target_floor, bool is_wait = true, unsigned long timeout_ms = 20000)
{
    // 参数安全检查
    if (target_floor < 1 || target_floor > 4) return;

    // 1. 检查当前层
    int current_floor = Zigbee_Get_Floor(type);
    if (current_floor == target_floor) {
        return;
    }

    uint8_t id = _garage_get_id(type);

    // 2. 发送移动指令 (转为 uint8_t 发送)
    uint8_t cmd[8] = { 0x55, id, 0x01, (uint8_t)target_floor, 0x00, 0x00, 0x00, 0xBB };
    cmd[6] = (uint8_t)((cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF);
    
    uint8_t dummy[8];
    while (zigbee_recv(dummy));
    
    zigbee_send(cmd, 3, 200);

    if (!is_wait) return;

    // 3. 阻塞等待
    unsigned long start_time = millis();
    unsigned long last_poll_time = 0;
    delay(1000); 

    while (millis() - start_time < timeout_ms)
    {
        if (millis() - last_poll_time > 1000)
        {
            last_poll_time = millis();
            int reported = Zigbee_Get_Floor(type);
            if (reported == target_floor) {
                break;
            }
        }
    }
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
void Zigbee_Gate_Display_LicensePlate(const char* licence_str) 
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
ZigbeeBusInfo data;

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

void Zigbee_Bus_FullSync_System(const ZigbeeBusInfo *info)
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
    zigbee_send(pkg, 3, 50);
}

void Zigbee_Bus_SpeakRandom(void) {
        uint8_t pkg[8] = {
        0x55, 0x06, 0x20,
        0x01, 0x00, 0x00,
        0x00, 0xBB
    };
    pkg[6] = checksum_2_5(pkg);
    zigbee_send(pkg, 3, 50);
}

// ========================== 公交站读取：内部工具 ==========================

static inline uint8_t bcd_to_u8(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F));
}

static inline bool zigbee_frame_check_basic(const uint8_t f[8])
{
    if (!f) return false;
    if (f[0] != 0x55) return false;
    if (f[7] != 0xBB) return false;
    return true;
}

static inline bool zigbee_bus_wait_resp(uint8_t expect_main_cmd,
                                        uint8_t out[8],
                                        uint16_t wait_time_ms)
{
    unsigned long start = millis();
    while (millis() - start < wait_time_ms)
    {
        if (zigbee_recv(out))
        {
            // 基本合法性
            if (!zigbee_frame_check_basic(out)) continue;

            // 必须是公交站节点 ID = 0x06
            if (out[1] != 0x06) continue;

            // 主指令匹配
            if (out[2] != expect_main_cmd) continue;

            return true;
        }
    }
    return false;
}

bool Zigbee_Bus_Read_Date(uint8_t* yy, uint8_t* mon, uint8_t* day, uint16_t wait_time = 800)
{
    uint8_t cmd[8] = {0x55, 0x06, 0x31, 0x01, 0x00, 0x00, 0x00, 0xBB};
    cmd[6] = (uint8_t)((cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF);
    zigbee_send(cmd, 3, 300);

    uint8_t fb[8];
    if (!zigbee_bus_wait_resp(0x02, fb, wait_time)) return false;

    if (yy)  *yy  = bcd_to_u8(fb[3]);
    if (mon) *mon = bcd_to_u8(fb[4]);
    if (day) *day = bcd_to_u8(fb[5]);
    return true;
}

bool Zigbee_Bus_Read_Time(uint8_t* hour, uint8_t* min, uint8_t* sec, uint16_t wait_time = 800)
{
    uint8_t cmd[8] = {0x55, 0x06, 0x41, 0x01, 0x00, 0x00, 0x00, 0xBB};
    cmd[6] = (uint8_t)((cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF);
    zigbee_send(cmd, 3, 300);

    uint8_t fb[8];
    if (!zigbee_bus_wait_resp(0x03, fb, wait_time)) return false;

    if (hour) *hour = bcd_to_u8(fb[3]);
    if (min)  *min  = bcd_to_u8(fb[4]);
    if (sec)  *sec  = bcd_to_u8(fb[5]);
    return true;
}

bool Zigbee_Bus_Read_Env(uint8_t* weather, int8_t* temp, uint16_t wait_time = 800)
{
    uint8_t cmd[8] = {0x55, 0x06, 0x43, 0x00, 0x00, 0x00, 0x00, 0xBB};
    cmd[6] = (uint8_t)((cmd[2] + cmd[3] + cmd[4] + cmd[5]) & 0xFF);
    zigbee_send(cmd, 3, 300);

    uint8_t fb[8];
    if (!zigbee_bus_wait_resp(0x04, fb, wait_time)) return false;

    if (weather) *weather = fb[3];
    if (temp)    *temp    = (int8_t)fb[4]; // 协议：16进制温度字节，单位℃
    return true;
}


bool Zigbee_Bus_Read_All(ZigbeeBusInfo* out, uint16_t wait_time_each = 800)
{
    if (!out) return false;

    bool ok1 = Zigbee_Bus_Read_Date(&out->yy, &out->mon, &out->day, wait_time_each);
    bool ok2 = Zigbee_Bus_Read_Time(&out->hour, &out->min, &out->sec, wait_time_each);
    bool ok3 = Zigbee_Bus_Read_Env(&out->weather, &out->temp, wait_time_each);

    return ok1 && ok2 && ok3;
}

static const char* ZigbeeWeatherToStr(uint8_t w)
{
    switch (w) {
        case 0: return "Windy";
        case 1: return "Cloudy";
        case 2: return "Sunny";
        case 3: return "Light Snow";
        case 4: return "Light Rain";
        case 5: return "Overcast";
        default: return "Unknown";
    }
}

void PrintBusInfo(const ZigbeeBusInfo* info)
{
    if (!info) {
        LOG_P("[BUS] info is NULL\r\n");
        return;
    }

    // 如果你希望按 20xx 年显示，这里 +2000；否则就直接打印两位年
    LOG_P("[BUS] Date: 20%02u-%02u-%02u\r\n", info->yy, info->mon, info->day);
    LOG_P("[BUS] Time: %02u:%02u:%02u\r\n", info->hour, info->min, info->sec);
    LOG_P("[BUS] Weather: %u (%s)\r\n", info->weather, ZigbeeWeatherToStr(info->weather));
    LOG_P("[BUS] Temp: %d C\r\n", (int)info->temp);
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

void Zigbee_TFT_Display_Hex(TftId id, uint8_t hex1, uint8_t hex2, uint8_t hex3) {
    _TFT_Send(id, 0x40, hex1, hex2, hex3);
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
