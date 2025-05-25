#ifndef __LCD_H__
#define __LCD_H__

#define uchar unsigned char
#define uint unsigned int

void lcd_initial(void);
void check_busy(void);
void write_command(uchar com);
void write_data(uchar dat);
void string(uchar ad,uchar *s);

#endif
