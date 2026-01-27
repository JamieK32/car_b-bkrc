// path_executor.h  (相对角度版本：新增自动规划/禁用边/优先边标记回调)
#ifndef PATH_EXECUTOR_H
#define PATH_EXECUTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// ===================== 动作定义 =====================
typedef enum {
    STATE_TURN = 0,            // 转向状态
    STATE_TRACK_TO_CROSS = 1   // 循迹到路口状态
} state_e;

typedef enum {
    DIR_UP = 0,
    DIR_RIGHT = 1,
    DIR_DOWN = 2,
    DIR_LEFT = 3,
    DIR_INVALID = 255
} dir_e;

// 段回调：
// - STATE_TURN：在 Car_Turn 执行完成后调用
// - STATE_TRACK_TO_CROSS：在 Car_TrackToCrossTrackingBoard 执行前调用
typedef void (*path_seg_cb_t)(void);

// mark 回调触发时机
typedef enum {
    PATH_MARK_CALL_TURN_ONLY = 0,  // 仅在转向动作前调用
    PATH_MARK_CALL_TRACK_ONLY = 1  // 仅在循迹动作前调用
} path_mark_call_e;

typedef struct {
    state_e  state;        // 状态
    dir_e    dir;          // 方向（转向目标/行进方向）
    uint8_t  speed;        // 速度（循迹用）
    int      angle;        // 转向角：-90 / +90 / 180 / 0

    // ===== 新增：段标记回调（可为空）=====
    path_seg_cb_t  cb;     // 段回调（可为空）
} action_t;

// ===================== 规划配置（简化） =====================
typedef struct {
    uint8_t default_speed;       // 默认循迹速度
} plan_cfg_t;

#ifndef MAX_NODES
#define MAX_NODES 128
#endif

#ifndef MAX_ACTIONS
#define MAX_ACTIONS 256
#endif

#ifndef MAX_SEG_RULES
#define MAX_SEG_RULES 32
#endif

#ifndef MAX_PATH_STR
#define MAX_PATH_STR 512
#endif

// ===================== 段规则 =====================
typedef struct {
    char from[4]; // "D4"
    char to[4];   // "D2"
} path_avoid_seg_t;

typedef struct {
    char from[4];          // "B4"
    char to[4];            // "D4"
    path_seg_cb_t cb;      // 道匝回调函数
    path_mark_call_e when; // 回调时机
} path_mark_seg_t;

// ===================== API（原有） =====================
// 生成动作数组：返回动作数量
// current_coord: "B6"
// path_data:     "B6D6D4F4F2" 或 "B6->D6->D4..."
int Path_BuildActions(const char* current_coord,
                      const char* path_data,
                      const plan_cfg_t* cfg,
                      action_t* out_actions,
                      int max_actions);

// 顺序执行动作数组（新增：支持 action 内回调）
void Path_ExecuteActions(const action_t* actions, int count);

// ===================== API（新增） =====================
// 1) 自动规划路径点（以2为单位：相邻点行/列差值为2），并输出 path 字符串（可传NULL）
// 返回：节点数量（>=2），失败返回0
int Path_PlanPath(const char* start_coord,
                  const char* end_coord,
                  const path_avoid_seg_t* avoid_segs, int avoid_count,
                  const path_mark_seg_t*  mark_segs,  int mark_count,
                  char* out_path_data, int out_path_data_len);

// 2) 自动规划 + 直接生成动作数组（推荐用这个）
// out_path_data 可传NULL；用于调试查看最终选的路径点
int Path_PlanAndBuildActions(const char* start_coord,
                             const char* end_coord,
                             const path_avoid_seg_t* avoid_segs, int avoid_count,
                             const path_mark_seg_t*  mark_segs,  int mark_count,
                             const plan_cfg_t* cfg,
                             action_t* out_actions, int max_actions,
                             char* out_path_data, int out_path_data_len);

#ifdef __cplusplus
}
#endif

#endif // PATH_EXECUTOR_H
