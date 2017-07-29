#include	"tm1638.h"
#include    <intrins.h>

unsigned char num[8];		//各个数码管显示的值

void init()
{
	unsigned char i;
	init_TM1638();	                           //初始化TM1638
	for(i=0;i<8;i++)
	Write_DATA(i<<1,tab[i]);	               //初始化寄存器
}

void delay(int ms)
{
    int i, j;
    for(i = 0;i<ms;i++)
        for(j=0;j<100;j++)
            ;
}

void test_all()
{
	unsigned char i;
	while(1)
	{
		i=Read_key();                          //读按键值
		if(i<8)
		{
			num[i]++;
			while(Read_key()==i);		       //等待按键释放
			if(num[i]>15)
			num[i]=0;
			Write_DATA(i*2,tab[num[i]]);
			Write_allLED(1<<i);
		}
	}
}

// 流水灯，用到8位数码管，8个LED
// 同时数码管显示二进制位的值
void test_flash_led()
{
    unsigned char i, j;
    unsigned char v;
    v = 0xfe;//1111 1110
    while(1)
    {
        for(i=0;i<8;i++)
        {  
            for(j=0;j<8;j++)
                Write_DATA(j<<1, tab[(v & (1 << j))>>j]);
            Write_allLED(v);
            v = _crol_(v,1);
            //v =(v << 1) | (v >> 7);
            //v = _cror_(v,1);
            //v = (v >> 1) | (v << 7)
            delay(500);
        }
    }
}

int main(void)
{
    init();
    test_flash_led();
}

