def chinese_to_gbk_c_array(text, var_name="gbk_data"):
    try:
        # 将字符串编码为 GBK 字节流
        gbk_bytes = text.encode('gbk')
        
        # 将字节转换为 0xXX 格式的十六进制字符串
        hex_values = [f"0x{b:02X}" for b in gbk_bytes]
        
        # 格式化输出
        array_content = ", ".join(hex_values)
        c_code = f"uint8_t {var_name}[] = {{ {array_content} }};"
        
        print(f"\n--- 转换结果 ({len(gbk_bytes)} 字节) ---")
        print(c_code)
        print("-" * 30)
        
    except UnicodeEncodeError as e:
        print(f"错误：部分字符无法转换为 GBK 编码 - {e}")

if __name__ == "__main__":
    user_input = input("请输入想要转换的中文内容: ")
    name_input = input("请输入 C 数组变量名 (直接回车默认为 'gbk_data'): ")
    
    if not name_input.strip():
        name_input = "gbk_data"
        
    chinese_to_gbk_c_array(user_input, name_input)