#ifndef CLOCK_H
#define CLOCK_H

#define SYS_CLK		72000000UL
#define AHB_CLK		(SYS_CLK >> 0)
#define USB_CLK		48000000UL

#define APB_TIM_CLK(clk)	(((clk) == AHB_CLK) ? (clk) : (clk) << 1)
#define APB1_CLK	(AHB_CLK >> 1)
#define APB1_TIM_CLK	APB_TIM_CLK(APB1_CLK)
#define APB2_CLK	(AHB_CLK >> 0)
#define APB2_TIM_CLK	APB_TIM_CLK(APB2_CLK)

#endif // CLOCK_H
