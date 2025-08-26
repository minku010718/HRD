/*
 * lcd.h
 *
 * Created: 2025-08-20 오후 1:15:49
 *  Author: COMPUTER
 */ 


#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// 핀 설정
#define LCD_WDATA PORTA
#define LCD_WINST PORTA
#define LCD_CTRL PORTG
#define LCD_EN 2
#define LCD_RW 1
#define LCD_RS 0
#define Byte unsigned char
#define On 1
#define Off 0

void Port_Init(void);
void LCD_Data(Byte ch);
void LCD_Comm(Byte ch);
void LCD_CHAR(Byte c);
void LCD_STR(Byte*str);
void LCD_pos(unsigned char col, unsigned char row);
void LCD_Clear(void);
void LCD_Init(void);




#endif /* LCD_H_ */