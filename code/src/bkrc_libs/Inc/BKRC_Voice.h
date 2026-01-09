// BKRC_Voice.h

#ifndef _BKRC_VOICE_h
#define _BKRC_VOICE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <stdint.h>

enum : uint16_t
{
    // ===== 10xx 站点/路线 =====
    VC_ST_FUQIANG_ROAD    = 0x1001, // 富强路站
    VC_ST_MINZHU_ROAD     = 0x1002, // 民主路站
    VC_ST_WENMING_ROAD    = 0x1003, // 文明路站
    VC_ST_HEXIE_ROAD      = 0x1004, // 和谐路站
    VC_ST_AIGUO_ROAD      = 0x1005, // 爱国路站
    VC_ST_JINGYE_ROAD     = 0x1006, // 敬业路站
    VC_ROUTE_A_TO_B       = 0x1007, // A到B
    VC_ROUTE_B_TO_A       = 0x1008, // B到A

    // ===== 11xx 车辆/设备提示 =====
    VC_CAR_PLATE_IS       = 0x1101, // 车牌是
    VC_3D_DISPLAY         = 0x1102, // 立体显示
    VC_BEACON_CODE_IS     = 0x1103, // 烽火台激活码为

    // ===== 12xx 车库 =====
    VC_GARAGE_FLOOR_1     = 0x1201, // 车库层数是一层
    VC_GARAGE_FLOOR_2     = 0x1202, // 车库层数是二层
    VC_GARAGE_FLOOR_3     = 0x1203, // 车库层数是三层
    VC_GARAGE_FLOOR_4     = 0x1204, // 车库层数是四层

    // ===== 13xx 路灯/交通灯 =====
    VC_STREETLIGHT_GEAR_1 = 0x1301, // 路灯挡位是一档
    VC_STREETLIGHT_GEAR_2 = 0x1302, // 路灯挡位是二档
    VC_STREETLIGHT_GEAR_3 = 0x1303, // 路灯挡位是三档
    VC_STREETLIGHT_GEAR_4 = 0x1304, // 路灯挡位是四档

    VC_TRAFFIC_RED        = 0x1305, // 交通灯的颜色为红色
    VC_TRAFFIC_GREEN      = 0x1306, // 交通灯的颜色为绿色
    VC_TRAFFIC_YELLOW     = 0x1307, // 交通灯的颜色为黄色

    // ===== 14xx 时间/日期 =====
    VC_TIME_IS            = 0x1401, // 时间为
    VC_YEAR               = 0x1402, // 年
    VC_MONTH              = 0x1403, // 月
    VC_DAY                = 0x1404, // 日
    VC_HOUR               = 0x1405, // 时
    VC_MINUTE             = 0x1406, // 分
    VC_SECOND             = 0x1407, // 秒

    // ===== 15xx 距离/单位 =====
    VC_DISTANCE_IS        = 0x1501, // 距离为
    VC_MM                 = 0x1502, // 毫米
    VC_CM                 = 0x1503, // 厘米
    VC_M                  = 0x1504, // 米

    // ===== 16xx 数字/量级 =====
    VC_DOT                = 0x1601, // 点
    VC_TEN                = 0x1602, // 十
    VC_HUND               = 0x1603, // 百
    VC_THOU               = 0x1604, // 千
    VC_WAN                = 0x1605, // 万
    VC_100K               = 0x1606, // 十万
    VC_1M                 = 0x1607, // 百万
    VC_10M                = 0x1608, // 千万
    VC_100M               = 0x1609, // 亿

    // ===== 17xx 天气/温度 =====
    VC_TODAY_WEATHER_IS   = 0x1701, // 今天的天气为
    VC_WEATHER_WINDY      = 0x1702, // 大风
    VC_WEATHER_CLOUDY     = 0x1703, // 多云
    VC_WEATHER_SUNNY      = 0x1704, // 晴
    VC_WEATHER_SNOW       = 0x1705, // 小雪
    VC_WEATHER_RAIN       = 0x1706, // 小雨
    VC_WEATHER_OVERCAST   = 0x1707, // 阴天
    VC_TEMPERATURE_IS     = 0x1708, // 温度为
    VC_CELSIUS            = 0x1709, // 摄氏度

    // ===== 18xx 状态提示 =====
    VC_AUTO_START         = 0x1801, // 全自动启动
    VC_TIMEOUT_OPEN       = 0x1802, // 请注意，超时进入开启
    VC_TIMEOUT_CLOSE      = 0x1803, // 请注意，超时进入关闭
    VC_STATE_CLOSED       = 0x1804, // 超时进入处于关闭状态
    VC_STATE_OPEN         = 0x1805, // 超时进入处于开启状态
};



#define VOICE_HDR0        (0x55)
#define VOICE_HDR1        (0x02)
#define VOICE_FRAME_LEN   (4)
#define VOICE_PAYLOAD_IDX (2)




class _BKRC_Voice
{
public:
	_BKRC_Voice();
	~_BKRC_Voice();
	void Voice_Broadcast_Text(char text[]);
	uint8_t Voice_WaitFor(uint16_t timeout_ms = 5000);
	void Initialization(void);							
	void Voice_Broadcast_Cmd(uint16_t cmd);
    void Voice_SpeakNumber(uint16_t num);
    void Voice_SpeakWeather(uint16_t weather_code);
    void Voice_SpeakTemperature(int16_t temp_c);
private:

};

extern _BKRC_Voice BKRC_Voice;



#endif

