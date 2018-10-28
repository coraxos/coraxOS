#pragma once
#include "types.h"

#define ASCII_NUMBER_CONST 0x30
#define ASCII_LETTER_CONST 0x57

u32 strlen(char *str);
void reverse(char *str);
void itoa(u32 number, char *str, u32 base);
void strcpy_max_len(char *src, char *dest, uint32_t maxLen);

static char *exceptions_string[32] = {
	"Divide-by-zero",
	"Debug",
	"Non-maskable Interrupt",
	"Breakpoint",
	"Overflow",
	"Bound Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack-Segment Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved",
	"x87 Floating-Point Exception",
	"Alignment Check",
	"Machine Check",
	"SIMD Floating-Point Exception",
	"Virtualization Exception", // 0x14
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Security Exception", // 30 (0x1E)
	"Reserved" // 31 (0x1F)
};

static char *interrupts_string[32] = { "Programmable Interrupt Timer",
				       "Keyboard",
				       "Cascade",
				       "Com2",
				       "Com1",
				       "LPT2",
				       "Floppy disk",
				       "LPT1",
				       "CMOS Real time clock",
				       "Peripherals",
				       "Peripherals",
				       "Peripherals",
				       "PS2 Mouse",
				       "FPU/Coprocessor",
				       "Primary ATA Hard Disk",
				       "Secondary ATA Hard Disk" };