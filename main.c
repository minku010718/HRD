/*
 * Project1.c
 *
 * Created: 2025-08-22 오전 9:15:46
 * Author : COMPUTER
 * Modified: 2025-08-25
 */

#define F_CPU 14745600UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lcd.h"

#define AUTH_TRIES 5	// 최대 시도 횟수
#define LED_PORT PORTD
#define FAN_PORT PORTC
#define FAN_PIN  0

volatile unsigned char rx_buffer[20];	// UART 수신 데이터 저장할 버퍼
volatile unsigned char rx_index = 0;	// 수신 버퍼의 위치
volatile int rx_complete_flag = 0;		// 데이터 수신 완료 여부
unsigned char led_status = 0x00;		// LCD 상태 저장
char lcd_buf[20];						// LCD 출력을 위한 공용 버퍼

// 등록된 사번 목록 
const char* valid_ids[] = {"6155", "4421", "6894"};

void system_init(void);
void uart_init(unsigned int baud);
void uart_tx(unsigned char data);
void uart_tx_string(const char* str);
int authenticate_user(void);
void show_menu(void);
void wait_for_uart_input(void);
void handle_menu(void);
void turn_on_led(void);
void turn_off_led(void);
void start_fan(void);
void stop_fan(void);
void exit_system(void);
void print_led_status(void);

ISR(USART0_RX_vect);	// UART0 수신 인터럽트 핸들러

int main(void)
{
    system_init();

    LCD_pos(0, 0);
    LCD_STR((Byte*)"Factory System");
    uart_tx_string("Factory System Boot.\r\n");

    if (authenticate_user() == 0) {		// 0을 반환시 인증 실패, 실패 5회시 잠금
        LCD_Clear();
        LCD_pos(0, 0);
        LCD_STR((Byte*)"System Locked");
        uart_tx_string("\r\nAuthentication failed. System locked.\r\n");
        while (1);
    }
	// 인증 성공
    LCD_Clear();
    LCD_pos(0, 0);
    LCD_STR((Byte*)"Login Success!");
    uart_tx_string("\r\nLogin Success!\r\n");
    _delay_ms(1500);

    while (1)
    {
        handle_menu();	// 메뉴를 보여주고 입출력 처리
    }
}

void system_init(void)
{
    DDRA = 0xFF;
    DDRG = 0x0F;
    DDRD = 0xFF;				// LED 포트 (출력)
    DDRC = (1 << FAN_PIN);		// FAN 핀 (출력)

    PORTD = 0x00;				// 모든 LED 끄기
    PORTC &= ~(1 << FAN_PIN);	// 팬 끄기
    
    LCD_Init();
    uart_init(9600);
    sei();
}

void handle_menu(void)
{
    char menu_choice;
    
    show_menu();							// 메뉴 출력
    wait_for_uart_input();					// 데이터 입력시까지 대기
    
    menu_choice = rx_buffer[0];

    switch(menu_choice)
    {
        case '1': turn_on_led(); break;		// 1. LED 켜기
        case '2': start_fan(); break;		// 2. 팬 켜기
        case '3': turn_off_led(); break;	// 3. LED 끄기
        case '4': stop_fan(); break;		// 4. 팬 끄기
        case '5': exit_system(); break;		// 5. 시스템 종료
        default:							// 그 외: 오류 메시지 출력
            uart_tx_string("\r\nInvalid option. Please try again.\r\n");
            LCD_Clear();
            LCD_pos(0, 0);
            LCD_STR((Byte*)"Invalid Option");
            _delay_ms(1500);
            break;
    }
}

// 1. 전등(LED) 켜기
void turn_on_led(void)
{
    int led_num;
    
    LCD_Clear();
    LCD_pos(0, 0);
    LCD_STR((Byte*)"Turn On LED");
    
    uart_tx_string("\r\n--- Turn On LED ---\r\n");
    print_led_status();
    uart_tx_string("Enter LED number to turn ON (1-8): ");

    wait_for_uart_input();
    led_num = atoi((const char*)rx_buffer);					// 입력받은 문자열을 정수로 변환

	// 입력 값이 1~8 사이인 경우
    if (led_num >= 1 && led_num <= 8) {
        led_status |= (1 << (led_num - 1));
        LED_PORT = led_status;
        
        // 출력 메시지 
        sprintf(lcd_buf, "LED %d -> ON", led_num);
        LCD_Clear();
        LCD_pos(0, 0);
        LCD_STR((Byte*)lcd_buf);
        
        uart_tx_string("\r\nOK. LED turned ON.\r\n");
        
    } else {
		// 잘못된 번호 입력
        uart_tx_string("\r\nInvalid order.\r\n");
        LCD_pos(1, 0);
        LCD_STR((Byte*)"Invalid Order");
    }
    _delay_ms(1500);
}

// 2. 환기하기(Mini Fan 켜기)
void start_fan(void)
{
	// FAN 비트의 위치를 1로 설정
    FAN_PORT |= (1 << FAN_PIN);
    
    // 출력 메시지
    LCD_Clear();
    LCD_pos(0, 0);
    LCD_STR((Byte*)"FAN ON");
    
    uart_tx_string("\r\n--- FAN ON ---\r\n");
    _delay_ms(1500);
}

// 3. 전등(LED) 끄기
void turn_off_led(void)
{
    int led_num;

    LCD_Clear();
    LCD_pos(0, 0);
    LCD_STR((Byte*)"Turn Off LED");

    uart_tx_string("\r\n--- Turn Off LED ---\r\n");
    print_led_status();
    uart_tx_string("Enter LED number to turn OFF (1-8): ");

    wait_for_uart_input();
    led_num = atoi((const char*)rx_buffer);					// 입력받은 문자열을 정수로 변환
	
	// 입력값이 1~8사이인 경우
    if (led_num >= 1 && led_num <= 8) {
        led_status &= ~(1 << (led_num - 1));
        LED_PORT = led_status;
        
        // 출력 메시지
        sprintf(lcd_buf, "LED %d -> OFF", led_num);
        LCD_Clear();
        LCD_pos(0, 0);
        LCD_STR((Byte*)lcd_buf);
        
        uart_tx_string("\r\nOK. LED turned OFF.\r\n");

    } else {
		// 잘못된 입력의 경우
        uart_tx_string("\r\nInvalid order.\r\n");
        LCD_pos(1, 0);
        LCD_STR((Byte*)"Invalid Order");
    }
    _delay_ms(1500);
}

// 4. 환기 중지(Mini Fan 끄기)
void stop_fan(void)
{
	// FAN 비트의 위치를 0으로 설정
    FAN_PORT &= ~(1 << FAN_PIN);
    
    // 출력 메시지
    LCD_Clear();
    LCD_pos(0, 0);
    LCD_STR((Byte*)"FAN OFF");

    uart_tx_string("\r\n--- FAN OFF ---\r\n");
    _delay_ms(1500);
}

// 5. 퇴실(시스템 종료)
void exit_system(void)
{
	// 모든 장치 종료
    led_status = 0x00;
    LED_PORT = led_status;
    FAN_PORT &= ~(1 << FAN_PIN);

    LCD_Clear();
    LCD_pos(0, 0);
    LCD_STR((Byte*)"System Shutdown");
    uart_tx_string("\r\nExiting system. All works are OFF.\r\n");
    
    cli();
    while(1);
}

// 현재 켜진 LED의 상태 출력 함수
void print_led_status(void)
{
    char buf[50];
    int count = 0;
    uart_tx_string("Currently ON: ");
    for (int i = 0; i < 8; i++) {
        if ((led_status >> i) & 1) {
            sprintf(buf, "%d ", i + 1);
            uart_tx_string(buf);
            count++;
        }
    }
    if (count == 0) uart_tx_string("None");
    uart_tx_string("\r\n");
}

// UART 입력 대기 함수
void wait_for_uart_input(void)
{
    rx_complete_flag = 0;
    while(rx_complete_flag == 0);
}

// 메뉴 표시 함수
void show_menu(void)
{
	LCD_Clear();
	LCD_pos(0, 0);
	
	LCD_STR((Byte*)"---Main Menu---");
	
	uart_tx_string("\r\n--- Main Menu ---\r\n");
	uart_tx_string("1. Turn on light\r\n2. Start fan\r\n");
	uart_tx_string("3. Turn off light\r\n4. Stop fan\r\n");
	uart_tx_string("5. Exit\r\nOrder: ");
}

// 사용자 인증 함수
int authenticate_user(void)
{
    int tries = 0;
    int num_valid_ids = sizeof(valid_ids) / sizeof(valid_ids[0]);
	// 최대 시도 횟수까지 반복 가능
    while (tries < AUTH_TRIES) {
        LCD_Clear();
        LCD_pos(0, 0);
        LCD_STR((Byte*)"Enter ID: ");
        uart_tx_string("\r\nEnter Employee ID: ");
        wait_for_uart_input();
        int authenticated = 0;
        for (int i = 0; i < num_valid_ids; i++) {
            if (strcmp((const char*)rx_buffer, valid_ids[i]) == 0) {
                authenticated = 1;
                break;
            }
        }
        if (authenticated) {
            return 1;
        } else {
			// 인증 실패시
            tries++;	// 시도 횟수 증가
            uart_tx_string("\r\nInvalid ID. ");
            char temp_buf[30];
			// 남은 시도 횟수 출력
            sprintf(temp_buf, "Tries left: %d\r\n", AUTH_TRIES - tries);
            uart_tx_string(temp_buf);
            LCD_pos(0, 0);
            LCD_STR((Byte*)"Invalid ID!");
            _delay_ms(1500);
        }
    }
    return 0;
}

// UART 초기화 함수
void uart_init(unsigned int baud)
{
    unsigned int ubrr = F_CPU / 16UL / baud - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// UART 수신 완료 인터럽트 루틴
// UART로 1바이트가 수신될 때 마다 실행
ISR(USART0_RX_vect)
{
    unsigned char data = UDR0;					// 수신된 1바이트 데이터 읽기
    if (data == '\r' || data == '\n') {			// 수신 문자 확인
        if (rx_index > 0) {						// 입력 내용이 있으면 수신 완료 하고 인덱스 초기화
            rx_buffer[rx_index] = '\0';
            rx_complete_flag = 1;
            rx_index = 0;						// 다음 수신을 위해 인덱스 초기화
        }
    } else {	
		// 버퍼 크기를 넘지 않을 때만 데이터 저장
        if (rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = data;		// 버퍼에 데이터 저장 후 인덱스 증가
        }
    }
}

// UART로 1바이트 데이터 송신
void uart_tx(unsigned char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// UART로 문자열 송신
void uart_tx_string(const char* str)
{
    while (*str) {
        uart_tx(*str++);
    }
}