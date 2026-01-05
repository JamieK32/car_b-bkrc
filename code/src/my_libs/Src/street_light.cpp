#include "street_light.hpp"
#include "infrare.h"
#include "BH1750.h"
#include "Drive.h"
#include <stdint.h>
#include <stdbool.h>
#include "log.hpp"

#define STREET_GEAR_MIN     1u
#define STREET_GEAR_MAX     4u
#define STREET_GEAR_CNT     4u
#define IR_CMD_LEN          4u
#define LUX_DELTA           1u   // 亮度抖动阈值(按现场调)

static uint8_t g_streetGear = 1u;     // 当前真实挡位(1~4)
static bool    g_gearValid  = false;  // 是否已通过 BH1750 校准


static inline void StreetLight_SendPlus(uint8_t step) // step: 1/2/3
{
    const uint8_t *cmd =
        (step == 1) ? Drive.Light_plus1 :
        (step == 2) ? Drive.Light_plus2 :
                      Drive.Light_plus3;
    Infrare.Transmition((uint8_t*)cmd, IR_CMD_LEN);
}

static inline void StreetLight_UpdateGear(uint8_t step)
{
    // 1~4 循环加 step
    g_streetGear = (uint8_t)(((g_streetGear - 1u + step) % STREET_GEAR_CNT) + 1u);
}

static uint16_t ReadLuxAvg(uint8_t n, uint16_t gap_ms)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < n; i++) {
        sum += (uint16_t)BH1750.ReadLightLevel();
        delay(gap_ms);
    }
    return (uint16_t)(sum / n);
}

// 校准：转一圈采样4次，找最亮的位置
static bool StreetLight_Calibrate(void)
{
    uint16_t lux[4];

    for (uint8_t k = 0; k < 4; k++) {
        // 采样当前挡位亮度
        delay(1000);                       // 等稳定(按现场调)
        lux[k] = ReadLuxAvg(5, 60);        // 5次均值

        // 前进到下一个挡位（最后一次其实不需要，但写着更统一）
        StreetLight_SendPlus(1);
    }

    // 找最亮的一档索引 idxBright
    uint8_t idxBright = 0;
    for (uint8_t i = 1; i < 4; i++) {
        if (lux[i] > lux[idxBright]) idxBright = i;
    }

    // 关键：idxBright 表示“在这次扫描中，第 idxBright 次采样时最亮”
    // 如果假设挡位4最亮，那么 idxBright 对应挡位4
    // 扫描过程中挡位每次 +1，所以可以反推出“第一次采样时的挡位”
    //
    // 若最亮=挡位4，则：
    // 初始挡位 = 4 - idxBright（在1~4循环里）
    uint8_t initial = (uint8_t)(4u - idxBright);
    if (initial == 0) initial = 4;

    // ⚠️ 注意：上面扫完后灯已经被你 +1 了 4 次（回到初始挡位）
    g_streetGear = initial;
    g_gearValid = true;
    return true;
}


static inline void StreetLight_EnsureCalibrated(void)
{
    if (!g_gearValid) {
        (void)StreetLight_Calibrate();
    }
}

/**
 * 加挡：add=1/2/3（循环 1~4）
 */
void Infrared_Street_Light_Plus(uint8_t add)
{
    if (add < 1u || add > 3u) return;

    StreetLight_EnsureCalibrated();

    StreetLight_SendPlus(add);
    StreetLight_UpdateGear(add);
}

/**
 * 设定到目标挡位：gear=1~4
 * 会自动发最少的 +1/+2/+3 命令达到目标
 */
void Infrared_Street_Light_Set(uint8_t gear)
{
    if (gear < STREET_GEAR_MIN || gear > STREET_GEAR_MAX) return;

    StreetLight_EnsureCalibrated();

    uint8_t steps = (uint8_t)((gear + STREET_GEAR_CNT - g_streetGear) % STREET_GEAR_CNT);
    if (steps == 0) return;

    // 用 +3/+2/+1 拼最少次数
    if (steps == 3)      Infrared_Street_Light_Plus(3);
    else if (steps == 2) Infrared_Street_Light_Plus(2);
    else                 Infrared_Street_Light_Plus(1);
}


uint8_t Infrared_Street_Light_GetGear(void)
{
    StreetLight_EnsureCalibrated();  // 如果未校准，会跑 BH1750 校准
    return g_streetGear;
}