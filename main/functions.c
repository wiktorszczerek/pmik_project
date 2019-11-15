#include "functions.h"
#include "esp_adc_cal.h" //for uints

uint8_t make_byte_from_bits(uint8_t* arr)
{
	return ((arr[0]!=0)<<7 | (arr[1]!=0) << 6 | (arr[2]!=0) << 5 | (arr[3]!=0) << 4 | (arr[4]!=0) << 3 | (arr[5]!=0) << 2 | (arr[6]!=0) << 1 | (arr[7]!=0) << 0);
}
