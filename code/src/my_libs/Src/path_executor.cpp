// path_executor.cpp
// C风格写法的 CPP 文件：自动规划(以2为单位) -> 路径字符串 -> 动作数组 -> 顺序执行（相对角度版本）
// 新增：禁用一段路径 / 优先选择一段路径并带回调
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "car_controller.hpp"
#include "path_executor.h"

// ========================== 坐标系定义（与 NodeJS 一致：行 G->A，列 7->1） ==========================
static const char ROW_LABELS[7] = {'G','F','E','D','C','B','A'};
static const int  COL_LABELS[7] = {7,6,5,4,3,2,1};

static int row_index(char r) {
    r = (char)toupper((unsigned char)r);
    for (int i = 0; i < 7; i++) if (ROW_LABELS[i] == r) return i;
    return -1;
}
static int col_index(int c) {
    for (int i = 0; i < 7; i++) if (COL_LABELS[i] == c) return i;
    return -1;
}
static char row_from_index(int idx) {
    if (idx < 0 || idx >= 7) return '?';
    return ROW_LABELS[idx];
}

// ========================== 节点/解析 ==========================
typedef struct {
    char row;   // 'A'..'G'
    int  col;   // 1..7
    char raw[4];// "B6"
} node_t;

static node_t make_node(char row, int col) {
    node_t n;
    n.row = (char)toupper((unsigned char)row);
    n.col = col;
    n.raw[0] = n.row;
    n.raw[1] = (char)('0' + col);
    n.raw[2] = '\0';
    n.raw[3] = '\0';
    return n;
}

static int node_equal(const node_t* a, const node_t* b) {
    return (toupper((unsigned char)a->row) == toupper((unsigned char)b->row)) && (a->col == b->col);
}

static int parse_coord(const char* s, node_t* out) {
    if (!s || !out) return 0;
    if (!isalpha((unsigned char)s[0]) || !isdigit((unsigned char)s[1])) return 0;
    char r = (char)toupper((unsigned char)s[0]);
    int  c = s[1] - '0';
    if (row_index(r) < 0 || col_index(c) < 0) return 0;
    *out = make_node(r, c);
    return 1;
}

// 扫描字符串，提取类似 "B6" 的坐标序列：
// 支持： "B6D6D4F4F2" 或 "B7->B6->D6" 或 "B6, D6, D4"
static int parse_path(const char* data, node_t* out_nodes, int max_nodes) {
    int n = 0;
    if (!data || !out_nodes || max_nodes <= 0) return 0;

    const int L = (int)strlen(data);
    for (int i = 0; i < L && n < max_nodes; i++) {
        char ch = (char)toupper((unsigned char)data[i]);
        if (ch >= 'A' && ch <= 'G') {
            if (i + 1 < L && isdigit((unsigned char)data[i + 1])) {
                int col = data[i + 1] - '0';
                int ri = row_index(ch);
                int ci = col_index(col);
                if (ri >= 0 && ci >= 0) {
                    out_nodes[n] = make_node(ch, col);
                    n++;
                    i++; // consume digit
                }
            }
        }
    }
    return n;
}

// ========================== 方向/转角计算 ==========================
static dir_e dir_between(const node_t* a, const node_t* b) {
    int ax = col_index(a->col);
    int ay = row_index(a->row);
    int bx = col_index(b->col);
    int by = row_index(b->row);
    if (ax < 0 || ay < 0 || bx < 0 || by < 0) return DIR_INVALID;

    int dx = bx - ax; // x 向右为正（index变大）
    int dy = by - ay; // y 向下为正（index变大）

    if (dx != 0 && dy == 0) {
        return (dx > 0) ? DIR_RIGHT : DIR_LEFT;
    } else if (dy != 0 && dx == 0) {
        return (dy > 0) ? DIR_DOWN : DIR_UP;
    }
    return DIR_INVALID;
}

// dir编码：UP=0 RIGHT=1 DOWN=2 LEFT=3
// delta=(target-current+4)%4
// 1 => +90(右转), 3 => -90(左转), 2 => 180(掉头)
// 你已经做了“左右符号互换”，这里保持你的版本：
static int relative_turn_angle(dir_e current, dir_e target) {
    if (current == DIR_INVALID || target == DIR_INVALID) return 0;
    int delta = ((int)target - (int)current + 4) % 4;
    if (delta == 0) return 0;
    if (delta == 1) return -90;
    if (delta == 3) return 90;
    return 180;
}

// ========================== 段规则匹配工具 ==========================
static int coord_str_equal(const char* a, const char* b) {
    if (!a || !b) return 0;
    if (toupper((unsigned char)a[0]) != toupper((unsigned char)b[0])) return 0;
    return (a[1] == b[1]);
}

static int is_avoided_edge(const node_t* u, const node_t* v,
                           const path_avoid_seg_t* avoid_segs, int avoid_count) {
    if (!u || !v || !avoid_segs || avoid_count <= 0) return 0;
    char ur[4], vr[4];
    ur[0] = u->row; ur[1] = (char)('0' + u->col); ur[2] = '\0';
    vr[0] = v->row; vr[1] = (char)('0' + v->col); vr[2] = '\0';

    for (int i = 0; i < avoid_count; i++) {
        const path_avoid_seg_t* s = &avoid_segs[i];
        // 双向禁用
        if ((coord_str_equal(s->from, ur) && coord_str_equal(s->to, vr)) ||
            (coord_str_equal(s->from, vr) && coord_str_equal(s->to, ur))) {
            return 1;
        }
    }
    return 0;
}

static const path_mark_seg_t* find_mark_for_edge(const node_t* u, const node_t* v,
                                                 const path_mark_seg_t* mark_segs, int mark_count) {
    if (!u || !v || !mark_segs || mark_count <= 0) return NULL;

    char ur[4], vr[4];
    ur[0] = u->row; ur[1] = (char)('0' + u->col); ur[2] = '\0';
    vr[0] = v->row; vr[1] = (char)('0' + v->col); vr[2] = '\0';

    for (int i = 0; i < mark_count; i++) {
        const path_mark_seg_t* s = &mark_segs[i];
        // 这里默认“有向优先”：from->to 优先
        // 如果你希望无向也优先，可以把下面条件改成双向
        if (coord_str_equal(s->from, ur) && coord_str_equal(s->to, vr)) {
            return s;
        }
    }
    return NULL;
}

// ========================== 自动规划：以2为单位的邻接 ==========================
// 节点映射：row_idx 0..6 (G..A), col_val 1..7
// 邻居：row_idx +/-2 或 col_val +/-2
static int list_neighbors_step2(const node_t* cur, node_t* out_nb, int max_nb) {
    if (!cur || !out_nb || max_nb <= 0) return 0;
    int n = 0;

    int ry = row_index(cur->row); // 0..6
    int ci = col_index(cur->col); // 0..6 (对应 COL_LABELS: 7..1)
    if (ry < 0 || ci < 0) return 0;

    // 上/下：row_idx -2 / +2
    int ry_up = ry - 2;
    int ry_dn = ry + 2;
    if (ry_up >= 0) {
        out_nb[n++] = make_node(row_from_index(ry_up), cur->col);
        if (n >= max_nb) return n;
    }
    if (ry_dn <= 6) {
        out_nb[n++] = make_node(row_from_index(ry_dn), cur->col);
        if (n >= max_nb) return n;
    }

    // 左/右：col_idx +/-2（注意 col 值通过 COL_LABELS 映射）
    int c_left  = ci - 2;
    int c_right = ci + 2;
    if (c_left >= 0) {
        out_nb[n++] = make_node(cur->row, COL_LABELS[c_left]);
        if (n >= max_nb) return n;
    }
    if (c_right <= 6) {
        out_nb[n++] = make_node(cur->row, COL_LABELS[c_right]);
        if (n >= max_nb) return n;
    }

    return n;
}

// 0-1 BFS：
// - 普通边 cost=1
// - 被 mark 的边(from->to) cost=0（优先）
// - avoid 的边直接不可走
typedef struct {
    int rowi; // 0..6
    int coli; // 0..6
} key_t;

static key_t node_key(const node_t* n) {
    key_t k;
    k.rowi = row_index(n->row);
    k.coli = col_index(n->col);
    return k;
}

static int key_to_index(const key_t* k) {
    // row 0..6, col 0..6 => 7*7=49
    return k->rowi * 7 + k->coli;
}

static int index_valid(int idx) { return idx >= 0 && idx < 49; }

static node_t index_to_node(int idx) {
    int r = idx / 7; // 0..6
    int c = idx % 7; // 0..6
    return make_node(row_from_index(r), COL_LABELS[c]);
}

int Path_PlanPath(const char* start_coord,
                  const char* end_coord,
                  const path_avoid_seg_t* avoid_segs, int avoid_count,
                  const path_mark_seg_t*  mark_segs,  int mark_count,
                  char* out_path_data, int out_path_data_len)
{
    node_t start, goal;
    if (!parse_coord(start_coord, &start)) return 0;
    if (!parse_coord(end_coord, &goal)) return 0;

    // dist / prev
    // 使用 16-bit 安全值：最大最短路径 < 49
    const int16_t INF = 0x3fff;
    int16_t dist[49];
    int16_t prev[49]; // store previous node index
    for (int i = 0; i < 49; i++) { dist[i] = INF; prev[i] = -1; }

    // deque for 0-1 BFS
    int dq[49 * 4];
    int head = 49 * 2, tail = 49 * 2; // start in middle

    key_t ks = node_key(&start);
    key_t kg = node_key(&goal);
    int sidx = key_to_index(&ks);
    int gidx = key_to_index(&kg);
    if (!index_valid(sidx) || !index_valid(gidx)) return 0;

    dist[sidx] = 0;
    dq[tail++] = sidx;

    while (head != tail) {
        int uidx = dq[head++];

        if (uidx == gidx) break;

        // reconstruct node from uidx
        node_t u = index_to_node(uidx);

        node_t nbs[8];
        int nb_n = list_neighbors_step2(&u, nbs, 8);

        for (int i = 0; i < nb_n; i++) {
            node_t v = nbs[i];

            // avoid edge
            if (is_avoided_edge(&u, &v, avoid_segs, avoid_count)) {
                continue;
            }

            key_t kv = node_key(&v);
            int vidx = key_to_index(&kv);
            if (!index_valid(vidx)) {
                continue;
            }

            // cost: marked edge cost=0 else 1
            int w = 1;
            if (find_mark_for_edge(&u, &v, mark_segs, mark_count) != NULL) {
                w = 0;
            }

            if (dist[uidx] + w < dist[vidx]) {
                dist[vidx] = dist[uidx] + w;
                prev[vidx] = uidx;
                if (w == 0) {
                    dq[--head] = vidx;
                } else {
                    dq[tail++] = vidx;
                }
            }
        }
    }

    if (dist[gidx] == INF) {
        // 无路可走
        if (out_path_data && out_path_data_len > 0) out_path_data[0] = '\0';
        return 0;
    }

    // 回溯路径
    int path_idx[49];
    int pn = 0;
    for (int cur = gidx; cur != -1 && pn < 49; cur = prev[cur]) {
        path_idx[pn++] = cur;
        if (cur == sidx) break;
    }
    if (pn <= 0 || path_idx[pn-1] != sidx) return 0;

    // reverse => start..goal
    for (int i = 0; i < pn / 2; i++) {
        int t = path_idx[i];
        path_idx[i] = path_idx[pn - 1 - i];
        path_idx[pn - 1 - i] = t;
    }

    // 输出 path 字符串：如 "B4D4D2"
    if (out_path_data && out_path_data_len > 0) {
        out_path_data[0] = '\0';
        int used = 0;
        for (int i = 0; i < pn; i++) {
            int r = path_idx[i] / 7;
            int c = path_idx[i] % 7;
            if (used + 2 >= out_path_data_len) break;
            out_path_data[used++] = row_from_index(r);
            out_path_data[used++] = (char)('0' + COL_LABELS[c]);
            out_path_data[used] = '\0';
        }
    }

    return pn; // 节点数量
}

// ========================== 动作生成（原有 + 支持 mark 回调绑定） ==========================
// 生成动作：每一段开始前先转向（若需要），再 TrackToCross
int Path_BuildActions(const char* current_coord,
                      const char* path_data,
                      const plan_cfg_t* cfg,
                      action_t* out_actions,
                      int max_actions)
{
    if (!current_coord || !path_data || !cfg || !out_actions || max_actions <= 0) return 0;

    node_t nodes[MAX_NODES];
    int node_count = parse_path(path_data, nodes, MAX_NODES);

    // 解析 current_coord（例如 "B6"）
    node_t cur;
    if (!parse_coord(current_coord, &cur)) return 0;

    // 如果路径为空，或者路径首点不是当前点，则把 current 插到最前面
    node_t fixed_nodes[MAX_NODES];
    int fixed_count = 0;

    if (node_count <= 0) {
        fixed_nodes[0] = cur;
        fixed_count = 1;
    } else {
        if (!node_equal(&cur, &nodes[0])) {
            fixed_nodes[0] = cur;
            fixed_count = 1;
        }
        for (int i = 0; i < node_count && fixed_count < MAX_NODES; i++) {
            fixed_nodes[fixed_count++] = nodes[i];
        }
    }

    if (fixed_count < 2) return 0;

    int act_n = 0;
    // 不在开头强制转向，假设车头已对准第一段方向
    dir_e heading = DIR_INVALID;

    // 注意：此函数不知道 mark/avoid（为兼容保留）。如果你需要 mark/avoid，请用 Path_PlanAndBuildActions。
    for (int i = 0; i < fixed_count - 1; i++) {
        // 始终包含最后一段循迹

        dir_e seg_dir = dir_between(&fixed_nodes[i], &fixed_nodes[i + 1]);
        if (seg_dir == DIR_INVALID) continue;

        // 1) 首段不转向；后续段需要时再转向
        if (heading != DIR_INVALID && seg_dir != heading) {
            if (act_n < max_actions) {
                action_t a;
                a.state = STATE_TURN;
                a.dir   = seg_dir;
                a.speed = 0;
                a.angle = relative_turn_angle(heading, seg_dir);
                a.cb = NULL;
                out_actions[act_n++] = a;
            }
            heading = seg_dir;
        } else if (heading == DIR_INVALID) {
            heading = seg_dir;
        }

        // 2) 再循迹到路口
        if (act_n < max_actions) {
            action_t a;
            a.state = STATE_TRACK_TO_CROSS;
            a.dir   = heading;
            a.speed = cfg->default_speed;
            a.angle = 0;
            a.cb = NULL;
            out_actions[act_n++] = a;
        }
    }

    return act_n;
}

// ========================== 新增：自动规划 + 绑定 mark 回调 生成动作 ==========================
static int build_actions_with_marks_from_nodes(const node_t* nodes, int node_count,
                                               const plan_cfg_t* cfg,
                                               const path_mark_seg_t* mark_segs, int mark_count,
                                               action_t* out_actions, int max_actions)
{
    if (!nodes || node_count < 2 || !cfg || !out_actions || max_actions <= 0) return 0;

    int act_n = 0;
    // 不在开头强制转向，假设车头已对准第一段方向
    dir_e heading = DIR_INVALID;

    for (int i = 0; i < node_count - 1; i++) {
        // 始终包含最后一段循迹

        const node_t* u = &nodes[i];
        const node_t* v = &nodes[i + 1];

        dir_e seg_dir = dir_between(u, v);
        if (seg_dir == DIR_INVALID) continue;

        const path_mark_seg_t* mk = find_mark_for_edge(u, v, mark_segs, mark_count);
        path_seg_cb_t cb_turn  = NULL;
        path_seg_cb_t cb_track = NULL;
        if (mk) {
            if (mk->when == PATH_MARK_CALL_TURN_ONLY) {
                cb_turn = mk->cb;
            } else if (mk->when == PATH_MARK_CALL_TRACK_ONLY) {
                cb_track = mk->cb;
            }
        }

        // 1) 首段不转向；后续段需要时再转向
        if (heading != DIR_INVALID && seg_dir != heading) {
            if (act_n < max_actions) {
                action_t a;
                a.state = STATE_TURN;
                a.dir   = seg_dir;
                a.speed = 0;
                a.angle = relative_turn_angle(heading, seg_dir);
                a.cb = cb_turn;
                out_actions[act_n++] = a;
            }
            heading = seg_dir;
        } else if (heading == DIR_INVALID) {
            heading = seg_dir;
        }

        // 2) 再循迹到路口
        if (act_n < max_actions) {
            action_t a;
            a.state = STATE_TRACK_TO_CROSS;
            a.dir   = heading;
            a.speed = cfg->default_speed;
            a.angle = 0;
            a.cb = cb_track;
            out_actions[act_n++] = a;
        }
    }

    return act_n;
}

int Path_PlanAndBuildActions(const char* start_coord,
                             const char* end_coord,
                             const path_avoid_seg_t* avoid_segs, int avoid_count,
                             const path_mark_seg_t*  mark_segs,  int mark_count,
                             const plan_cfg_t* cfg,
                             action_t* out_actions, int max_actions,
                             char* out_path_data, int out_path_data_len)
{
    if (!start_coord || !end_coord || !cfg || !out_actions || max_actions <= 0) return 0;

    // 先规划得到 path 字符串
    char tmp_path[MAX_PATH_STR];
    char* path_buf = tmp_path;
    int path_len = (int)sizeof(tmp_path);

    if (out_path_data && out_path_data_len > 0) {
        path_buf = out_path_data;
        path_len = out_path_data_len;
    }

    int node_count = Path_PlanPath(start_coord, end_coord,
                                   avoid_segs, avoid_count,
                                   mark_segs, mark_count,
                                   path_buf, path_len);
    if (node_count < 2) {
        return 0;
    }

    // 把 path_buf 再 parse 成 nodes（你也可以直接在 PlanPath 输出 nodes，但为了接口简单复用 parse）
    node_t nodes[MAX_NODES];
    int n = parse_path(path_buf, nodes, MAX_NODES);
    if (n < 2) return 0;

    return build_actions_with_marks_from_nodes(nodes, n, cfg,
                                               mark_segs, mark_count,
                                               out_actions, max_actions);
}

// ========================== 执行器（新增：回调） ==========================
void Path_ExecuteActions(const action_t* actions, int count) {
    if (!actions || count <= 0) return;

    for (int i = 0; i < count; i++) {
        const action_t* a = &actions[i];

        switch (a->state) {
            case STATE_TURN:
                Car_Turn(a->angle);
                // 转向完成后再回调
                if (a->cb) {
                    a->cb();
                }
                break;

            case STATE_TRACK_TO_CROSS:
                // 直行前回调
                if (a->cb) {
                    a->cb();
                }
                Car_TrackToCrossTrackingBoard(a->speed);
                break;

            default:
                break;
        }
    }
}
