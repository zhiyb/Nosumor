#ifndef MACROS_H
#define MACROS_H

#include <stm32f7xx.h>

#define PACKED	__attribute__((__packed__))
#define FIELD(r, f)	(((r) & (f##_Msk)) >> (f##_Pos))
#define ROUND(a, b)	((a + (b / 2)) / b)
#define ASIZE(a)	(sizeof((a)) / sizeof((a)[0]))

#define GPIO_AFRL_AFR0_Msk	GPIO_AFRL_AFRL0_Msk
#define GPIO_AFRL_AFR1_Msk	GPIO_AFRL_AFRL1_Msk
#define GPIO_AFRL_AFR2_Msk	GPIO_AFRL_AFRL2_Msk
#define GPIO_AFRL_AFR3_Msk	GPIO_AFRL_AFRL3_Msk
#define GPIO_AFRL_AFR4_Msk	GPIO_AFRL_AFRL4_Msk
#define GPIO_AFRL_AFR5_Msk	GPIO_AFRL_AFRL5_Msk
#define GPIO_AFRL_AFR6_Msk	GPIO_AFRL_AFRL6_Msk
#define GPIO_AFRL_AFR7_Msk	GPIO_AFRL_AFRL7_Msk

#define GPIO_AFRL_AFR0_Pos	GPIO_AFRL_AFRL0_Pos
#define GPIO_AFRL_AFR1_Pos	GPIO_AFRL_AFRL1_Pos
#define GPIO_AFRL_AFR2_Pos	GPIO_AFRL_AFRL2_Pos
#define GPIO_AFRL_AFR3_Pos	GPIO_AFRL_AFRL3_Pos
#define GPIO_AFRL_AFR4_Pos	GPIO_AFRL_AFRL4_Pos
#define GPIO_AFRL_AFR5_Pos	GPIO_AFRL_AFRL5_Pos
#define GPIO_AFRL_AFR6_Pos	GPIO_AFRL_AFRL6_Pos
#define GPIO_AFRL_AFR7_Pos	GPIO_AFRL_AFRL7_Pos

#define GPIO_AFRH_AFR8_Msk	GPIO_AFRH_AFRH0_Msk
#define GPIO_AFRH_AFR9_Msk	GPIO_AFRH_AFRH1_Msk
#define GPIO_AFRH_AFR10_Msk	GPIO_AFRH_AFRH2_Msk
#define GPIO_AFRH_AFR11_Msk	GPIO_AFRH_AFRH3_Msk
#define GPIO_AFRH_AFR12_Msk	GPIO_AFRH_AFRH4_Msk
#define GPIO_AFRH_AFR13_Msk	GPIO_AFRH_AFRH5_Msk
#define GPIO_AFRH_AFR14_Msk	GPIO_AFRH_AFRH6_Msk
#define GPIO_AFRH_AFR15_Msk	GPIO_AFRH_AFRH7_Msk

#define GPIO_AFRH_AFR8_Pos	GPIO_AFRH_AFRH0_Pos
#define GPIO_AFRH_AFR9_Pos	GPIO_AFRH_AFRH1_Pos
#define GPIO_AFRH_AFR10_Pos	GPIO_AFRH_AFRH2_Pos
#define GPIO_AFRH_AFR11_Pos	GPIO_AFRH_AFRH3_Pos
#define GPIO_AFRH_AFR12_Pos	GPIO_AFRH_AFRH4_Pos
#define GPIO_AFRH_AFR13_Pos	GPIO_AFRH_AFRH5_Pos
#define GPIO_AFRH_AFR14_Pos	GPIO_AFRH_AFRH6_Pos
#define GPIO_AFRH_AFR15_Pos	GPIO_AFRH_AFRH7_Pos

#define GPIO_MODER(b, p, m)	(b)->MODER = \
	(((b)->MODER & ~GPIO_MODER_MODER##p##_Msk) | ((m) << GPIO_MODER_MODER##p##_Pos))
#define GPIO_AFRL(b, p, m)	(b)->AFR[0] = \
	(((b)->AFR[0] & ~GPIO_AFRL_AFR##p##_Msk) | ((m) << GPIO_AFRL_AFR##p##_Pos))
#define GPIO_AFRH(b, p, m)	(b)->AFR[1] = \
	(((b)->AFR[1] & ~GPIO_AFRH_AFR##p##_Msk) | ((m) << GPIO_AFRH_AFR##p##_Pos))
#define GPIO_OTYPER_PP(b, p)	(b)->OTYPER &= ~GPIO_OTYPER_OT_##p
#define GPIO_OTYPER_OD(b, p)	(b)->OTYPER |= GPIO_OTYPER_OT_##p
#define GPIO_OSPEEDR(b, p, m)	(b)->OSPEEDR = \
	(((b)->OSPEEDR & ~GPIO_OSPEEDER_OSPEEDR##p##_Msk) | \
	((m) << GPIO_OSPEEDER_OSPEEDR##p##_Pos))
#define GPIO_PUPDR(b, p, m)	(b)->PUPDR = \
	(((b)->PUPDR & ~GPIO_PUPDR_PUPDR##p##_Msk) | \
	((m) << GPIO_PUPDR_PUPDR##p##_Pos))
#define GPIO_PUPDR_UP	0b01
#define GPIO_PUPDR_DOWN	0b10

#define EXTICR_EXTI_PA	0u
#define EXTICR_EXTI_PB	1u
#define EXTICR_EXTI_PC	2u
#define EXTICR_EXTI_PD	3u
#define EXTICR_EXTI_PE	4u
#define EXTICR_EXTI_PF	5u
#define EXTICR_EXTI_PG	6u
#define EXTICR_EXTI_PH	7u
#define EXTICR_EXTI_PI	8u
#define EXTICR_EXTI_PJ	9u
#define EXTICR_EXTI_PK	10u

static inline void exticr_exti(uint32_t pin, uint32_t port)
{
	uint32_t pos = ((pin) & 3u) << 2u;
	uint32_t mask = SYSCFG_EXTICR1_EXTI0_Msk << pos;
	__IO uint32_t *io = &SYSCFG->EXTICR[(pin) >> 2u];
	*io = (*io & ~mask) | (port << pos);
}

#endif // MACROS_H
