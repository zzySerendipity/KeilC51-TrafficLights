#include <REGX51.H>
#include "Delay.h"
#include "LCD.h"

#define uchar unsigned char
#define uint unsigned int
	
sbit Emergency = P3^2;
sbit      Test = P1^3;
bit    Emergency_Flag;
	
// 初始化串口（11.0592MHz晶振，波特率9600）
void UART_Init() 
{
    TMOD &= 0x0F;       // 清除定时器1模式位
    TMOD |= 0x20;       // 定时器1设为模式2（8位自动重载）
    TH1 = 0xFD;         // 波特率9600（计算公式：(256 - TH1) * 12 / 11.0592MHz = 波特率周期）
    TL1 = 0xFD;
    TR1 = 1;            // 启动定时器1
    
    // 串口控制寄存器配置
    SCON = 0x50;        // 模式1（8位UART），允许接收（REN=1）
    PCON &= 0x7F;       // SMOD=0（波特率不倍增）
    
    // 不开启中断（纯发送示例）
    ES = 0;             // 禁用串口中断
    EA = 1;             // 全局中断使能（可选）
}

void UART_SendChar(unsigned char ch) 
{
    SBUF = ch;          // 写入发送缓冲区
    while(!TI);         // 等待发送完成（TI=1）
    TI = 0;             // 手动清除发送中断标志
}

void EmergencyState() 
{
	string(0x83,"Emergency!");
	string(0xC0,"Send Signal ....");
	Delay(100);
	write_command(0x01);
	Delay(100);	
}
void DefaultState() 
{
	string(0x81,"Traffic Light.");
	string(0xC1,"Center Control");
	Delay(100);
	write_command(0x01);
	Delay(100);		
}


void main(void)
{
	lcd_initial();
	UART_Init();
	IP   |= 0x01;				// 中断优先级 INT0 > TIMER0
	IT0 = 1;					// 下降沿
	EX0 = 1;					// 允许外部中断0
	EA  = 1; 					// 中断总允许
	Test = 0;
	Emergency_Flag = 0;
	while(1)
	{
		if(Emergency_Flag){
			EmergencyState();
		}
		else{
			DefaultState();
		}
	}
}

// 外部中断函数
// 一旦有INT0:Mode的下降沿就会进入,进入后Emergency_Flag置1,进入紧急状态
void Int0_Routine() interrupt 0
{
	Emergency_Flag = ~Emergency_Flag;
	if(Emergency_Flag){
		UART_SendChar('E');		// Emergency
		Test = 1;
	}
	else {
		UART_SendChar('N');		// Normal
		Test = 0;
	}
}
