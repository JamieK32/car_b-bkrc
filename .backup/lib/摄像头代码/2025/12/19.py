class StrictValidator:
    @staticmethod
    def is_hex(s):
        if not s: return False
        for c in s:
            if not ((c >= '0' and c <= '9') or (c >= 'A' and c <= 'Z')):
                return False
        return True

    def is_num(s):
        if not s: return False
        for c in s:
            if not (c >= '0' and c <= '9'):
                return False
        return True

    def is_letter(s):
        if not s: return False
        for c in s:
            if not (c >= 'A' and c <= 'Z'):
                return False
        return True

    def parse_bracket_content(text, bracket_types):
        """
        text: 待解析字符串
        bracket_types: 需要提取的括号类型，如 ['<>', '[]', '||']
        """
        results = []

        # 构造 起始符 -> 结束符 映射
        bracket_map = {b[0]: b[1] for b in bracket_types}

        inside = False
        start_char = None
        end_char = None
        buffer = []

        for ch in text:
            # 进入指定类型的括号
            if not inside and ch in bracket_map:
                inside = True
                start_char = ch
                end_char = bracket_map[ch]
                buffer = []
                continue

            # 在括号内部
            if inside:
                if ch == end_char:
                    results.append(''.join(buffer))
                    inside = False
                    start_char = None
                    end_char = None
                else:
                    buffer.append(ch)

        return results
def parse_bracket_content(text, bracket_types, keep_type=False):
    """
    text: 待解析字符串
    bracket_types: 需要提取的括号类型，如 ['<>', '[]', '||']
    keep_type: 是否保留括号类型信息；True 返回 (type, content)
    """
    if not text or not bracket_types:
        return []

    # 起始符 -> 结束符
    open_to_close = {b[0]: b[1] for b in bracket_types}
    opens = set(open_to_close.keys())
    closes = set(open_to_close.values())

    results = []
    stack = []  # 栈元素: [start_char, end_char, buffer_list]

    for ch in text:
        # 1) 如果遇到起始符：开启一个新的捕获（允许嵌套）
        if ch in opens:
            stack.append([ch, open_to_close[ch], []])
            continue

        # 2) 如果当前处于某层括号内部
        if stack:
            start_char, end_char, buf = stack[-1]
            # 2.1) 当前字符是这一层的结束符：收束这一层
            if ch == end_char:
                content = ''.join(buf)
                if keep_type:
                    results.append((start_char + end_char, content))
                else:
                    results.append(content)
                stack.pop()
                continue
            # 2.2) 当前字符是某个“结束符”，但不是当前层需要的结束符：
            #      忽略它（防止串扰），仍继续在当前层收集字符
            #      你也可以选择把它当普通字符加入 buf，这里默认忽略更稳。
            if ch in closes and ch != end_char:
                continue
            # 2.3) 普通字符：加入当前层 buffer
            buf.append(ch)
    return results

# darw_QR=["dwsa<564ABC>$&hg","<ojjp[DFBA]hfcide","hdfw[1234]cyfud<5>Gi<G>"]
# str1 = None
# str2 = None
# str3 = None
# str4 = None
# result = []
# found_flags = {'wxd': False, 'cp': False, 'ld': False}
# for q in darw_QR:
#     QR1 = parse_bracket_content(q,['<>','[]'])
#     print(QR1)
#     for q in QR1:
#         lenght = len(q)
#         if not found_flags['cp'] and lenght == 6:
#             str1 = q
#             found_flags['cp'] = True
#             continue
#         if lenght==4 and StrictValidator.is_letter(q):
#          len1 = len(q)
#          count = 0
#          for _ in range(len1):
#             index=0
#             for i in q:
#               if q[count]>i:
#                 index+=1
#             count += 1
#             result.append(index)
#          print("result", result)
#         if  not found_flags['wxd'] and lenght==4 and StrictValidator.is_num(q) :
#             result2 = [1] * len(q)
#             for i in range(len(q)):
#                 result2[i] = q[result[i]]
#             str2=''.join(result2)
#             found_flags['wxd'] = True
#             continue
#         if not found_flags['ld'] and lenght==1 and StrictValidator.is_num(q):
#             str3=q
#             found_flags['ld'] = True
#             continue
# print("str1",str1)
# print("str2",str2)
# print("str3",str3)
# print("cp",found_flags['cp'])
# print("ld",found_flags['ld'])
# print("wxd",found_flags['wxd'])

darw_QR=["<0x1A,0xB3,0x0F,0x9C,0x45,0xD2>[3]{不是真}","<0xAA,0xBC,0xDE,0xFA,0xCF,0xED>[0]{是假的}",
         "<0xFF,0x00,0xAA,0x99,0xF0,0x0F>[9]{不是假}","<0x12,0x34,0x56,0x78,0x90,0x09>[7]{是假的}"]
found_flags = {'YXXX': False, 'BH': False}
str1 = None
str2 = None
for q in darw_QR:
    str = parse_bracket_content(q,['<>','[]','{}'])
    if str[2]=="不是假" or str[2]=="是真的":
        str1=str
    print("str1:",str1)
        
    

        
    
    
