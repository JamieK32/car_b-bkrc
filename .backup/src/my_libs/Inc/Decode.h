#ifndef _DECODE_H
#define _DECODE_H

extern char temp_array_1[50];
extern char temp_array_2[50];
extern char temp_array_3[50];
extern char temp_array_4[50];
extern char temp_array_5[50];
extern char temp_array_6[50];
extern char temp_array_7[6];
extern uint8_t temp[50];
extern uint8_t YYBB[50];
extern void YY_Play_Zigbee_2(char* p);
extern void Colon_sort(uint8_t* array_buf, uint8_t smallTobig);		//数组排序 0表示从小到大  1表示从大到小	        
extern int Len(uint8_t* data);										//计算数组有效长度
extern void copy(char* arr, const char* data);						//复制数组
extern void Reverse(char* str);										//将数组倒序
extern void take_out_AoutHex(char* letters);						//将char类型的A转为0x0A
extern void numToStr(char* tempArrays, const long num);				//将一个数字拆解成字符串
extern double strToNum(const char* tempArrays);						//将字符串转为数字
extern int AHexToNum(char a);										//单个字符转数字
extern int ANumToHex(int data);//12转0x12
extern void hexToNum(uint8_t* data);								//将数组0x12转为12
extern void numToHex(uint8_t* data, uint8_t* data1);					//将数组12转为0x12
extern void get_middle_six(const uint8_t *a, const uint8_t *b,uint8_t *result);  //取出两个字符中间的数据
extern void hex_sort(uint8_t *arr, uint8_t len);
extern void getLX(uint8_t* LX, const char* data);					//取出随机路线的数据
extern void dataOperation(uint8_t* data1, char pattern, int n);					//将一个数组与n进行逻辑运算
extern void logicalOperation(uint8_t* data1, uint8_t* data2, uint8_t* tempArrays, char pattern);		//两个数组进行逻辑运算 结果在tempArrays中
extern int getMaxOrMin(const uint8_t* data, int maxOrMin);			//在大于零的数组中查找1:最大值或0:最小值
extern void replace(char* str, char* temp, char moban);			//对str中的template进行替换为temp(前提是这个数字本身就能放下拓展后的长度)
extern double calculator(char* string);								//计算器 支持括号、幂运算、sin、cos、tan、asin、acos
extern void index_move(char* arrays, int n, int k);				//数组下标位移 模式  k=1 向左  k=0向右
extern void rearward_shift(char* arrays, int n, int k);			//将数组位移n位    k: 位移的模式 1:向后位移  0：向前位移
extern int ys(char);
extern void displacement(char * arrays, int n);					///*数字右移，字母左移*/
int D8(int value);
int G8(int value);
extern void midData(const char* data, char c1, char c2, char* arr);  //取出两个字符中间的数据
extern void instrument(char* str);
extern void gitNum(char* qr_Tab, char* data2, int l);
//C语言内置函数
/*
* strcpy(char *dest, const char *src);						//将将参数src字符串拷贝至参数dest(注意数组越界)
* strncpy(char *dest, const char *src, size_t n);			//将字符串src前n个字符拷贝到字符串dest
* strcat(char *dest, const char *src);						//将字符串src拼接到dest的队尾
* strncat(char *dest, const char *src, size_t n);			//将字符串src前n个字符拼接到dest的队尾
* int strcmp(const char *s1, const char *s2);				//字符串比较 = return 0 | > return 1 | < return -1
* int atoi (const char * str);								//将字符串转化为int型(不知道为什么，支持的数字很小)
* double atof(const char *str);								//将字符串转化为double型 (数字一大小数部分不知为何丢失了)
* long atol(const char * *str);								//将字符串转化为long型
* itoa(int num,char *tempArr,int n);						//将数字num转为字符串 n=进制
*/


//打印
extern void show_Num(long num, const char* theme);						//打印数字
extern void show_HEX(int num, const char* theme);						//打印HEX数字
extern void show_arr(const int* data, const char* theme);				//打印数组
extern void show_str(const char* data, const char* theme);				//打印字符串
extern void show_ArrHex(const uint8_t* data, const char* theme);		//打印Hex数组

//判断
extern int ifNum(char num);		//是否为数字
extern int ifA(char num);		//是否为大写字母
extern int ifa(char num);		//是否为小写字母
extern int ifAa1(char num);		//是否


#endif /* _DECODE_H */