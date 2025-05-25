#include <REGX51.H>
#include "Delay.h"
#define	uchar	unsigned char
#define	uint	unsigned int

sbit	EW_LED2   = P2^3;		// EW_LED2
sbit	EW_LED1   = P2^2;		// EW_LED1
sbit	SN_LED2   = P2^1;		// SN_LED2
sbit	SN_LED1   = P2^0;		// SN_LED1
sbit    SN_Yellow = P1^6; 		// 南北主干道 黄灯
sbit    SN_Red    = P1^7;    	// 南北主干道 红灯
sbit    EW_Yellow = P1^2; 		// 东西次干道 黄灯
sbit    EW_Red    = P1^3;    	// 东西次干道 红灯

sbit 	Mode	  = P3^2;		// 模式引脚开关
sbit    ALTER     = P2^5;		// 道路切换 引脚开关
sbit 	TimeInc   = P2^6;		// 时间+1按键
sbit    TimeDec   = P2^7;		// 时间-1按键

sbit 	Debug	  = P2^4;		// Debug LED 1亮0灭

bit     Flag_SN_Yellow;     // 南北主干道 黄灯 标志位
bit     Flag_EW_Yellow;     // 东西次干道 黄灯 标志位
bit		Emergency_Flag;			
bit 	TimeSetting_Flag;			
char	Time_SN;  			// 南北主干道 计时时间
char	Time_EW;  			// 东西次干道 计时时间

// 默认某条路的红灯时间和绿灯时间相等，仅可调整某条路的计时时间
uchar EW = 5,SN = 5;					// SN EW 道路计时默认时间10s 变量:受用户控制
uchar EW_Init = 2,SN_Init = 2;  		// 初始状态 计时3s进入系统	

// 交通信号灯时间 用于数码管P0指示
uchar code table[10]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};		// 0~~~~9段选

// 交通信号灯控制 用于信号灯P1指示
/* S[0]: 南北道路通行
 * S[1]：南北道路黄灯
 * S[4]: 东西道路通行
 * S[5]: 东西道路黄灯
 * S[8]: 两条道路均为红灯 初态 + Emengency状态
 * S[9]: Emergency 红灯黄灯全亮                */
uchar code S[10]={0x28,0x48,0x18,0x48,0x82,0x84,0x81,0x84,0x88,0xCC};	

// 串口接收
bit UART_Update = 0;
uchar UART_Data;

// 数码管动态显示
// 根据当前时刻的Time_EW Time_SN输出四路交通灯显示模块
void Display(int Time) {
	int high,low;
	
	high = Time/10;
	low = Time%10;
	
    P0 = table[low];
	EW_LED2 = 1;
	Delay(1);
	EW_LED2 = 0;
    P0 = table[high];
	EW_LED1 = 1;
	Delay(1);
	EW_LED1 = 0;

	high = Time/10;
	low = Time%10;
	
	P0 = table[low];
	SN_LED2 = 1;
	Delay(1);
	SN_LED2 = 0;
    P0 = table[high];
	SN_LED1 = 1;
	Delay(1);
	SN_LED1 = 0;
} 
void Display_TimeSetting(int Main, int Sub){
	int high,low;
	
	high = Sub/10;
	low = Sub%10;
	
    P0 = table[low];
	EW_LED2 = 1;
	Delay(1);
	EW_LED2 = 0;
    P0 = table[high];
	EW_LED1 = 1;
	Delay(1);
	EW_LED1 = 0;

	high = Main/10;
	low = Main%10;
	
	P0 = table[low];
	SN_LED2 = 1;
	Delay(1);
	SN_LED2 = 0;
    P0 = table[high];
	SN_LED1 = 1;
	Delay(1);
	SN_LED1 = 0;
}

void Init() {
    TMOD= 0x01;					// TIEMR Mode 1
	TH0 = (65536-50000)/256;	// 约0.050s
	TL0 = (65536-50000)%256;
	ET0 = 1;					// 定时中断
    
	IP   |= 0x01;				// 中断优先级 INT0 > TIMER0
	IT0 = 1;					// 下降沿
	EX0 = 1;					// 允许外部中断0
	EA  = 1; 					// 中断总允许
	
	TR0 = 1;					// 启动定时	
	
	Time_EW = EW_Init;
	Time_SN = SN_Init;	
	Emergency_Flag = 0;
	TimeSetting_Flag = 0;
	
    // UART初始化（波特率9600）
    SCON = 0x50;        // 模式1，允许接收
    TMOD |= 0x20;       // 定时器1模式2
    TH1 = 0xFD;         // 9600@11.0592MHz
    TL1 = 0xFD;
    TR1 = 1;            // 启动定时器1
    ES = 1;             // 允许串口中断
}
	
void main()
{ 
	Init();
	Debug = 0;
	// 3s进入 交通灯系统
	while((Time_SN >= 0) || (Time_EW >= 0))
	{ 
		Flag_EW_Yellow = 0;	   		// EW 关黄灯显示信号
		Flag_SN_Yellow = 0;			// SN 关黄灯显示信号
		P1 = S[8];					// 初态: 所有路口红灯
		Display(Time_SN);			// 显示初态的3s倒计时
	}
	while(1)			 
	{
		
        // 紧急状态处理（保持全红）
        if(Emergency_Flag) {
            P1 = S[9];         		// Emergency状态
            // Flag_SN_Yellow = 0;
            // Flag_EW_Yellow = 0;
            Display(Time_SN);   	// 显示当前冻结时间
            continue;          		// 跳过正常流程
        }
		// 修改时间（等待本轮结束后才可以调整时间）
		else if(Emergency_Flag == 0 && TimeSetting_Flag){
			P1 = 0x00;					// 交通灯全灭
			Flag_EW_Yellow = 0;	   		// EW 关黄灯显示信号
			Flag_SN_Yellow = 0;			// SN 关黄灯显示信号
			TimeInc = 1;				// +1 -1 按键重置
			TimeDec = 1;
			Time_EW = EW;		
			Time_SN = SN;
			if(ALTER){		// 修改 南北主干道 时间
				if(TimeInc == 0){
					Delay(10);while(TimeInc==0);Delay(10);SN += 1;
				}
				if(TimeDec == 0){
					Delay(10);while(TimeDec==0);Delay(10);SN -= 1;
				}
			}
			else{			// 修改 东西次干道 时间
				if(TimeInc == 0){
					Delay(10);while(TimeInc==0);Delay(10);EW += 1;
				}
				if(TimeDec == 0){
					Delay(10);while(TimeDec==0);Delay(10);EW -= 1;
				}				
			}
			Display_TimeSetting(SN, EW);
		}
		
		// 正常运行状态 
		else{
			Time_SN = SN;
			Time_EW = EW;	
			
			while(Time_SN >= 4) { 
				Flag_EW_Yellow = 0;	   	//EW关黄灯显示信号
				P1 = S[0];	 			//SN通行，EW红灯
				Display(Time_SN);
			}
			P1 = 0x00;
			while(Time_SN >= 0) {
				Flag_SN_Yellow = 1;	 	//SN开黄灯信号位
				EW_Red = 1;      		//SN黄灯亮,等待左转信号，EW红灯
				Display(Time_SN);
			}
			Time_SN = SN;
			Time_EW = EW;	
			while(Time_EW >= 4) {
				Flag_SN_Yellow = 0;  	//SN关黄灯显示信号			  
				P1 = S[4];	 			//EW通行，SN红灯
				Display(Time_EW);
			}
			P1 = 0x00;
			while(Time_EW >= 0) {
				Flag_EW_Yellow = 1;		//EW开黄灯信号位
				SN_Red = 1;				//EW黄灯亮，等待左拐信号，SN红灯	
				Display(Time_EW);
			}				
		}
	} 
}

// 外部中断函数
// 偶尔跳入 不需要函数精简 中断期间定时器可开也可关闭
// 一旦有INT0:Mode的下降沿就会进入,进入后TimeSetting_Flag置1,进入调时状态
void Int0_Routine() interrupt 0
{
	TimeSetting_Flag = ~TimeSetting_Flag;
	if(TimeSetting_Flag){
		Debug = 1;
	}
	else {
		Debug = 0;
	}
}

// 定时器中断函数
// 频繁跳入 需要中断函数精简
void timer0() interrupt 1 using 1
{
	static uchar count;
	TH0 = (65536-50000) / 256;
	TL0 = (65536-50000) % 256;		// 约0.05s溢出一次
	count++;
	
	if(count == 10) {
		if(Flag_SN_Yellow == 1)			// 南北黄灯标志
			SN_Yellow = ~SN_Yellow;
		if(Flag_EW_Yellow == 1)			// 东西黄灯标志
			EW_Yellow = ~EW_Yellow;
	}
	if(count == 20) {						// 约1s
		Time_EW--;
		Time_SN--;
		if(Flag_SN_Yellow == 1)			// 南北黄灯标志
			SN_Yellow = ~SN_Yellow;
		if(Flag_EW_Yellow == 1)			// 东西黄灯标志
			EW_Yellow = ~EW_Yellow;
		count = 0;
	}
}

// 新增串口中断服务函数
void UART_ISR() interrupt 4 {
    if(RI) {
        RI = 0;                 // 清除接收中断标志
        UART_Data = SBUF;       // 获取接收数据
        
        // 根据接收数据设置标志位
        if(UART_Data == 'E') {
            Emergency_Flag = 1;
            P1 = S[9];
            TR0 = 0;           // 暂停定时器
        }
        else if(UART_Data == 'N') {
            Emergency_Flag = 0;
            TR0 = 1;           // 恢复定时器
        }
    }
}