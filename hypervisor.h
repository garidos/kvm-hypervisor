#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <string.h>

#define TERM_PORT   0xe9
#define FILE_PORT   0x278

#define BUFF_SZ     2048

#define GET_VM_FILE_PATH(buffer, dir, fname) 	strcpy(buffer, dir); \
											 	strcat(buffer, fname);

#define PML4_ENTRY(addr)	(( addr >> 39 ) & ENTRY_MASK)
#define PDPT_ENTRY(addr)	(( addr >> 30 ) & ENTRY_MASK)
#define PD_ENTRY(addr)		(( addr >> 21 ) & ENTRY_MASK)
#define PT_ENTRY(addr)		(( addr >> 12 ) & ENTRY_MASK)

#define ENTRY_TABLE_ADDR(entry, page_sz) 	(( ( entry >> 12 ) & 0xFFFFFFFFFF ) * page_sz)

#define FETCH_IO_DATA_U32   (*(uint32_t*)( (uint8_t*)vm.kvm_run + vm.kvm_run->io.data_offset ))
#define FETCH_IO_DATA_U8    (*( (uint8_t*)vm.kvm_run + vm.kvm_run->io.data_offset ))

#define STORE_IO_DATA_U8(value)     (*((uint8_t*)vm.kvm_run + vm.kvm_run->io.data_offset) = value)

#define VM_MEM_AT(vaddr)    (vm.mem + (uint64_t)vaddr)

#define ENTRY_MASK              0x1FF
#define ENTRY_TABLE_ADDR_MASK   0xFFFFFFFFFF000

#define PDE64_PRESENT   1
#define PDE64_RW        (1U << 1)
#define PDE64_USER      (1U << 2)
#define PDE64_PS        (1U << 7)

// CR4
#define CR4_PAE (1U << 5)

// CR0
#define CR0_PE 1u
#define CR0_PG (1U << 31)

#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)

struct vm_args {
	size_t mem_sz;
	size_t page_sz; 
	const char* guest_image;
	char** files;
    int file_cnt;
	int id;
};


struct vm {
	int kvm_fd;
	int vm_fd;
	int vcpu_fd;
	char *mem;
	struct kvm_run *kvm_run;
};

struct shared_file_info {
	int original_fd;
	int copy_fd;	
	int open_flags;
};



#endif