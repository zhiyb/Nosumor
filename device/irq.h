#ifndef IRQ_H
#define IRQ_H

#define NVIC_PRIORITY_GROUPING	3	// 4+4 bits

#define NVIC_PRIORITY_PVD	0, 0

#define NVIC_PRIORITY_USB	4
#define NVIC_PRIORITY_SYSTICK	6
#define NVIC_PRIORITY_KEYBOARD	10

#endif // IRQ_H
