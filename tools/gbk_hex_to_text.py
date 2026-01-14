#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
gbk_hex_to_text.py
Convert GBK-encoded hex bytes to Chinese text.

Examples:
  python gbk_hex_to_text.py "C4 E3 BA C3"
  python gbk_hex_to_text.py "username: C4 E3 BA C3 00 00"
  python gbk_hex_to_text.py --strip-zero "C4 E3 BA C3 00 00"
"""

import argparse
import re
from typing import List

HEX_BYTE_RE = re.compile(r'(?i)\b[0-9a-f]{2}\b')

def extract_hex_bytes(s: str) -> List[int]:
    """Extract all 2-hex-digit tokens (00..FF) from a string."""
    return [int(x, 16) for x in HEX_BYTE_RE.findall(s)]

def bytes_to_text(b: bytes, encoding: str, errors: str) -> str:
    return b.decode(encoding, errors=errors)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input", nargs="*", help="Hex bytes string (can include other text). If omitted, read stdin.")
    ap.add_argument("--encoding", default="gbk", help="Text encoding (default: gbk).")
    ap.add_argument("--errors", default="strict", choices=["strict", "replace", "ignore"],
                    help="Decode error handling (default: strict).")
    ap.add_argument("--strip-zero", action="store_true",
                    help="Strip trailing 0x00 bytes (common for fixed-size C fields).")
    args = ap.parse_args()

    if args.input:
        raw = " ".join(args.input)
    else:
        raw = input().strip()

    data = extract_hex_bytes(raw)
    if not data:
        print("No hex bytes found. Please input like: 'C4 E3 BA C3' or include such bytes in text.")
        return

    b = bytes(data)

    if args.strip_zero:
        b = b.rstrip(b"\x00")

    try:
        text = bytes_to_text(b, args.encoding, args.errors)
    except UnicodeDecodeError as e:
        print(f"Decode failed: {e}")
        print("Tip: try --errors replace  or  --strip-zero")
        return

    # Print results
    print("HEX BYTES:", " ".join(f"{x:02X}" for x in b))
    print("TEXT:", text)

if __name__ == "__main__":
    main()
