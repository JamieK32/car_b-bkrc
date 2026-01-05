def _analyze_and_send(self, raw_basket):
    # ========== 状态收集区 ==========
    found_flags = {
        'WXD': False, 'FHT': False, 'CP': False,
        'CK': False, 'LD': False, 'SJ': False, 'WD': False
    }

    cp_raw = None      # 保存 DF3
    cp_offset = None   # 保存 [0]

    # ========== 第一阶段：只解析、不计算 ==========
    for raw_data in raw_basket:
        print("raw_data:", raw_data)

        items = StrictValidator.parse_bracket_content(
            raw_data, ['<>', '[]']
        ) or [raw_data]

        for content in items:
            print("content:", content)
            length = len(content)

            # ---------- SJ ----------
            if not found_flags['SJ'] and length == 10:
                self.uart_comm.send_var(VarID.SJ, content.encode())
                found_flags['SJ'] = True
                continue

            # ---------- WD ----------
            if not found_flags['WD'] and length >= 2 and StrictValidator.is_num(content):
                self.uart_comm.send_var(VarID.WD, content.encode())
                found_flags['WD'] = True
                continue

            # ---------- CP 原始数据 ----------
            
            if length == 3 and StrictValidator.is_hex(content):
                print("[CACHE] CP raw =", content)
                cp_raw = content
                continue

            # ---------- CP 偏移量 ----------
            if length == 1 and StrictValidator.is_num(content):
                cp_offset = int(content)
                print("[CACHE] CP offset =", cp_offset)
                
                if  
                
                continue

    # ========== 第二阶段：统一计算 CP ==========
    if cp_raw is not None and cp_offset is not None:
        print("[CALC] CP =", cp_raw, "offset =", cp_offset)

        cp_list = list(cp_raw)

        cp_list[0] = chr(ord(cp_list[0]) + cp_offset)
        cp_list[1] = chr(ord(cp_list[1]) + cp_offset)
        cp_list[2] = chr(ord(cp_list[2]) + cp_offset)

        cp_result = ''.join(cp_list)
        print("[SEND] CP =", cp_result)

        self.uart_comm.send_var(VarID.CP,cp_result.encode() )
        found_flags['CP'] = True
    else:
        print("[WARN] CP 数据不完整，raw:", cp_raw, "offset:", cp_offset)