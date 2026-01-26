# 16-bit line tracking lookup table generator (enhanced)
# - Comments include 16-bit binary
# - More pattern modes: adjacent runs, separated pairs, 101 triplets, special masks
# - Generates both BLACK (1=active) and WHITE (inverted) entries

from dataclasses import dataclass
from typing import List, Tuple, Dict, Optional

BITS = 16
FULL = (1 << BITS) - 1

@dataclass(frozen=True)
class Entry:
    mask: int
    offset: float
    desc: str

def bits_bin(mask: int, bits: int = BITS) -> str:
    return "0b" + format(mask & ((1 << bits) - 1), f"0{bits}b")

def fmt_hex(mask: int, bits: int = BITS) -> str:
    width = (bits + 3) // 4
    return f"0x{mask & ((1<<bits)-1):0{width}X}"

def centroid_offset(mask: int, bits: int = BITS, scale: float = 1.0) -> float:
    """
    Offset by centroid of active bits relative to center.
    bit index: 0..15 (0=left, 15=right)  ——如果你的位序反了，可在最后做bit镜像
    """
    idxs = [i for i in range(bits) if (mask >> i) & 1]
    if not idxs:
        return 0.0
    centroid = sum(idxs) / len(idxs)
    center = (bits - 1) / 2.0  # 7.5
    return (centroid - center) * scale

def gen_adjacent_runs(bits: int = BITS, max_run: int = 8) -> List[Tuple[int, str]]:
    """111.. patterns"""
    out = []
    for run in range(1, max_run + 1):
        for start in range(0, bits - run + 1):
            mask = ((1 << run) - 1) << start
            out.append((mask, f"{run}adj idx[{start}..{start+run-1}]"))
    return out

def gen_separated_pairs(bits: int = BITS, max_gap: int = 3) -> List[Tuple[int, str]]:
    """Two bits with gap: 1...1"""
    out = []
    for gap in range(1, max_gap + 1):
        for i in range(0, bits - (gap + 1)):
            j = i + gap + 1
            mask = (1 << i) | (1 << j)
            out.append((mask, f"pair gap{gap} idx[{i},{j}]"))
    return out

def gen_101_triplets(bits: int = BITS) -> List[Tuple[int, str]]:
    """101 patterns: bits i and i+2 set, i+1 clear"""
    out = []
    for i in range(0, bits - 2):
        mask = (1 << i) | (1 << (i + 2))
        out.append((mask, f"triplet 101 idx[{i},{i+2}]"))
    return out

def gen_center_focus(bits: int = BITS) -> List[Tuple[int, str]]:
    """Some extra common center combos to make tuning easy"""
    c0, c1 = 7, 8  # center two bits around 7.5
    patterns = [
        (1 << c0, "CENTER single left-of-center"),
        (1 << c1, "CENTER single right-of-center"),
        ((1 << c0) | (1 << c1), "CENTER double center"),
        ((1 << (c0-1)) | (1 << c0) | (1 << c1), "CENTER 3bits leaning-left"),
        ((1 << c0) | (1 << c1) | (1 << (c1+1)), "CENTER 3bits leaning-right"),
        ((1 << (c0-2)) | (1 << (c0-1)) | (1 << c0) | (1 << c1), "CENTER 4bits more-left"),
        ((1 << c0) | (1 << c1) | (1 << (c1+1)) | (1 << (c1+2)), "CENTER 4bits more-right"),
    ]
    # filter valid
    out = []
    for m, d in patterns:
        if 0 <= m <= ((1 << bits) - 1):
            out.append((m, d))
    return out

def unique_by_mask(items: List[Tuple[int, str]]) -> List[Tuple[int, str]]:
    """Keep first occurrence per mask."""
    seen = set()
    out = []
    for m, d in items:
        m &= FULL
        if m in seen:
            continue
        seen.add(m)
        out.append((m, d))
    return out

def generate_entries(scale: float = 1.0,
                     max_run: int = 8,
                     max_gap: int = 3,
                     include_white: bool = True) -> List[Entry]:
    patterns: List[Tuple[int, str]] = []

    # 1) adjacent runs (主要)
    patterns += gen_adjacent_runs(max_run=max_run)

    # 2) separated pairs (弯道/断线/噪声时很常见)
    patterns += gen_separated_pairs(max_gap=max_gap)

    # 3) 101 triplets (中间空一格)
    patterns += gen_101_triplets()

    # 4) some center-focused combos
    patterns += gen_center_focus()

    # 5) specials
    patterns += [
        (0x0000, "SPECIAL none active"),
        (0xFFFF, "SPECIAL all active"),
        (0x00FF, "SPECIAL low 8 active"),
        (0xFF00, "SPECIAL high 8 active"),
        (0x0FFF, "SPECIAL low 12 active"),
        (0xFFF0, "SPECIAL high 12 active"),
    ]

    patterns = unique_by_mask(patterns)

    entries: Dict[int, Entry] = {}

    def add(mask: int, desc: str, tag: str):
        m = mask & FULL
        off = centroid_offset(m, bits=BITS, scale=scale)
        # desc里带上 BLACK/WHITE 标识
        full_desc = f"{tag} {desc}"
        # 去重：同mask只保留第一条（你也可以改成“保留offset绝对值更小”那种策略）
        if m not in entries:
            entries[m] = Entry(m, off, full_desc)

    # BLACK
    for m, d in patterns:
        add(m, d, "BLACK")

    # WHITE (invert)
    if include_white:
        for m, d in patterns:
            inv = (~m) & FULL
            add(inv, f"inv({d})", "WHITE")

    # 输出顺序：先按 active bit 数，再按 mask 值
    def bitcount(x: int) -> int:
        return bin(x & FULL).count("1")

    out = sorted(entries.values(), key=lambda e: (bitcount(e.mask), e.mask))
    return out

def emit_c_table(entries: List[Entry], name: str = "lookup_table16"):
    print("// Auto-generated 16-bit tracking lookup table (enhanced)")
    print("typedef struct {")
    print("    uint16_t input;")
    print("    float    output;")
    print("} lookup16_table_t;")
    print()
    print(f"static const lookup16_table_t {name}[] = {{")
    for e in entries:
        b = bits_bin(e.mask, BITS)
        hx = fmt_hex(e.mask, BITS)
        print(f"    {{{hx}, {e.offset:.3f}f}}, // {b}  {e.desc}")
    print("};")

if __name__ == "__main__":
    # 你可以按需要调整：
    # scale   : 偏移量步长（越大转向越激进）
    # max_run : 相邻触发长度覆盖到多少（建议 8 或 10）
    # max_gap : 两点分开触发的最大间隔（建议 3）
    entries = generate_entries(scale=1.0, max_run=8, max_gap=3, include_white=True)
    emit_c_table(entries, name="lookup_table16")
