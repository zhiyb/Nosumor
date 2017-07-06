#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stm32f722xx.h>
#include "debug.h"
#include "escape.h"
#include "clock.h"
#include "fio.h"
#include "irq.h"
#include "macros.h"
#include "systick.h"
#include "peripheral/uart.h"
#include "peripheral/audio.h"
#include "usb/usb.h"
#include "usb/keyboard/keyboard.h"

static usb_t usb;

// RGB2_RGB:	PA0(R), PA2(G), PA1(B)
// RGB1_RGB:	PA11(R), PA15(G), PA10(B)
// RGB1_LR:	PB14(L), PB15(R)

void usart6_init()
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOC, 6, 0b10);
	GPIO_MODER(GPIOC, 7, 0b10);
	GPIO_OTYPER_PP(GPIOC, 6);
	GPIO_OSPEEDR(GPIOC, 6, 0b00);	// Low speed (4MHz)
	GPIO_PUPDR(GPIOC, 7, GPIO_PUPDR_UP);
	// AF8: USART6
	GPIO_AFRL(GPIOC, 6, 8);
	GPIO_AFRL(GPIOC, 7, 8);
	uart_init(USART6);
	uart_config(USART6, 115200);
	fio_setup(uart_putc, uart_getc, USART6);
}

static inline void init()
{
	rcc_init();
	NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);
	__enable_irq();
	systick_init(1000);
	usart6_init();

	puts(ESC_CLEAR ESC_MAGENTA VARIANT " build @ " __DATE__ " " __TIME__);
	printf(ESC_YELLOW "Core clock: " ESC_WHITE "%lu\n", clkAHB());

	puts(ESC_CYAN "Initialising keyboard...");
	keyboard_init();

	puts(ESC_CYAN "Initialising audio...");
	audio_init();

	puts(ESC_CYAN "Initialising USB HS...");
	usb_init(&usb, USB_OTG_HS);
	printf(ESC_YELLOW "USB in " ESC_WHITE "%s" ESC_YELLOW " mode\n",
	       usb_mode(&usb) ? "host" : "device");
	while (usb_mode(&usb) != 0);
	usb_keyboard_init(&usb);
	usb_init_device(&usb);

	puts(ESC_CYAN "Initialising LEDs...");
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
	// 01: General purpose output mode
	GPIO_MODER(GPIOA, 0, 0b01);
	GPIO_MODER(GPIOA, 1, 0b01);
	GPIO_MODER(GPIOA, 2, 0b01);
	GPIOA->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
	GPIO_MODER(GPIOB, 14, 0b01);
	GPIO_MODER(GPIOB, 15, 0b01);
	GPIOB->ODR |= (GPIO_ODR_ODR_14 | GPIO_ODR_ODR_15);
	GPIOB->ODR &= ~(GPIO_ODR_ODR_15);
	GPIOB->ODR &= ~(GPIO_ODR_ODR_14);
	GPIO_MODER(GPIOA, 10, 0b01);
	GPIO_MODER(GPIOA, 11, 0b01);
	GPIO_MODER(GPIOA, 15, 0b01);
	GPIOA->ODR &= ~(GPIO_ODR_ODR_10 | GPIO_ODR_ODR_11 | GPIO_ODR_ODR_15);

	puts(ESC_GREEN "Initialisation done");
	usb_connect(&usb, 1);
}

int main()
{
	init();
	uint32_t s;
#if 0
	int i = 1;
	while (!((s = keyboard_status()) & keyboard_masks[2])) {
		//GPIOA->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
		GPIOA->ODR &= ~(GPIO_ODR_ODR_2);
		systick_delay(i << 1);
		GPIOA->ODR |= (GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
		GPIOB->ODR &= ~(GPIO_ODR_ODR_14);
		systick_delay(i);
		GPIOB->ODR |= (GPIO_ODR_ODR_14);
		GPIOB->ODR &= ~(GPIO_ODR_ODR_15);
		systick_delay(i);
		GPIOB->ODR |= (GPIO_ODR_ODR_15);
		if (s & keyboard_masks[3])
			i++;
		if (s & keyboard_masks[4])
			i = i == 0 ? 0 : i - 1;
		if (s)
			while (keyboard_status());
	}
#endif
#ifdef DEBUG
	uint32_t mask = keyboard_masks[2] | keyboard_masks[3] | keyboard_masks[4];
#endif
	for (;;) {
		s = keyboard_status();
		if (s & keyboard_masks[0])
			GPIOB->ODR &= ~GPIO_ODR_ODR_15;
		else
			GPIOB->ODR |= GPIO_ODR_ODR_15;
		if (s & keyboard_masks[1])
			GPIOB->ODR &= ~GPIO_ODR_ODR_14;
		else
			GPIOB->ODR |= GPIO_ODR_ODR_14;
		if (s & keyboard_masks[2])
			GPIOA->ODR &= ~GPIO_ODR_ODR_0;
		else
			GPIOA->ODR |= GPIO_ODR_ODR_0;
		if (s & keyboard_masks[3])
			GPIOA->ODR &= ~GPIO_ODR_ODR_2;
		else
			GPIOA->ODR |= GPIO_ODR_ODR_2;
		if (s & keyboard_masks[4])
			GPIOA->ODR &= ~GPIO_ODR_ODR_1;
		else
			GPIOA->ODR |= GPIO_ODR_ODR_1;
#ifdef DEBUG
		if ((s & mask) == mask) {
			usb_connect(&usb, 0);
			while (keyboard_status());
			usb_connect(&usb, 1);
		}
#endif
	}
	return 0;
}
