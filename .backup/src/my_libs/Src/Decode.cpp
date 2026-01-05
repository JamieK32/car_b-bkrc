#include "DCMotor.h"
#include "Decode.h"
#include "My_Lib.h"
#include <string.h>
#include "math.h"

char temp_array_1[50];
char temp_array_2[50];
char temp_array_3[50];
char temp_array_4[50];
char temp_array_5[50];
char temp_array_6[50];
char temp_array_7[6];
uint8_t temp[50];
uint8_t sum_flag = 0;


void gitNum(char* qr_Tab, char* data2, int l) {
	int i = 0;
	long num1 = 0, num2 = 0;
	int sum = 0;
	while (qr_Tab[i] != '-') {
		num1 *= 10;
		num1 += qr_Tab[i] - 0x30;
		i++;
	}
	i++;
	while (i < l) {
		num2 *= 10;
		num2 += qr_Tab[i] - 0x30;
		i++;
	}
	
	Serial.print("num1:");
	Serial.println(num1);
	Serial.print("num2:");
	Serial.println(num2);
	while (num1 <= num2) {
		int a = 0;
		long num = num1;

		for (int j = 2; j <= num - 1; j++) {
			if (num % j == 0) {
				
				a++;
				break;
			}
		}
		if (a == 0) {
			Serial.print("zs:");
			Serial.println(num);
			data2[sum] = num;
			sum++;
		}

		num1 += 1;
	}
	Serial.print("sum:");
	Serial.println(sum);
	char a[20] = { 0 };
	itoa(a, sum, 10);//将a以10进制的形式写入buff中
	/*data2[0] = a[0];
	data2[1] = a[1];
	data2[2] = a[2];
	data2[3] = a[3];
	data2[4] = a[4];
	data2[5] = a[5];
	Serial.print((char)data2[0]);
	Serial.print((char)data2[1]);
	Serial.print((char)data2[2]);
	Serial.print((char)data2[3]);
	Serial.print((char)data2[4]);
	Serial.println((char)data2[5]);*/
}

//统计数组有效长度
int Len(int* data) {
	int len = 0;
	int dataLen = sizeof(data) / sizeof(data[0]);
	while (len< dataLen &&data[len] != 0) {
		len++;
	}
	return len;
}

#define TOTAL_LEN 12

void hex_sort(uint8_t *arr, uint8_t len)
{
    uint8_t i, j, temp;

    for (i = 0; i < len - 1; i++)
    {
        for (j = 0; j < len - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}


/**
 * @brief  两个6字节数组 → 排序 → 取中间6字节
 * @param  a,b      输入数组
 * @param  result   输出中间6位
 */
void get_middle_six(const uint8_t *a,  const uint8_t *b,uint8_t *result)
{
    uint8_t buf[TOTAL_LEN];
    uint8_t i;

    // 1. 合并
    for (i = 0; i < 6; i++)
    {
        buf[i]     = a[i];
        buf[i + 6] = b[i];
    }

    // 2. 排序
    hex_sort(buf, TOTAL_LEN);

    // 3. 取中间6位（12 → 下标 3 ~ 8）
    for (i = 0; i < 6; i++)
    {
        result[i] = buf[i + 3];
    }
}



int ifNum(char num) {
	return num >= '0' && num <= '9';
}

int ifA(char c) {
	return c >= 'A' && c <= 'Z';
}

int ifa(char c) {
	return c >= 'a' && c <= 'z';
}

int ifAa(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int ifAa1(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

//排序     // 0表示从小到大  1表示从大到小
void Colon_sort(uint8_t* array_buf,  uint8_t smallTobig)
{
	uint8_t len = strlen(array_buf);
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t temp;

	if (smallTobig == 0)
	{
		for (i = 0; i < len - 1; i++)
		{
			for (j = 0; j < len - i - 1; j++)
			{
				if (array_buf[j] > array_buf[j + 1])
				{
					temp = *(array_buf + j);
					*(array_buf + j) = *(array_buf + j + 1);
					*(array_buf + j + 1) = temp;
				}
			}
		}
	}
	if (smallTobig == 1)
	{
		for (i = 0; i < len - 1; i++)
		{
			for (j = 0; j < len - i - 1; j++)
			{
				if (array_buf[j] < array_buf[j + 1])
				{
					temp = *(array_buf + j);
					*(array_buf + j) = *(array_buf + j + 1);
					*(array_buf + j + 1) = temp;
				}
			}
		}
	}
}




//复制数组
void copy(char* arr, const char* data) {
	int len = strlen(data);
	for (int i = 0; i < len; ++i) {
		arr[i] = data[i];
	}
	Serial.print("copy:");
	Serial.println(arr);
}

/*逆序*/
void Reverse(char* str)//将字符串倒序
{
	uint8_t len = strlen(str);
	uint8_t i;
	char temp;
	for (i = 0; i < (len / 2); i++)//第一个和最后一个交换
	{
		temp = str[i];
		str[i] = str[len - i - 1];
		str[len - i - 1] = temp;
	}
}
/********************************************************
将char类型的A转为0x0A(大于F的就会转换成对应的十六进制数字，G=>0x10)
参数：   	 
返回值： 无
********************************************************/
void take_out_AoutHex(char* letters) {
	int len = strlen(letters);
	for (int i = 0; i < len; i++)
	{
		if (letters[i] <= 'Z' && letters[i] >= 'A') {
			letters[i] = letters[i] - 0x37;
		}
		else {
			letters[i] = letters[i] - 0x30;
		}

	}
}

/**********************************************
* 函数名称:将一个数字拆解成数组
* 参数：	tempArrays		接收数组
*			num				带拆解的数字
***********************************************/
//void  (char* tempArrays, const long num) {
//	long temp = 1;
//	long num2 = num;
//	long index = 0;
//	while (num2 != 0) {
//		temp *= 10;
//		num2 /= 10;
//	}
//	temp /= 10;
//	while (temp > 0) {
//		tempArrays[index] = ((num / temp) % 10)+0x30;
//		index++;
//		temp /= 10;
//	}
//}

/**********************************************
* 函数名称:将字符串转为数字
***********************************************/
double strToNum(const char* tempArrays) {
	int len = strlen(tempArrays);
	int D = 1;
	double sum = 0;
	double temp = 10;
	
	for (int i = 0; i < len; i++)
	{
		if (ifNum(tempArrays[i])) {
			if (D) {
				sum *= 10;
				sum += tempArrays[i] - 0x30;
			}
			else {
				double num2 = (tempArrays[i] - 0x30) / temp;
				sum += num2;
				temp *= 10;
			}

		}
		else if (tempArrays[i] == '.') {
			D = 0;

		}
	}
	return sum;
}

//单个字符转数字
int AHexToNum(char a) {
	int num = 0;
if(	ifA(a)){
		num = a - 0x37;
	}
else if( ifNum(a))
{
	num = a - 0x30;
}
else
{
	num = a - 0x57;
}
return num;
}

/*
将0x12转为12
*/
void hexToNum(uint8_t* data) {
	int len = strlen(data);
	for (int i = 0; i < len; i++)
	{
		data[i] = AHexToNum(data[i]);
	}
}
//将单个12转为0x12
int ANumToHex(int data) {
	int num = 0;
	num += (data / 10) * 0x10;
	num += data % 10;
	return num;
}
//将数组字符12转为0x12
void numToHex(uint8_t* data,uint8_t* data1) {
	int len = strlen(data);
	Serial.println(len);
	int j = 0;
	for (int i = 0; i < len; i += 2) {
		if (i+1< len) {

			if (ifNum(data[i])) {
				data1[j] = (data[i] - 0x30) * 0x10;
			}
			else if (ifA(data[i])) {
				data1[j] = (data[i] - 0x37) * 0x10;
			}
			else if (ifa(data[i])) {
				data1[j] = (data[i] - 0x57) * 0x10;
			}
			if (ifNum(data[i + 1])) {
				data1[j] += data[i + 1] - 0x30;
			}
			else if (ifA(data[i + 1])) {
				data1[j] += data[i + 1] - 0x37;
			}
			else if (ifa(data[i+1])) {
				data1[j] += data[i + 1] - 0x57;
			}
			j++;
		}
	}
	Serial.println("strlen(data1)");
	Serial.println(strlen(data1));
	for (int x = 0; x < strlen(data1); x++)
	{
		Serial.println(data1[x]);
	}
}



/**
 * 将数组按照下标进行位移
 * @param arrays    数据
 * @param n         位移的位数
 * @param k         模式  k=1 向左  k=0向右
 */
void index_move(uint8_t* arrays, int n, int k) {
	int len = strlen(arrays);
	uint8_t  temp[len];
	int j = 0;
	if (k == 1) {
		for (int i = (n % len); i < len; i++) {
			temp[j++] = arrays[i];
		}
		for (int i = 0; i < (n % len); ++i) {
			temp[j++] = arrays[i];
		}
	}
	else {
		for (int i = (len - (n % len)) % len; i < len; i++) {
			temp[j++] = arrays[i];
		}
		for (int i = 0; i < (len - (n % len)) % len; i++) {
			temp[j++] = arrays[i];
		}
	}
	copy((char*)arrays, (char*)temp);
}

//打印数字
void show_Num(long num,const char* theme) {
	Serial.print(theme);
	Serial.print(":");
	Serial.println(num,DEC);
}

//打印十六进制数字
void show_HEX(int num, const char* theme) {
	Serial.print(theme);
	Serial.print(":");
	Serial.println(num,HEX);
}


/*************************************************************
* 函数名：打印整型数组
* 参  数：	data		原始数据
*			theme		标题
**************************************************************/
void show_arr(const int* data,const char * theme) {
	int len = Len(data);
	Serial.print(theme);
	Serial.print(":");
	for (int i = 0; i < len; i++)
	{
		Serial.print("[");
		Serial.print(data[i],DEC);
		Serial.print("]");
		if (i != len - 1) {
			Serial.print(",");
		}
	}
	Serial.println("");
}

/*************************************************************
* 函数名：打印char数组
* 参  数：	data		原始数据
*			theme		标题
**************************************************************/
void show_str(const char* data, const char* theme) {
	Serial.print(theme);
	Serial.print(":");
	Serial.println(data);

}

/*************************************************************
* 函数名：打印16进制数组
* 参  数：	data		原始数据
*			theme		标题
**************************************************************/
void show_ArrHex(const uint8_t* data, const char* theme) {
	int len = Len(data);
	Serial.print(theme);
	Serial.print(":");
	for (int i = 0; i < len; i++)
	{
		Serial.print("[");
		Serial.print(data[i],HEX);
		Serial.print("]");
		if (i != len - 1) {
			Serial.print(",");
		}
	}
	Serial.println("");
}

/*************************************************************
* 函数名：取出随机路线的数据
* 参  数：	LX			随机路线存放的数组
*			data		原始数据
* 实例：getLX(lx2, "A1B2C3");
*		lx[0]=0xA1;lx[1]=0xB2;lx[3]=0xC3;
**************************************************************/
void getLX(uint8_t* LX, const char* data) {
	int dataLen = strlen((uint8_t*)data);
	int j = 0;
	for (int i = 0; i < dataLen; i += 2) {
		if (i + 1 < dataLen) {
			if (ifA(data[i]) && ifNum(data[i + 1])) {
				if (data[i] == 'G') {
					LX[j] = 0;
				}
				else {
					LX[j] = (data[i] - 0x37) * 0x10;
				}
				LX[j] += data[i + 1] - 0x30;
				j++;
			}
		}
	}
	for (int i = 0; i < 10; i++)
	{
		Serial.println(LX[i],HEX);
	}
}

/*************************************************************
* 函数名：取出两个字符中间的数据
* 参  数：	data		原始数据
*			c1			左边的字符
*			c2			右边的字符
*			arr			储存的数组
**************************************************************/
void midData(const char* data, char c1, char c2, char* arr) {
	int len = strlen((uint8_t*)data);
	int left = -1, right = -1;
	for (int i = 0; i < len; i++) {
		if (data[i] == c1 && left == -1) {//
			left = i;
		}
		else if (data[i] == c2 && left != -1) {
			right = i;
		}
	}
	int j = 0;
	for (int i = left + 1; i < right; i++) {
		arr[j++] = data[i];
	}

}



/**
 * 两个数组进行逻辑运算
 * @param data1 数组1
 * @param data2 数组2
 * @param tempArrays 储存的数组
 * @param pattern ‘|’，‘&’，‘^’
 */
void logicalOperation(uint8_t* data1, uint8_t* data2, uint8_t* tempArrays, char pattern) {
	int len = strlen(data1) < strlen(data2) ? strlen(data1) : strlen(data2);
	for (int i = 0; i < len; i++) {
		if (pattern == '|') {
			tempArrays[i] = data1[i] | data2[i];
		}
		else if (pattern == '&') {
			tempArrays[i] = data1[i] & data2[i];
		}
		else if (pattern == '^') {
			tempArrays[i] = data1[i] ^ data2[i];
		}
		else if (pattern == '+') {
			tempArrays[i] = data1[i] + data2[i];
		}else if (pattern == '-') {
			tempArrays[i] = data1[i] - data2[i];
		}else if (pattern == '*') {
			tempArrays[i] = data1[i] * data2[i];
		}else if (pattern == '/') {
			tempArrays[i] = data1[i] / data2[i];
		}
	}
}

/**
 * 将一个数组与n进行逻辑运算
 * @param data1 数组
 * @param pattern 运算
 */
void dataOperation(uint8_t* data1, char pattern, int n) {
	for (int i = 0; i < strlen(data1); i++) {
		if (pattern == '|') {
			data1[i] |= n;
		}
		else if (pattern == '&') {
			data1[i] &= n;
		}
		else if (pattern == '^') {
			data1[i] ^= n;
		}
	}
}

/**
 * 在大于零的数组中查找最大值或最小值
 * @param data  数据
 * @param maxOrMin  1；查找最大值  0：查找最小值
 */
int getMaxOrMin(const uint8_t* data, int maxOrMin) {
	int num = data[0];
	if (maxOrMin == 1) {
		for (int i = 0; i < strlen(data); i++) {
			if (data[i] > num) {
				num = data[i];
			}
		}
	}
	else {
		for (int i = 0; i < strlen(data); i++) {
			if (data[i] < num) {
				num = data[i];
			}
		}
	}
	return num;
}







/****************************************  计算器部分  ********************************************************************/


//compute函数使用
#define FNX0 3//常数数组
#define FNX1 8//指向有一个参数的函数的指针的数组
#define FNX2 1//指向有二个参数的函数的指针的数组

char* fn0[FNX0] = { "pi","e","m" };
double fx0[FNX0] = { 3.14159265358979323846,2.71828182845904523536,0 };

char* fn1[FNX1] = { "acos","asin","atan","cos","sin","tan","ln","sqrt" };
double (*fx1[FNX1])(double) = { acos,asin,atan,cos,sin,tan,log,sqrt };

char* fn2[FNX2] = { "pow" };
double (*fx2[FNX2])(double, double) = { pow };


double compute(char* xin, int length, int* error) {//算式字符串，字符串长度，错误信息
	signed char o[3] = { 0 };//运算符栈 +-*/ 为1234 0为空
	signed char s[3] = { 1,1,1 };//数字栈符号
	short Now0 = 0;//运算符下标
	short Now1 = 0;//数字下标
	short yes = 0;//判断函数是否成功
	int i = 0;//计数
	int j = 0;//计数
	int k = 0;//计数
	int n = 0;//计数 已读字符数目
	int nextlength = 0;//计数 第一参数字符数目
	int nextlength2 = 0;//计数 第一参数字符数目
	char* p = xin;//指向字符的指针
	double value[3] = { 0 };//数字栈
	double pt = 0;//小数点

	//循环开始 读字符 不支持空格
	while (*p != '=' && n != length && *p != 0) {
		if (*p >= '0' && *p <= '9') {//输入为数字
			pt == 0 ? (value[Now1] = 10 * value[Now1] + *p - '0') : ((value[Now1] = value[Now1] + (*p - '0') / pt), pt = 10 * pt);
		}
		else if (*p == '.') {//输入为小数点
			pt = 10;
		}
		else if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {//输入为加减乘除
			pt = 0;
			if (p == xin ? 1 : !(*(p - 1) >= '0' && *(p - 1) <= '9' || *(p - 1) == '.' || *(p - 1) == ')')) {//+-是一元运算符
				if (*p == '+') {
					s[Now1] = s[Now1];
				}
				else if (*p == '-') {
					s[Now1] = -s[Now1];
				}
				else {//格式错误
					*error = 0;
					return 0;
				}
				++p;
				++n;
				continue;//结束 读入下一个字符
			}
			if (o[Now0] == 0) {//栈为空 入栈
				++Now0;
                switch (*p) { case '+':o[Now0] = 1; break; case '-':o[Now0] = 2; break; case '*':o[Now0] = 3; break; case '/':o[Now0] = 4; break; }
									  ++Now1;
			}
			else if (o[Now0] == 1 || o[Now0] == 2) {//栈顶为加减
				if (*p == '*' || *p == '/') {//输入乘除优先级大于栈顶符号     压栈
					++Now0;
				switch (*p) { case '*':o[Now0] = 3; break; case '/':o[Now0] = 4; break; }
									  ++Now1;
				}
				else {//优先级不大于栈顶符号   输入加减出栈
					--Now1;
					if (o[Now0] == 1)
						value[Now1] = (s[Now1] * value[Now1]) + (s[Now1 + 1] * value[Now1 + 1]);
					else
						value[Now1] = (s[Now1] * value[Now1]) - (s[Now1 + 1] * value[Now1 + 1]);
					--Now0;
					s[Now1 + 1] = 1;
					value[Now1 + 1] = 0;
					continue;//没有压栈下标不右移
				}
			}
			else if (o[Now0] == 3 || o[Now0] == 4) {//栈顶乘除 优先级一定不大于乘除   出栈
				--Now1;
				if (o[Now0] == 3)
					value[Now1] = (s[Now1] * value[Now1]) * (s[Now1 + 1] * value[Now1 + 1]);
				else if (o[Now0] == 4)
					value[Now1] = (s[Now1] * value[Now1]) / (s[Now1 + 1] * value[Now1 + 1]);
				--Now0;
				s[Now1 + 1] = 1;
				value[Now1 + 1] = 0;
				continue;//没有压栈下标不右移
			}
		}
		else if (*p == '(') {//左括号 递归
			pt = 0;
			nextlength = 0;
			i = 1;
			while (i != 0) {
				++nextlength;
				if (*(p + nextlength) == '(')//左括号加1
					++i;
				else if (*(p + nextlength) == ')')//右括号减一
					--i;
				else if (*(p + nextlength) == 0) {//错误
					*error = 0;
					return 0;
				}
			}
			--nextlength;
			value[Now1] = compute(p + 1, nextlength, error);
			p = p + nextlength + 1;//补全缺少的字符
			n = n + nextlength + 1;
		}
		else if (*p >= 'a' && *p <= 'z') {//函数首字母小写
			pt = 0;
			yes = 0;//判断是否成功执行函数

			for (j = 0; j < FNX0; j++) {//常数
				for (k = 0; *(fn0[j] + k) == *(p + k); k++);//匹配字符串
				if (*(fn0[j] + k) == 0) {
					if (*(p + k) == '(' || *(p + k + 1) == ')') {
						value[Now1] = fx0[j];
						p = p + 1 + k;
						n = n + 1 + k;
						yes = 1;//成功
					}
					else { *error = 0; return 0; }//错误
				}
			}

			for (j = 0; j < FNX1; j++) {//一元
				for (k = 0; *(fn1[j] + k) == *(p + k); k++);//匹配字符串
				if (*(fn1[j] + k) == 0) {
					if (*(p + k) == '(') {
						nextlength = 0;
						i = 1;
						while (i != 0) {
							++nextlength;
							if (*(p + nextlength + k) == '(')//左括号加1
								++i;
							else if (*(p + nextlength + k) == ')')//右括号减一
								--i;
							else if (*(p + nextlength + k) == 0) {//错误 退出
								*error = 0;
								return 0;
							}
						}
						--nextlength;
						value[Now1] = fx1[j](compute(p + 1 + k, nextlength, error));
						p = p + nextlength + 1 + k;
						n = n + nextlength + 1 + k;
						yes = 1;
					}
					else { *error = 0; return 0; }
				}
			}

			for (j = 0; j < FNX2; j++) {//二元
				for (k = 0; *(fn2[j] + k) == *(p + k); k++);//匹配字符串
				if (*(fn2[j] + k) == 0) {
					if (*(p + k) == '(') {
						nextlength = 0;
						nextlength2 = 0;
						i = 1;
						while (i != 0) {
							++nextlength2;
							if (*(p + nextlength + nextlength2 + k) == '(')
								++i;
							else if (*(p + nextlength + nextlength2 + k) == ')')
								--i;
							else if (*(p + nextlength + nextlength2 + k) == 0) {//在)前遭遇字符串结束 错误
								*error = 0;
								return 0;
							}
							else if (*(p + nextlength + nextlength2 + k) == ',') {//逗号 计算第二参数长度
								nextlength = nextlength2;
								nextlength2 = 0;
							}
						}
						--nextlength;
						--nextlength2;
						value[Now1] = fx2[j](compute(p + 1 + k, nextlength, error), compute(p + 2 + k + nextlength, nextlength2, error));
						p = p + nextlength + nextlength2 + 2 + k;
						n = n + nextlength + nextlength2 + 2 + k;
						yes = 1;
					}
					else { *error = 0; return 0; }
				}
			}
			//失败 错误
			if (yes == 0) { *error = 0; return 0; }
		}
		else {//未知的符号 错误
			*error = 0;
			return 0;
		}
		++p;//右移下标
		++n;//读入字符增加
	}
	//循环结束

	//读字符停止   循环开始 出栈
	while (o[Now0] != 0) {
		--Now1;
		if (o[Now0] == 1)
			value[Now1] = (s[Now1] * value[Now1]) + (s[Now1 + 1] * value[Now1 + 1]);
		else if (o[Now0] == 2)
			value[Now1] = (s[Now1] * value[Now1]) - (s[Now1 + 1] * value[Now1 + 1]);
		else if (o[Now0] == 3)
			value[Now1] = (s[Now1] * value[Now1]) * (s[Now1 + 1] * value[Now1 + 1]);
		else if (o[Now0] == 4)
			value[Now1] = (s[Now1] * value[Now1]) / (s[Now1 + 1] * value[Now1 + 1]);
		s[Now1] = 1;
		--Now0;
	}
	//循环结束

	//去除错误输入 算式最后不可能不是数字或右括号
	if (!(*(p - 1) >= '0' && *(p - 1) <= '9' || *(p - 1) == '.' || *(p - 1) == ')')) {
		*error = 0;
		return 0;
	}

	//返回答案
	return s[0] * value[0];//数字数组第一个元素与符号数组第一个元素乘积为最终答案
}


/**
 * 将str中的^替换为pow(x,x) 已经弃用
 * @param str
 */
void instrument(char* str) {
	int len = strlen(str);
	for (int i = 0; i < len; i++) {
		if (str[i] == '^') {
			int left = i - 1;
			int right = i + 1;
			char leftData[20];
			char rightData[20];
			int leftIndex = 0;
			int rightIndex = 0;
			//统计左边的内容
			int bracket = 0;
			while ((str[left] <= '9' && str[left] >= '0') || str[left] == '(' || str[left] == ')' || bracket != 0) {
				if (str[left] == ')') {
					bracket++;
				}
				else if (str[left] == '(') {
					if (bracket > 0) {
						bracket--;
					}
					else {
						break;
					}

				}
				leftData[leftIndex++] = str[left];
				left--;
			}

			//交换
			int l = 0, r = leftIndex - 1;
			while (l < r) {
				char c = leftData[l];
				leftData[l] = leftData[r];
				leftData[r] = c;
				l++;
				r--;
			}

			//统计右边的
			bracket = 0;
			while ((str[right] <= '9' && str[right] >= '0') || str[right] == '(' || str[right] == ')' || bracket != 0) {
				if (str[right] == ')') {
					if (bracket == 0) {
						break;
					}
					else {
						bracket--;
					}

				}
				else if (str[right] == '(') {
					bracket++;


				}
				rightData[rightIndex++] = str[right];
				right++;
			}
			//将数组后面的先用一个临时数组保存下来
			char tempArr[20];
			int tempIndex = 0;
			for (int j = right; j < len; ++j) {
				tempArr[tempIndex++] = str[j];
			}
			//show(tempArr,-1);
			left += 1;


			int leftNum = compute(leftData, leftIndex, 1);
			int rightNum = compute(rightData, rightIndex, 1);
			int tempSum = leftNum;
			for (int p = 0; p < rightNum - 1; p++) {
				tempSum *= leftNum;
			}
			char tempNum[20];
			//getNum_A(tempNum,tempSum);

			itoa(tempSum, tempNum, 10);

			int tempLen = strlen(tempNum);
			for (int x = 0; x < tempLen; x++) {
				str[left++] = tempNum[x];
			}
			//补齐剩余的内容
			for (int j = 0; j < tempIndex; j++) {
				str[left++] = tempArr[j];
			}
			len = strlen(str);
			for (int j = left; j < len; j++) {
				str[j] = 0;
			}
		}
	}
}



double calculator(char* string) {
	char c[200];
	sum_flag = 0;
	int len = strlen(string);
	for (int i = 0; i < len; i++) {
		c[i] = string[i];
	}
	for (int i = len; i < 200; ++i) {
		c[i] = 0;
	}
	instrument(c);
	Serial.print("c:");
	Serial.println(c);
	len = strlen(c);
	Serial.print("len:");
	Serial.println(len);
	double num = compute(c, len,1);
	Serial.print("num:");
	Serial.println(num);
	return num;
}


/**
 * 对str中的moban进行替换为temp(前提是这个数字本身就能放下拓展后的长度)
 * @param str
 * @param temp
 * @param template
 */
void replace(char* str, char* temp, char moban) {
	int len = strlen(str);
	Serial.print("len:");
	Serial.println(len);
	for (int i = 0; i < len; i++) {
		if (str[i] == moban) {
			//先将后面的保存起来
			char data[50];
			int dataIndex = 0;
			int right = i + 1;
			while (right < len) {
				data[dataIndex++] = str[right++];
			}
			int tempLen = strlen(temp);
			int k = i;
			for (int j = 0; j < tempLen; j++) {
				str[k++] = temp[j];
			}
			for (int j = 0; j < dataIndex; j++) {
				str[k++] = data[j];
			}
			len = strlen(str);
		}
	}
}


int D8(int value) {
	int num = value & 0xFF;
	return num;
}

int G8(int value) {
	int num = value;
	while (num > 0xFFFF) {
		num = num >> 1;
	}
	int high = (num >> 8) & 0xff;
	return high;
}


/**
 * 将数组元素位移
 * @param arrays 数据
 * @param n      位移的位数
 * @param k      位移的模式 1:向后位移/向右  0：向前位移/向左
 */
void rearward_shift(char* arrays, int n, int k) {
	int  len=strlen(arrays) ;
	Serial.print("len:");
	Serial.println(len);
	if (k == 1) {

		for (int i = 0; i < len; ++i) {
			Serial.print(arrays[i]);
			if (ifNum(arrays[i])) {
				arrays[i] = (((arrays[i] + n) - 0x30) % 10) + 0x30;
			}
			else 
			if (ifA((char)arrays[i])) {
				arrays[i] = (((arrays[i] + n) - 'A') % 26) + 0x41;
			}
			else {
				arrays[i] = (((arrays[i] + n) - 'a') % 26) + 0x61;
			}
			
		}
	}
	else {
		for (int i = 0; i < len; ++i) {

			if (ifNum(arrays[i])) {
				arrays[i] = arrays[i] - n;
				if (arrays[i] < '0') {
					arrays[i] = 0x3a - ('0' - arrays[i]);
				}
			}
			else if (ifA((char)arrays[i])) {
				arrays[i] = arrays[i] - n;
				if (arrays[i] < 'A') {
					arrays[i] = 0x5B - ('A' - arrays[i]);
				}
			}
			else {
				arrays[i] = arrays[i] - n;
				if (arrays[i] < 'a') {
					arrays[i] = 0x7b - ('a' - arrays[i]);
				}
			}
		}
	}
	Serial.println("arrays:");
	for (int i = 0; i < len; i++) {
		Serial.print(arrays[i]);
	}
}


int ys(char a) {
	if (a == 'A' || a == 'a') {
		return 1;
	}
	if (a == 'B' || a == 'b') {
		return 2;
	}
	if (a == 'C' || a == 'c') {
		return 3;
	}
	if (a == 'D' || a == 'd') {
		return 4;
	}
	if (a == 'E' || a == 'e') {
		return 5;
	}
	if (a == 'F' || a == 'f') {
		return 6;
	}
	if (a == 'G' || a == 'g') {
		return 7;
	}
	if (a == 'H' || a == 'h') {
		return 8;
	}
	if (a == 'I' || a == 'i') {
		return 9;
	}
	if (a == 'J' || a == 'j') {
		return 10;
	}
	if (a == 'K' || a == 'k') {
		return 11;
	}
	if (a == 'L' || a == 'l') {
		return 12;
	}
	if (a == 'M' || a == 'm') {
		return 13;
	}
	if (a == 'N' || a == 'n') {
		return 14;
	}
	if (a == 'O' || a == 'o') {
		return 15;
	}
	if (a == 'P' || a == 'p') {
		return 16;
	}
	if (a == 'Q' || a == 'q') {
		return 17;
	}
	if (a == 'R' || a == 'r') {
		return 18;
	}
	if (a == 'S' || a == 's') {
		return 19;
	}
	if (a == 'T' || a == 't') {
		return 20;
	}
	if (a == 'U' || a == 'u') {
		return 21;
	}
	if (a == 'V' || a == 'v') {
		return 22;
	}
	if (a == 'W' || a == 'w') {
		return 23;
	}
	if (a == 'X' || a == 'x') {
		return 24;
	}
	if (a == 'Y' || a == 'y') {
		return 25;
	}
	if (a == 'Z' || a == 'z') {
		return 26;
	}
}


/**(字符)
 * 将数组按照下标进行位移
 * @param arrays    数据
 * @param n         位移的位数
 * @param k         模式  k=1 向左  k=0向右
 */
void index_move(char* arrays, int n, int k) {
	int len = strlen(arrays);
	char  temp[len];
	int j = 0;
	if (k == 1) {
		for (int i = (n % len); i < len; i++) {
			temp[j++] = arrays[i];
		}
		for (int i = 0; i < (n % len); ++i) {
			temp[j++] = arrays[i];
		}
	}
	else {
		for (int i = (len - (n % len)) % len; i < len; i++) {
			temp[j++] = arrays[i];
		}
		for (int i = 0; i < (len - (n % len)) % len; ++i) {
			temp[j++] = arrays[i];
		}
	}
	copy(arrays, temp);
}

/*数字右移，字母左移*/
void displacement (char * arrays, int n) {

	int len = strlen(arrays);
	Serial.print("len:");
	Serial.println(len);
	int temp = 0;
	//for (int i = 0; i < 3; i++) {						//反转字符串
	//	temp = arrays[i];
	//	arrays[i] = arrays[6 - i - 1];
	//	arrays[6 - i - 1] = temp;
	//}
		for (int i = 0; i < len; ++i) {
			if(ifAa(arrays[i])){
				if (ifA(arrays[i])){
					arrays[i] = arrays[i] - n;
					if (arrays[i] < 'A') {
						arrays[i] = 0x5B - ('A' - arrays[i]);
					}
				}
				else {
					arrays[i] = arrays[i] - n;
					if (arrays[i] < 'a') {
						arrays[i] = 0x7b - ('a' - arrays[i]);
					}
				}
			}
			else if(ifNum(arrays[i])){
				arrays[i] = (((arrays[i] + n) - 0x30) % 10) + 0x30;
			}
			Serial.print(arrays[i]);
		}
}







