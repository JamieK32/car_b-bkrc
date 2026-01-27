#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string.h>

#include "lsm6dsv16x.h"
#include "lsm6dsv16x_reg.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef float_t
    typedef float float_t;
#endif

#define BOOT_TIME      (10)

// 默认 0x6B；如果你 WHOAMI 读不到，试试 0x6A
#define LSM6DSV16X_ADDR (0x6A)

// ---------------------- 全局变量（和你原来一致） ----------------------
static uint8_t whoamI;
static lsm6dsv16x_fifo_sflp_raw_t fifo_sflp;
static float lsm6dsv16x_yaw_offset = 0.0f;
static float lsm6dsv16x_yaw_raw = 0.0f;


lsm6dsv16x_fifo_status_t fifo_status;
stmdev_ctx_t dev_ctx;
lsm6dsv16x_reset_t rst;
lsm6dsv16x_pin_int_route_t pin_int;

float_t lsm6dsv16x_quat[4];
short   lsm6dsv16x_gyro[3], lsm6dsv16x_accel[3];

// ---------------------- Wire 读写封装（给 ST 驱动用） ----------------------
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay(uint32_t ms);

// 角度归一化到 [-180, 180)
static float normalize_deg(float a)
{
  while (a >= 180.0f) a -= 360.0f;
  while (a <  -180.0f) a += 360.0f;
  return a;
}

// 四元数转欧拉角：q = [x, y, z, w]
static void quat_to_euler(const float q[4], float *roll, float *pitch, float *yaw)
{
  float x = q[0], yv = q[1], z = q[2], w = q[3];

  float sinr_cosp = 2.0f * (w * x + yv * z);
  float cosr_cosp = 1.0f - 2.0f * (x * x + yv * yv);
  *roll = atan2f(sinr_cosp, cosr_cosp);

  float sinp = 2.0f * (w * yv - z * x);
  if (fabsf(sinp) >= 1.0f) *pitch = copysignf(M_PI / 2.0f, sinp);
  else *pitch = asinf(sinp);

  float siny_cosp = 2.0f * (w * z + x * yv);
  float cosy_cosp = 1.0f - 2.0f * (yv * yv + z * z);
  *yaw = atan2f(siny_cosp, cosy_cosp);

  *roll  *= RAD_TO_DEG;
  *pitch *= RAD_TO_DEG;
  *yaw   *= RAD_TO_DEG;
}

static float_t npy_half_to_float(uint16_t h)
{
  union { float_t ret; uint32_t retbits; } conv;
  conv.retbits = lsm6dsv16x_from_f16_to_f32(h);
  return conv.ret;
}

static void sflp2q(float_t quat[4], uint8_t raw[6])
{
  float_t sumsq = 0;
  uint16_t sflp[3];

  memcpy(&sflp[0], &raw[0], 2);
  memcpy(&sflp[1], &raw[2], 2);
  memcpy(&sflp[2], &raw[4], 2);

  quat[0] = npy_half_to_float(sflp[0]);
  quat[1] = npy_half_to_float(sflp[1]);
  quat[2] = npy_half_to_float(sflp[2]);

  for (uint8_t i = 0; i < 3; i++) sumsq += quat[i] * quat[i];

  if (sumsq > 1.0f) {
    float_t n = sqrtf(sumsq);
    quat[0] /= n; quat[1] /= n; quat[2] /= n;
    sumsq = 1.0f;
  }
  quat[3] = sqrtf(1.0f - sumsq);
}

// ---------------------- 初始化 ----------------------
void LSM6DSV16X_Init()
{
  // 硬件 I2C 初始化：Mega2560 SDA=20 SCL=21（Wire 默认就是）
  Wire.begin();
  Wire.setClock(100000);        // 可选：400kHz
#if defined(TWBR) || defined(WIRE_HAS_TIMEOUT)
  // 有些核心支持超时，防死锁（可选）
  Wire.setWireTimeout(3000, true);
#endif

  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg  = platform_read;
  dev_ctx.mdelay    = platform_delay;

  platform_delay(BOOT_TIME);

  // 重试读 WHOAMI
  const uint32_t max_retry_count = 100;
  uint32_t retry_count = 0;

  do {
    lsm6dsv16x_device_id_get(&dev_ctx, &whoamI);
    if (whoamI == LSM6DSV16X_ID) break;
    retry_count++;
    platform_delay(10);
  } while (retry_count < max_retry_count);

  if (whoamI != LSM6DSV16X_ID) {
    // 读不到：常见原因=地址不对(0x6A/0x6B)、供电/电平、上拉、接线
    return;
  }

  // 复位
  lsm6dsv16x_reset_set(&dev_ctx, LSM6DSV16X_GLOBAL_RST);
  do { lsm6dsv16x_reset_get(&dev_ctx, &rst); }
  while (rst != LSM6DSV16X_READY);

  lsm6dsv16x_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

  lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_4g);
  lsm6dsv16x_gy_full_scale_set(&dev_ctx, LSM6DSV16X_2000dps);

  // FIFO batch
  fifo_sflp.game_rotation = 1;
  fifo_sflp.gravity = 0;
  fifo_sflp.gbias = 0;
  lsm6dsv16x_fifo_sflp_batch_set(&dev_ctx, fifo_sflp);
  lsm6dsv16x_fifo_xl_batch_set(&dev_ctx, LSM6DSV16X_XL_BATCHED_AT_60Hz);
  lsm6dsv16x_fifo_gy_batch_set(&dev_ctx, LSM6DSV16X_GY_BATCHED_AT_60Hz);

  lsm6dsv16x_fifo_mode_set(&dev_ctx, LSM6DSV16X_STREAM_MODE);

  // ODR
  lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_60Hz);
  lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_60Hz);
  lsm6dsv16x_sflp_data_rate_set(&dev_ctx, LSM6DSV16X_SFLP_60Hz);
  lsm6dsv16x_sflp_game_rotation_set(&dev_ctx, PROPERTY_ENABLE);

  pin_int.drdy_xl = PROPERTY_ENABLE;
  lsm6dsv16x_pin_int2_route_set(&dev_ctx, &pin_int);
  lsm6dsv16x_data_ready_mode_set(&dev_ctx, LSM6DSV16X_DRDY_PULSED);

  // 取一帧四元数做 yaw 零偏
  const uint32_t timeout_ms = 200;
  uint32_t waited = 0;
  lsm6dsv16x_fifo_status_t st;

  while (waited < timeout_ms) {
    lsm6dsv16x_fifo_status_get(&dev_ctx, &st);
    if (st.fifo_level > 0) {
      lsm6dsv16x_fifo_out_raw_t f_data;
      lsm6dsv16x_fifo_out_raw_get(&dev_ctx, &f_data);

      if (f_data.tag == LSM6DSV16X_SFLP_GAME_ROTATION_VECTOR_TAG) {
        sflp2q(lsm6dsv16x_quat, &f_data.data[0]);
        float r, p, y;
        quat_to_euler(lsm6dsv16x_quat, &r, &p, &y);
        lsm6dsv16x_yaw_offset = y;
        break;
      }
    } else {
      platform_delay(5);
      waited += 5;
    }
  }
}

// ---------------------- 读取 FIFO 并更新 yaw/pitch/roll ----------------------
void Read_LSM6DSV16X(euler_angles_t *angle)
{
  uint16_t num;
  lsm6dsv16x_fifo_status_t st;

  lsm6dsv16x_fifo_status_get(&dev_ctx, &st);
  num = st.fifo_level;

  while (num--) {
    lsm6dsv16x_fifo_out_raw_t f_data;
    lsm6dsv16x_fifo_out_raw_get(&dev_ctx, &f_data);

    int16_t datax, datay, dataz;
    memcpy(&datax, &f_data.data[0], 2);
    memcpy(&datay, &f_data.data[2], 2);
    memcpy(&dataz, &f_data.data[4], 2);

    switch (f_data.tag) {
      case LSM6DSV16X_SFLP_GAME_ROTATION_VECTOR_TAG:
        sflp2q(lsm6dsv16x_quat, &f_data.data[0]);
        quat_to_euler(lsm6dsv16x_quat, &angle->roll, &angle->pitch, &lsm6dsv16x_yaw_raw);
        angle->yaw = normalize_deg(lsm6dsv16x_yaw_raw - lsm6dsv16x_yaw_offset);
        break;

      case LSM6DSV16X_XL_NC_TAG:
        lsm6dsv16x_accel[0] = lsm6dsv16x_from_fs4_to_mg(datax);
        lsm6dsv16x_accel[1] = lsm6dsv16x_from_fs4_to_mg(datay);
        lsm6dsv16x_accel[2] = lsm6dsv16x_from_fs4_to_mg(dataz);
        break;

      case LSM6DSV16X_GY_NC_TAG:
        lsm6dsv16x_gyro[0] = lsm6dsv16x_from_fs2000_to_mdps(datax) / 1000;
        lsm6dsv16x_gyro[1] = lsm6dsv16x_from_fs2000_to_mdps(datay) / 1000;
        lsm6dsv16x_gyro[2] = lsm6dsv16x_from_fs2000_to_mdps(dataz) / 1000;
        break;

      default:
        break;
    }
  }
}

bool LSM6DSV16X_RezeroYaw_Fresh(uint32_t timeout_ms )
{
  uint32_t waited = 0;
  lsm6dsv16x_fifo_status_t st;

  while (waited < timeout_ms) {
    lsm6dsv16x_fifo_status_get(&dev_ctx, &st);
    if (st.fifo_level > 0) {
      lsm6dsv16x_fifo_out_raw_t f_data;
      lsm6dsv16x_fifo_out_raw_get(&dev_ctx, &f_data);

      if (f_data.tag == LSM6DSV16X_SFLP_GAME_ROTATION_VECTOR_TAG) {
        sflp2q(lsm6dsv16x_quat, &f_data.data[0]);
        float r, p, y;
        quat_to_euler(lsm6dsv16x_quat, &r, &p, &y);
        lsm6dsv16x_yaw_raw = y;
        lsm6dsv16x_yaw_offset = lsm6dsv16x_yaw_raw;   // 直接用当前值当零点
        return true;
      }
    } else {
      platform_delay(5);
      waited += 5;
    }
  }
  return false;
}


// ---------------------- platform 回调实现（Wire） ----------------------
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
  (void)handle;
  Wire.beginTransmission(LSM6DSV16X_ADDR);
  Wire.write(reg);
  for (uint16_t i = 0; i < len; i++) Wire.write(bufp[i]);
  uint8_t err = Wire.endTransmission(true);
  return (err == 0) ? 0 : -1;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  (void)handle;

  Wire.beginTransmission(LSM6DSV16X_ADDR);
  Wire.write(reg);
  uint8_t err = Wire.endTransmission(false);
  if (err != 0) return -1;

  uint16_t remaining = len;
  uint16_t offset = 0;

  while (remaining) {
    uint8_t chunk = remaining > 32 ? 32 : remaining;

    uint16_t got = Wire.requestFrom((int)LSM6DSV16X_ADDR, (int)chunk, (int)true);
    if (got != chunk) return -1;

    for (uint8_t i = 0; i < chunk; i++) {
      if (!Wire.available()) return -1;
      bufp[offset + i] = Wire.read();
    }

    offset += chunk;
    remaining -= chunk;
  }
  return 0;
}


static void platform_delay(uint32_t ms)
{
  delay(ms);
}
