#include <rtthread.h>	

__packed typedef struct
{
    uint8_t fontok;				//�ֿ���ڱ�־��0XAA���ֿ��������������ֿⲻ����
    uint32_t ugbkaddr; 			//unigbk�ĵ�ַ
    uint32_t ugbksize;			//unigbk�Ĵ�С
    uint32_t f12addr;			//gbk12��ַ
    uint32_t gbk12size;			//gbk12�Ĵ�С
    uint32_t f16addr;			//gbk16��ַ
    uint32_t gbk16size;			//gbk16�Ĵ�С
    uint32_t f24addr;			//gbk24��ַ
    uint32_t gbk24size;			//gbk24�Ĵ�С
    uint32_t f32addr;			//gbk32��ַ
    uint32_t gbk32size;			//gbk32�Ĵ�С
} _font_info;

