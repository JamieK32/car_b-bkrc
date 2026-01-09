#include "vehicle_exchange.h"
#include "zigbee_frame_io.h"

#define TX_REPEAT_COUNT   4
#define TX_REPEAT_GAP_MS  50

/* 收到数据后，如果连续这么久都没新帧，就认为“这一包结束了” */
#define RX_IDLE_END_MS    1000

/* ====== 小工具：超时判断（支持 uint32_t 回卷） ====== */
static inline bool time_reached(uint32_t start_ms, uint32_t timeout_ms)
{
    return (timeout_ms == 0) ? true : ((uint32_t)(get_ms() - start_ms) >= timeout_ms);
}

/* ====== 小工具：帧级校验（head/tail/cs） ====== */
static inline uint8_t calc_cs_local(uint8_t type, uint8_t b0, uint8_t b1, uint8_t b2)
{
    return (uint8_t)((type + b0 + b1 + b2) & 0xFF);
}

static bool frame_valid(const uint8_t buf[8])
{
    if (buf[0] != ZIGBEE_HEAD0 || buf[1] != ZIGBEE_HEAD1) return false;
    if (buf[7] != ZIGBEE_TAIL) return false;

    uint8_t type = buf[2];
    uint8_t b0   = buf[3];
    uint8_t b1   = buf[4];
    uint8_t b2   = buf[5];
    uint8_t cs   = buf[6];

    return (cs == calc_cs_local(type, b0, b1, b2));
}

/* ====== 小工具：重复发 1 帧 ====== */
static void send_frame_repeat(uint8_t type, uint8_t b0, uint8_t b1, uint8_t b2)
{
    for (int i = 0; i < TX_REPEAT_COUNT; i++) {
        SendFrame8(type, b0, b1, b2);
        delay_ms(TX_REPEAT_GAP_MS);
    }
}

/* ===================== CMD ===================== */

void Send_Start_Cmd(void)
{
    send_frame_repeat(CMD_START1, CMD_START2, CMD_START3, CMD_START4);
}

bool Wait_Start_Cmd(uint32_t timeout_ms)
{
    uint8_t buf[8];
    uint32_t start = get_ms();

    while (!time_reached(start, timeout_ms)) {

        if (ZigbeeRead8(buf)) {
            if (!frame_valid(buf)) continue;

            if (buf[2] == CMD_START1 && buf[3] == CMD_START2 && buf[4] == CMD_START3 && buf[5] == CMD_START4) {
                return true;
            }
        } else {
            delay_ms(1);
        }
    }
    return false;
}

/* ===================== DATA HELPERS ===================== */

static inline bool any_nonzero3(uint8_t a, uint8_t b, uint8_t c)
{
    return (a | b | c) != 0;
}

static inline void clear3(uint8_t *a, uint8_t *b, uint8_t *c)
{
    *a = 0; *b = 0; *c = 0;
}

static inline void send_item_if_nonzero_and_clear(CarDataEnum id, uint8_t *d0, uint8_t *d1, uint8_t *d2)
{
    if (any_nonzero3(*d0, *d1, *d2)) {
        send_frame_repeat((uint8_t)id, *d0, *d1, *d2);
        clear3(d0, d1, d2);
    }
}

static inline void send_item_u8_if_nonzero_and_clear(CarDataEnum id, uint8_t *v)
{
    if (*v != 0) {
        send_frame_repeat((uint8_t)id, *v, 0, 0);
        *v = 0;
    }
}

static inline void send_item_u16bytes_if_nonzero_and_clear(CarDataEnum id, uint8_t v2[2])
{
    if ((v2[0] | v2[1]) != 0) {
        send_frame_repeat((uint8_t)id, v2[0], v2[1], 0);
        v2[0] = 0; v2[1] = 0;
    }
}

/* ===================== SEND WHOLE STRUCT ===================== */
/* ✅ 规则：字段不为0才发送；发送后清零（避免重复发送） */
bool SendData(ProtocolData_t *inout)
{
    if (!inout) return false;

    /* 车牌：6位 -> 前3/后3（分别判断各自3字节是否全0） */
    send_item_if_nonzero_and_clear(ENUM_CAR_PLATE_FRONT_3,
                                   &inout->car_plate[0], &inout->car_plate[1], &inout->car_plate[2]);

    send_item_if_nonzero_and_clear(ENUM_CAR_PLATE_BACK_3,
                                   &inout->car_plate[3], &inout->car_plate[4], &inout->car_plate[5]);

    /* 时间 */
    send_item_if_nonzero_and_clear(ENUM_DATE_YYYYMMDD,
                                   &inout->datetime.date[0], &inout->datetime.date[1], &inout->datetime.date[2]);

    send_item_if_nonzero_and_clear(ENUM_TIME_HHMMSS,
                                   &inout->datetime.time[0], &inout->datetime.time[1], &inout->datetime.time[2]);

    /* 1位数据 */
    send_item_u8_if_nonzero_and_clear(ENUM_GARAGE_FINAL_FLOOR, &inout->garage_final_floor);
    send_item_u8_if_nonzero_and_clear(ENUM_TERRAIN_STATUS,     &inout->terrain_status);
    send_item_u8_if_nonzero_and_clear(ENUM_TERRAIN_POSITION,   &inout->terrain_position);
    send_item_u8_if_nonzero_and_clear(ENUM_VOICE_BROADCAST,    &inout->voice_broadcast);

    /* 6位：拆成 low3/high3（每段非0才发；发完清段） */
    send_item_if_nonzero_and_clear(ENUM_BEACON_CODE_LOW_3,
                                   &inout->beacon_code[0], &inout->beacon_code[1], &inout->beacon_code[2]);
    send_item_if_nonzero_and_clear(ENUM_BEACON_CODE_HIGH_3,
                                   &inout->beacon_code[3], &inout->beacon_code[4], &inout->beacon_code[5]);

    send_item_if_nonzero_and_clear(ENUM_TFT_DATA_LOW_3,
                                   &inout->tft_data[0], &inout->tft_data[1], &inout->tft_data[2]);
    send_item_if_nonzero_and_clear(ENUM_TFT_DATA_HIGH_3,
                                   &inout->tft_data[3], &inout->tft_data[4], &inout->tft_data[5]);

    send_item_if_nonzero_and_clear(ENUM_WIRELESS_CHARGE_LOW_3,
                                   &inout->wireless_charge_code[0], &inout->wireless_charge_code[1], &inout->wireless_charge_code[2]);
    send_item_if_nonzero_and_clear(ENUM_WIRELESS_CHARGE_HIGH_3,
                                   &inout->wireless_charge_code[3], &inout->wireless_charge_code[4], &inout->wireless_charge_code[5]);

    /* 路灯：两个 1位 */
    send_item_u8_if_nonzero_and_clear(ENUM_STREET_LIGHT_INIT_LEVEL,  &inout->street_light.init_level);
    send_item_u8_if_nonzero_and_clear(ENUM_STREET_LIGHT_FINAL_LEVEL, &inout->street_light.final_level);

    /* 交通灯（三位） */
    send_item_if_nonzero_and_clear(ENUM_TRAFFIC_LIGHT_ABC_COLOR,
                                   &inout->traffic_light_abc[0], &inout->traffic_light_abc[1], &inout->traffic_light_abc[2]);

    /* 立体显示 4组（三位） */
    send_item_if_nonzero_and_clear(ENUM_3D_DISPLAY_DATA1,
                                   &inout->display_3d[0], &inout->display_3d[1], &inout->display_3d[2]);
    send_item_if_nonzero_and_clear(ENUM_3D_DISPLAY_DATA2,
                                   &inout->display_3d[3], &inout->display_3d[4], &inout->display_3d[5]);
    send_item_if_nonzero_and_clear(ENUM_3D_DISPLAY_DATA3,
                                   &inout->display_3d[6], &inout->display_3d[7], &inout->display_3d[8]);
    send_item_if_nonzero_and_clear(ENUM_3D_DISPLAY_DATA4,
                                   &inout->display_3d[9], &inout->display_3d[10], &inout->display_3d[11]);

    /* 2位数据（b0/b1，b2补0） */
    send_item_u16bytes_if_nonzero_and_clear(ENUM_BUS_STOP_TEMPERATURE, inout->bus_stop_temp);
    send_item_u16bytes_if_nonzero_and_clear(ENUM_GARAGE_AB_FLOOR,      inout->garage_ab_floor);

    /* 3位数据 */
    send_item_if_nonzero_and_clear(ENUM_DISTANCE_MEASURE,
                                   &inout->distance[0], &inout->distance[1], &inout->distance[2]);

    send_item_if_nonzero_and_clear(ENUM_GARAGE_INIT_COORD,
                                   &inout->garage_init_coord[0], &inout->garage_init_coord[1], &inout->garage_init_coord[2]);

    /* 随机路线 4组（三位） */
    send_item_if_nonzero_and_clear(ENUM_RANDOM_ROUTE_POINT1,
                                   &inout->random_route[0], &inout->random_route[1], &inout->random_route[2]);
    send_item_if_nonzero_and_clear(ENUM_RANDOM_ROUTE_POINT2,
                                   &inout->random_route[3], &inout->random_route[4], &inout->random_route[5]);
    send_item_if_nonzero_and_clear(ENUM_RANDOM_ROUTE_POINT3,
                                   &inout->random_route[6], &inout->random_route[7], &inout->random_route[8]);
    send_item_if_nonzero_and_clear(ENUM_RANDOM_ROUTE_POINT4,
                                   &inout->random_route[9], &inout->random_route[10], &inout->random_route[11]);

    bool rfid_any = false;
    for (int i=0;i<16;i++) if (inout->rfid[i] != 0) { rfid_any = true; break; }
    if (rfid_any) {
        send_frame_repeat(ENUM_RFID_DATA1, inout->rfid[0],  inout->rfid[1],  inout->rfid[2]);
        send_frame_repeat(ENUM_RFID_DATA2, inout->rfid[3],  inout->rfid[4],  inout->rfid[5]);
        send_frame_repeat(ENUM_RFID_DATA3, inout->rfid[6],  inout->rfid[7],  inout->rfid[8]);
        send_frame_repeat(ENUM_RFID_DATA4, inout->rfid[9],  inout->rfid[10], inout->rfid[11]);
        send_frame_repeat(ENUM_RFID_DATA5, inout->rfid[12], inout->rfid[13], inout->rfid[14]);
        send_frame_repeat(ENUM_RFID_DATA6, inout->rfid[15], 0, 0);
    }
    memset(inout->rfid, 0, 16);


    return true;
}

/* ===================== WAIT WHOLE STRUCT ===================== */
/* ✅ 规则：
 * - 收到就填充 out（不清零 out 的历史值会更危险，所以这里先 memset 0）
 * - 只要收到至少1帧有效数据，就返回 true
 * - 并且：收到后如果一段时间没新帧（RX_IDLE_END_MS），就提前结束返回 true（不必等满 timeout）
 */
bool WaitData(ProtocolData_t *out, uint32_t timeout_ms)
{
    if (!out) return false;

    memset(out, 0, sizeof(*out));

    bool received_any = false;
    uint32_t start_ms = get_ms();
    uint32_t last_rx_ms = start_ms;

    uint8_t buf[8];

    while (true) {

        if (!received_any) {
            if (time_reached(start_ms, timeout_ms)) return false;
        } else {
            if (time_reached(last_rx_ms, RX_IDLE_END_MS)) return true;
            if (time_reached(start_ms, timeout_ms)) return true; /* 总超时也结束 */
        }

        if (ZigbeeRead8(buf)) {
            if (!frame_valid(buf)) continue;

            uint8_t id = buf[2];

            /* 只处理我们的 CarDataEnum（0x09 ~ ENUM_DATA_MAX-1） */
            if (id < (uint8_t)ENUM_CAR_PLATE_FRONT_3 || id >= (uint8_t)ENUM_DATA_MAX) {
                continue;
            }

            switch ((CarDataEnum)id) {

            case ENUM_CAR_PLATE_FRONT_3:
                out->car_plate[0] = buf[3];
                out->car_plate[1] = buf[4];
                out->car_plate[2] = buf[5];
                break;

            case ENUM_CAR_PLATE_BACK_3:
                out->car_plate[3] = buf[3];
                out->car_plate[4] = buf[4];
                out->car_plate[5] = buf[5];
                break;

            case ENUM_DATE_YYYYMMDD:
                out->datetime.date[0] = buf[3];
                out->datetime.date[1] = buf[4];
                out->datetime.date[2] = buf[5];
                break;

            case ENUM_TIME_HHMMSS:
                out->datetime.time[0] = buf[3];
                out->datetime.time[1] = buf[4];
                out->datetime.time[2] = buf[5];
                break;

            case ENUM_GARAGE_FINAL_FLOOR:
                out->garage_final_floor = buf[3];
                break;

            case ENUM_TERRAIN_STATUS:
                out->terrain_status = buf[3];
                break;

            case ENUM_TERRAIN_POSITION:
                out->terrain_position = buf[3];
                break;

            case ENUM_VOICE_BROADCAST:
                out->voice_broadcast = buf[3];
                break;

            case ENUM_BEACON_CODE_LOW_3:
                out->beacon_code[0] = buf[3];
                out->beacon_code[1] = buf[4];
                out->beacon_code[2] = buf[5];
                break;

            case ENUM_BEACON_CODE_HIGH_3:
                out->beacon_code[3] = buf[3];
                out->beacon_code[4] = buf[4];
                out->beacon_code[5] = buf[5];
                break;

            case ENUM_TFT_DATA_LOW_3:
                out->tft_data[0] = buf[3];
                out->tft_data[1] = buf[4];
                out->tft_data[2] = buf[5];
                break;

            case ENUM_TFT_DATA_HIGH_3:
                out->tft_data[3] = buf[3];
                out->tft_data[4] = buf[4];
                out->tft_data[5] = buf[5];
                break;

            case ENUM_WIRELESS_CHARGE_LOW_3:
                out->wireless_charge_code[0] = buf[3];
                out->wireless_charge_code[1] = buf[4];
                out->wireless_charge_code[2] = buf[5];
                break;

            case ENUM_WIRELESS_CHARGE_HIGH_3:
                out->wireless_charge_code[3] = buf[3];
                out->wireless_charge_code[4] = buf[4];
                out->wireless_charge_code[5] = buf[5];
                break;

            case ENUM_STREET_LIGHT_INIT_LEVEL:
                out->street_light.init_level = buf[3];
                break;

            case ENUM_STREET_LIGHT_FINAL_LEVEL:
                out->street_light.final_level = buf[3];
                break;

            case ENUM_TRAFFIC_LIGHT_ABC_COLOR:
                out->traffic_light_abc[0] = buf[3];
                out->traffic_light_abc[1] = buf[4];
                out->traffic_light_abc[2] = buf[5];
                break;

            case ENUM_3D_DISPLAY_DATA1:
                out->display_3d[0] = buf[3];
                out->display_3d[1] = buf[4];
                out->display_3d[2] = buf[5];
                break;

            case ENUM_3D_DISPLAY_DATA2:
                out->display_3d[3] = buf[3];
                out->display_3d[4] = buf[4];
                out->display_3d[5] = buf[5];
                break;

            case ENUM_3D_DISPLAY_DATA3:
                out->display_3d[6] = buf[3];
                out->display_3d[7] = buf[4];
                out->display_3d[8] = buf[5];
                break;

            case ENUM_3D_DISPLAY_DATA4:
                out->display_3d[9] = buf[3];
                out->display_3d[10] = buf[4];
                out->display_3d[11] = buf[5];
                break;

            case ENUM_BUS_STOP_TEMPERATURE:
                out->bus_stop_temp[0] = buf[3];
                out->bus_stop_temp[1] = buf[4];
                break;

            case ENUM_GARAGE_AB_FLOOR:
                out->garage_ab_floor[0] = buf[3];
                out->garage_ab_floor[1] = buf[4];
                break;

            case ENUM_DISTANCE_MEASURE:
                out->distance[0] = buf[3];
                out->distance[1] = buf[4];
                out->distance[2] = buf[5];
                break;

            case ENUM_GARAGE_INIT_COORD:
                out->garage_init_coord[0] = buf[3];
                out->garage_init_coord[1] = buf[4];
                out->garage_init_coord[2] = buf[5];
                break;

            case ENUM_RANDOM_ROUTE_POINT1:
                out->random_route[0] = buf[3];
                out->random_route[1] = buf[4];
                out->random_route[2] = buf[5];
                break;

            case ENUM_RANDOM_ROUTE_POINT2:
                out->random_route[3] = buf[3];
                out->random_route[4] = buf[4];
                out->random_route[5] = buf[5];
                break;

            case ENUM_RANDOM_ROUTE_POINT3:
                out->random_route[6] = buf[3];
                out->random_route[7] = buf[4];
                out->random_route[8] = buf[5];
                break;

            case ENUM_RANDOM_ROUTE_POINT4:
                out->random_route[9] = buf[3];
                out->random_route[10] = buf[4];
                out->random_route[11] = buf[5];
                break;
            case ENUM_RFID_DATA1:
                out->rfid[0] = buf[3];
                out->rfid[1] = buf[4];
                out->rfid[2] = buf[5];
                break;
            case ENUM_RFID_DATA2:
                out->rfid[3] = buf[3];
                out->rfid[4] = buf[4];
                out->rfid[5] = buf[5];
                break;
            case ENUM_RFID_DATA3:
                out->rfid[6] = buf[3];
                out->rfid[7] = buf[4];
                out->rfid[8] = buf[5];
                break;
            case ENUM_RFID_DATA4:
                out->rfid[9] = buf[3];
                out->rfid[10] = buf[4];
                out->rfid[11] = buf[5];
                break;
            case ENUM_RFID_DATA5:
                out->rfid[12] = buf[3];
                out->rfid[13] = buf[4];
                out->rfid[14] = buf[5];
                break;
            case ENUM_RFID_DATA6:
                out->rfid[15] = buf[3];
                break;
            default:
                break;
            }

            received_any = true;
            last_rx_ms = get_ms();

        } else {
            delay_ms(1);
        }
    }
}
