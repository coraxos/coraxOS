#include "types.h"
#include "serial.h"

struct DebugEntry {
    uint32_t address;
    char *func_name;
    char *file;
};

//TODO, this is not maintainable, have it be autogenerated or read from ELF file
struct DebugEntry entries[] = {
{ 0xc010000c, "asm_lgdt", "boot/boot.asm:25" },
{ 0xc0100014, "tss_flush", "boot/boot.asm:31" },
{ 0xc010001c, "load_idt", "boot/boot.asm:39" },
{ 0xc0100025, "start", "boot/boot.asm:64" },
{ 0xc010004a, "LoadNewPageDirectory", "boot/boot.asm:79" },
{ 0xc0100052, "DisablePSE", "boot/boot.asm:84" },
{ 0xc010005c, "StartHigherHalf", "boot/boot.asm:90" } //Note: there are more here but this has to be changed before use
};

uint32_t get_function(uint32_t ip) {
    for(uint32_t x = 0; x < (sizeof(entries) / sizeof(entries[0])) - 1; x++) {
        if(entries[x].address <= ip && entries[x + 1].address >= ip)
        {
            return entries[x].address;
        }
    }
    return entries[0].address; //TODO, sensible default
}
void get_func_info(uint32_t addr, char **name, char **file) {
    for(uint32_t x = 0; x < (sizeof(entries) / sizeof(entries[0])); x++) {
        if(entries[x].address == addr) {
            *name = entries[x].func_name;
            *file = entries[x].file;
            break;
        }
    }
}

extern uint32_t* get_ebp();
void dump_stack_trace() {
    uint32_t *ebp = get_ebp();

    kpanic_fmt("Backtrace:\n");

    for(uint32_t x = 0; x < 15; x++) {
        uint32_t retIP = *(ebp + 1);

        if(retIP == 0) { //We finished. See boot.asm where we push 0
            break;
        }

        uint32_t func_addr = get_function(retIP);

        char *func_name;
        char *func_file;

        get_func_info(func_addr, &func_name, &func_file);

        kpanic_fmt("%d: %s in %s\n", x, func_name, func_file);   

        ebp = (uint32_t*) *ebp; //Move up
    }
}