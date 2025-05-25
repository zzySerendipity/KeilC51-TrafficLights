#include <REGX51.H>
#include "Delay.h"
#include <intrins.h>

#define uchar unsigned char
#define uint unsigned int
#define out P2

sbit RS = P1^0;
sbit RW = P1^1;
sbit E  = P1^2;

void check_busy(void)
{
	uchar dt;
	do{
		dt = 0xFF;
		E = 0;
		RS = 0;
		RW = 1;
		E = 1;
		dt = out;
	}while(dt&0x80);
	E = 0;
}

void write_command(uchar com)
{
	check_busy();
	E = 0;
	RS = 0;
	RW = 0;
	out = com;
	E = 1;
	_nop_();
	E = 0;
	Delay(1);
}

void write_data(uchar dat)
{
	check_busy();
	E = 0;
	RS = 1;
	RW = 0;
	out = dat;
	E = 1;
	_nop_();
	E = 0;
	Delay(1);
}

void lcd_initial(void)
{
	write_command(0x38);
	write_command(0x0C);
	write_command(0x06);
	write_command(0x01);
	Delay(1);
}

void string(uchar ad, uchar *s)
{
	write_command(ad);
	while(*s > 0)
	{
		write_data(*s++);
		Delay(100);
	}
}