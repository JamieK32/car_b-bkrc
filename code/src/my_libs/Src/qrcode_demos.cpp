// 我的封装
#include "car_controller.hpp"
#include "log.hpp"
#include "k210_protocol.hpp"
#include "k210_app.hpp"
#include "lsm6dsv16x.h"
#include "qrcode_datamap.hpp"
#include "algorithm.hpp"
#include "qrcode_demos.hpp"

#define LOG_QRCODE_EN 1

#if LOG_QRCODE_EN
  #define log_qr(fmt, ...)  LOG_P("[QRCODE] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define log_qr(...)  do {} while(0)
#endif


void qrcodeProcess(void) {
	const uint8_t *qr_data1;
	const uint8_t *qr_data2;
	
	identifyQrCode_New(3);
	qr_data_map();
	
	qr_data1 = qr_get_typed_data(kQrType_Formula);
	qr_data2 = qr_get_typed_data(kQrType_Bracket_All);
	
	log_qr("d1 = %s, d2 = %s", qr_data1, qr_data2);

	
	// 算法A
	int a_res = 0;
	double n = 4;
	double x = 5;
	double y = 8;
	VarBinding vars[] = {
  		{"n", n},
  		{"x", x},
  		{"y", y},
	};

	char expr2[128];
	if (bind_expr(expr2, sizeof(expr2), (const char *)qr_data1, vars, 3) == 0) {
		double out;
		log_qr("expr2 = %s", expr2);
		algo_calc_eval(expr2, &out);
		a_res = (int)lround(out);
		log_qr("a_res = %d", a_res);
	}

	//算法B
	int b_res[6] = {0};

	char *s1 = algo_extract_bracket((const char *)qr_data2, '['); 
	char *s2 = algo_extract_bracket((const char *)qr_data2, '<');

	int *arr1 = digits_to_intv(s1);
	int *arr2 = digits_to_intv(s2);

	if (arr1 && arr2) {
		b_res[0] = (arr1[0] << arr2[0]) % 10;
		b_res[1] = (arr1[1] >> arr2[1]) % 10;
		b_res[2] = (arr1[2] |  arr2[2]) % 10;
		b_res[3] = (arr1[3] &  arr2[3]) % 10;
		b_res[4] = (arr1[4] ^  arr2[4]) % 10;
		b_res[5] = (arr1[5] +  arr2[5]) % 10;
	}

	for (int i = 0; i < 6; i++) {
		log_qr("%d  ", b_res[i]);
	}
	if (s1) free(s1);
	if (s2) free(s2);
	if (arr1) free(arr1);
	if (arr2) free(arr2);
}