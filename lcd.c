/*
 * lcd.c
 *
 * Created: 2025-08-20 오후 1:16:10
 *  Author: COMPUTER
 */ 
#define F_CPU	14745600UL
#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"

void Port_Init(void)
{
	DDRG = 0x0f;
	DDRA = 0xff;
}

void LCD_Data(Byte ch)
{
	LCD_CTRL |= (1 << LCD_RS);
	LCD_CTRL &= ~(1 << LCD_RW);
	LCD_CTRL |= (1 << LCD_EN);
	_delay_us(100);
	LCD_WDATA = ch;
	_delay_us(100);
	LCD_CTRL &= ~(1 << LCD_EN);
}

// LCD에 명령어 전송
void LCD_Comm(Byte ch)
{
	LCD_CTRL &= ~(1 << LCD_RS);
	LCD_CTRL &= ~(1 << LCD_RW);
	LCD_CTRL |= (1 << LCD_EN);
	_delay_us(100);
	LCD_WINST = ch;
	_delay_us(100);
	LCD_CTRL &= ~(1 << LCD_EN);
}

void LCD_CHAR(Byte c)
{
	LCD_Data(c);
	_delay_ms(1);
}

void LCD_STR(Byte*str)
{
	while(*str != 0)
	{
		LCD_CHAR(*str);
		str++;
	}
}

void LCD_pos(unsigned char col, unsigned char row)
{
	LCD_Comm(0x80 | (col + row * 0x40));
}

void LCD_Clear(void)
{
	LCD_Comm(0x01);
	_delay_ms(5);
}

// LCD 초기화
void LCD_Init(void)
{
	_delay_ms(50);
	LCD_Comm(0x38);
	_delay_ms(2);
	LCD_Comm(0x38);
	_delay_ms(2);
	LCD_Comm(0x38);
	_delay_ms(2);
	LCD_Comm(0x0e);
	_delay_ms(2);
	LCD_Comm(0x06);
	_delay_ms(2);
	LCD_Clear();
}
