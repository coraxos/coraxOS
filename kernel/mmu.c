#include "types.h"
#include "p_allocator.h"
#include "mmu.h"
#include "serial.h"
#include "trap.h"

extern void LoadNewPageDirectory(uint32_t pd);
extern void DisablePSE();
extern char _kernel_end;

void page_fault_handler(int error_no);

extern page_directory_t *boot_page_directory;
page_directory_t *current_directory;

int PAGING_INIT = 0;
/*
 * Returns the physical address
 * 
 * If paging has not been initialized (PAGING_INIT = 0), then this method returns ONLY the address of frames on the heap
 */
pptr_t virtual_to_physical(page_directory_t *pgdir, vptr_t addr)
{
	if(!PAGING_INIT) {
		//Initial heap is right after the kernel_end
		return (addr - KERN_HEAP_START) + ((size_t)&_kernel_end - KERN_BASE);
	}
	size_t pdx = PDX(addr);
	size_t ptx = PTX(addr);

	return (pgdir->tables[pdx]->pages[ptx].frame << POFFSHIFT) | (addr & 0xFFF);
}

//Just map it, we don't care where
void mmap(size_t base, size_t len)
{
	assert((base & PGSIZE) == 0); //Has to be 4k aligned
	//len = PG_ROUND_UP(len);
	for (size_t x = 0; x < len / PGSIZE; x++) {
		vptr_t vaddr = base + x * PGSIZE;
		pptr_t phyaddr = alloc_frame();
		setPTE(current_directory, vaddr, phyaddr, 0);
	}
}

//Map it to a specific address, ex. for the kernel code or MMIO
void mmap_addr(vptr_t vaddr_start, pptr_t phyaddr_start, size_t len,
	       uint8_t flags)
{
	assert((vaddr_start & 0xFFF) == 0); //Has to be 4k aligned
	assert((phyaddr_start & 0xFFF) == 0); //Has to be 4k aligned

	size_t vaddr_pdx = PDX(vaddr_start);
	size_t vaddr_ptx = PTX(vaddr_start);

	//Mark all the pages as used first and then actually create the entries
	//This ensures the pages are contiguous
	//Also, at boot time, it makes sure the kernel code is marked as used, then the
	//page tables are allocated

	//Mark pages used
	for (size_t x = 0; x < len / PGSIZE; x++) {
		pptr_t phyaddr = phyaddr_start + x * PGSIZE;

		frame_set_used(phyaddr, flags & FLAG_IGNORE_PHY_REUSE);
	}

	//Make the page entries
	for (size_t x = 0; x < len / PGSIZE; x++) {
		vptr_t vaddr = vaddr_start + x * PGSIZE;
		pptr_t phyaddr = phyaddr_start + x * PGSIZE;
		setPTE(current_directory, vaddr, phyaddr, 0);
	}
}

void setPTE(page_directory_t *pgdir, vptr_t vaddr, pptr_t phyaddr, int user)
{
	uint32_t pdx = PDX(vaddr);
	uint32_t ptx = PTX(vaddr);

	//Page table does not exist yet so create it
	if (!(pgdir->actual_tables[pdx].present)) {
		vptr_t page = (vptr_t)kvmalloc(PGSIZE);
		memset((void*)page, 0, PGSIZE); //Initialize table

		//Set the pointer in the page directory table
		pgdir->tables[pdx] = (page_table_t*) page;
		pptr_t frame_addr = virtual_to_physical(pgdir, page);

		page_dir_entry_t *pde = &pgdir->actual_tables[pdx];

		pde->user = user;
		pde->frame = frame_addr >> POFFSHIFT;
		pde->present = 1;
	}
	pte_t *page_entry = &pgdir->tables[pdx]->pages[ptx];

	assert(!page_entry->present); //Entry not already mapped

	page_entry->frame = PTE_ADDR(phyaddr); //Set the frame address
	page_entry->user = user;
	page_entry->present = 1;
}

void paging_init(size_t memory_map_base, size_t memory_map_full_len)
{
	frame_init(memory_map_base, memory_map_full_len);
	register_handler(TRAP_PAGE_FAULT, page_fault_handler);

	size_t kernel_end_phy_addr = (size_t)&_kernel_end - KERN_BASE;

	//Mark kernel in use
	for(uint32_t i = 0; i < kernel_end_phy_addr; i += PGSIZE) {
		frame_set_used(i, FLAG_IGNORE_PHY_REUSE);
	}
	//Mark initial 4MB heap in use
	for(uint32_t i = kernel_end_phy_addr; i <= kernel_end_phy_addr + LPGSIZE; i += PGSIZE) {
		frame_set_used(i, FLAG_IGNORE_PHY_REUSE);
	}

	current_directory = (page_directory_t*) kvmalloc(sizeof(page_directory_t));

	//Map kernel code
	mmap_addr(KERN_BASE, 0x0, kernel_end_phy_addr, FLAG_IGNORE_PHY_REUSE);

	//First 4MB of heap are mapped already by the boot_page_directory
	mmap_addr(KERN_HEAP_START, kernel_end_phy_addr, 0x400000, FLAG_IGNORE_PHY_REUSE);
	//Map the rest of it
	mmap(KERN_HEAP_START + 0x400000, KERN_HEAP_END - KERN_HEAP_START - 0x400000);


	LoadNewPageDirectory((size_t)virtual_to_physical(current_directory, (vptr_t)current_directory));
	DisablePSE(); //Disable 4 MiB pages

	PAGING_INIT = 1;
}
void page_fault_handler(int error_no)
{
	static char *page_fault_msgs[] = {
		"Supervisory process tried to read a non-present page entry",
		"Supervisory process tried to read a page and caused a protection fault",
		"Supervisory process tried to write to a non-present page entry",
		"Supervisory process tried to write a page and caused a protection fault",
		"User process tried to read a non-present page entry",
		"User process tried to read a page and caused a protection fault",
		"User process tried to write to a non-present page entry",
		"User process tried to write a page and caused a protection fault"
	};
	fail_stmt_stop("Page Fault Error: %s\n", page_fault_msgs[error_no]);
}
void alloc_page(pte_t *page, int is_user, int is_writable)
{
	page->user = is_user;
	page->rw = is_writable;

	if (page->frame != 0) {
		page->present = 1;
		return;
	}
	pptr_t frame = alloc_frame();
	page->frame = frame >> 12;
	page->present = 1;
}
/*
 * Copies the page directory
 * Keeps the kernel mappings, but copies the user entries/tables
*/
page_directory_t *clone_directory()
{
	/*page_directory_t *new_pg_directory = kvmalloc(sizeof(page_directory_t));


	for (int x = 0; x < 1024; x++) {
		if (current_page_table->pages[x] != 0) {
			new_pg_directory->tables[x] = virtual_to_physical(
				clone_table(&real_page_directory->tables[x]));
		}

		//Keep kernel tables the same
		if (x >= (KERN_BASE / PGSIZE)) {
			new_pg_directory->tables[x] =
				current_page_table->pages[x];
		}
	}

	return new_pg_directory; //TODO*/
	return NULL;
}
static inline void invlpg(vptr_t addr)
{
	asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

void memcpy_frame_contents(pptr_t dst, pptr_t src)
{
	//Map these two pages
	setPTE(current_directory, COPY_PAGE_SOURCE, src, 0); //TODO PAGE_R | PAGE_KERNEL
	setPTE(current_directory, COPY_PAGE_DEST, dst, 0); //TODO PAGE_RW | PAGE_KERNEL

	//invalidate entries
	invlpg(COPY_PAGE_SOURCE);
	invlpg(COPY_PAGE_DEST);

	//copy
	memcpy((void*)COPY_PAGE_DEST, (void*)COPY_PAGE_SOURCE, PGSIZE);

	//TODO
	//remove mappings
	//invalidate entries
}

vptr_t clone_table(page_table_t *table)
{
	page_table_t *newTbl = kmalloc(PGSIZE);

	for (int x = 0; x < 1024; x++) {
		if (!table->pages[x].frame) {
			continue;
		}

		alloc_page(&newTbl->pages[x], table->pages[x].user,
			   table->pages[x].rw);

		memcpy_frame_contents(FRAME_TO_ADDR(newTbl->pages[x].frame),
				      FRAME_TO_ADDR(table->pages[x].frame));
	}

	return (vptr_t)newTbl;
}

//Graphic showing memory layout, modified from https://pdos.csail.mit.edu/6.828/2008/lec/l5.html
/*
        4 Gig -------->  +------------------------------+
                        :              .               :
                        :              .               :
                        :              .               :
                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| 0xE0F00000
                        |   Remapped IO                | RW/--
                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| 0xE0002000
                        | Temporary Copy Page (dest)   | RW/--
  COPY_PAGE_DEST  -->   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| 0xE0001000
                        | Temporary Copy Page (source) | RW/--
  COPY_PAGE_SOURCE -->  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| 0xE0000000
                        |                              | RW/--
                        |   Kernel Heap                | RW/--
                        |                              | RW/--
  KERN_HEAP_START  -->  +------------------------------+ 0xD0000000
                        |                              | RW/--
                        |   Remapped Physical Memory   | RW/--
                        |                              | RW/--
        KERNBASE ----->  +------------------------------+ 0xc0000000
                        |       Empty Memory           | R-/R-  PTSIZE
        UPAGES    ---->  +------------------------------+ 0xef000000
                        |           RO ENVS            | R-/R-  PTSIZE
    UTOP,UENVS ------>  +------------------------------+ 0xeec00000
    UXSTACKTOP -/       |     User Exception Stack     | RW/RW  PGSIZE
                        +------------------------------+ 0xeebff000
                        |       Empty Memory           | --/--  PGSIZE
        USTACKTOP  --->  +------------------------------+ 0xeebfe000
                        |      Normal User Stack       | RW/RW  PGSIZE
                        +------------------------------+ 0xeebfd000
                        |                              |
                        |                              |
                        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                        .                              .
                        .                              .
                        .                              .
                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
                        |     Program Data & Heap      |
        UTEXT -------->  +------------------------------+ 0x00800000
        PFTEMP ------->  |       Empty Memory           |        PTSIZE
                        |                              |
        UTEMP -------->  +------------------------------+ 0x00400000
                        |       Empty Memory           |        PTSIZE
        0 ------------>  +------------------------------+
*/