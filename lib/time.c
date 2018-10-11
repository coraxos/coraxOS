#include "../include/types.h"

#define CMOS_PORT 0x70
#define CMOS_PORT_INOUT 0x71
#define IRQ_PIT 0x20

char read_cmos(u8 cmos_reg) {
	write_port(CMOS_PORT, cmos_reg); //Must always reselect before reading because it seems reading clears the selection
	return read_port(CMOS_PORT_INOUT);
}

void print_time() {
	/* From https://wiki.osdev.org/CMOS#Accessing_CMOS_Registers
	Register  Contents
	0x00      Seconds
 	0x02      Minutes
	0x04      Hours
	0x06      Weekday
	0x07      Day of Month
	0x08      Month
	0x09      Year
	0x32      Century (maybe)
	0x0A      Status Register A
	0x0B      Status Register B
	*/
	u64 value = (u64) read_cmos(0);
	kpanic_fmt("%d\n", value);
}