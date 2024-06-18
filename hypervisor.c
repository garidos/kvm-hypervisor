#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <math.h>
#include "hypervisor.h"



ssize_t copy_file(const char* from, const char* to)
{

    char* buffer = malloc(BUFF_SZ);
    if (NULL == buffer) {
        return 0;
    }


	int fd_to = open(to, O_WRONLY | O_CREAT, 0777);
    if (fd_to < 0) {
        free(buffer);
        return 0;
    }

    int fd_from = open(from, O_RDONLY);
	// Original file didn't exist, so all we had to do is create an empty local copy
	if ( fd_from < 0 ) {
		free(buffer);
		close(fd_to);
		return 1;
	}

    ssize_t size = 0;
    while(1) {
        int read_sz = read(fd_from, buffer, BUFF_SZ);
        if (read_sz) {
            if (read_sz != write(fd_to, buffer, read_sz)) {
                close(fd_from);
                close(fd_to);
                free(buffer);
                remove(to);
                return 0;
            }
        } else break;
        size += read_sz;
    }

    close(fd_to);
    close(fd_from);
    free(buffer);
    return (size == 0 ? 1 : size); 
}

int init_vm(struct vm *vm, size_t mem_size)
{
	struct kvm_userspace_memory_region region;
	int kvm_run_mmap_size;

	vm->kvm_fd = open("/dev/kvm", O_RDWR);
	if (vm->kvm_fd < 0) {
		perror("open /dev/kvm");
		return -1;
	}

	vm->vm_fd = ioctl(vm->kvm_fd, KVM_CREATE_VM, 0);
	if (vm->vm_fd < 0) {
		perror("KVM_CREATE_VM");
		return -1;
	}

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
		   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (vm->mem == MAP_FAILED) {
		perror("mmap mem");
		return -1;
	}

	region.slot = 0;
	region.flags = 0;
	region.guest_phys_addr = 0;
	region.memory_size = mem_size;
	region.userspace_addr = (unsigned long)vm->mem;
    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
		perror("KVM_SET_USER_MEMORY_REGION");
        return -1;
	}

	vm->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
    if (vm->vcpu_fd < 0) {
		perror("KVM_CREATE_VCPU");
        return -1;
	}

	kvm_run_mmap_size = ioctl(vm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (kvm_run_mmap_size <= 0) {
		perror("KVM_GET_VCPU_MMAP_SIZE");
		return -1;
	}

	vm->kvm_run = mmap(NULL, kvm_run_mmap_size, PROT_READ | PROT_WRITE,
			     MAP_SHARED, vm->vcpu_fd, 0);
	if (vm->kvm_run == MAP_FAILED) {
		perror("mmap kvm_run");
		return -1;
	}

	return 0;
}

static void setup_64bit_code_segment(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.present = 1, // Present in memory
		.type = 11, // Code: execute, read, accessed
		.dpl = 0, // Descriptor Privilage Level: 0 (0, 1, 2, 3)
		.db = 0, // Default size - has value of 0 in long mode
		.s = 1, // Code/data segment type
		.l = 1, // Long mode - 1
		.g = 1, // 4KB granularity
	};

	sregs->cs = seg;

	seg.type = 3; // Data: read, write, accessed
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

// free_addr - address that points to next free address in virtual memory
// free_page - free page that should be allocated
void allocate_page(struct vm *vm, uint64_t pml4_addr, uint64_t addr, uint64_t free_addr, size_t page_sz) {

	uint64_t last_addr = free_addr;
	
	uint64_t* pml4 = (void *)(vm->mem + pml4_addr);
	
	if ( !(pml4[PML4_ENTRY(addr)] & PDE64_PRESENT) ) {
		// Allocate space for new pdpt table if needed
		pml4[PML4_ENTRY(addr)] = PDE64_PRESENT | PDE64_RW | PDE64_USER | last_addr;
		last_addr += 0x1000;
	}

	uint64_t* pdpt = (void *)(vm->mem + (pml4[PML4_ENTRY(addr)] & ENTRY_TABLE_ADDR_MASK));

	if ( !(pdpt[PDPT_ENTRY(addr)] & PDE64_PRESENT) ) {
		pdpt[PDPT_ENTRY(addr)] = PDE64_PRESENT | PDE64_RW | PDE64_USER | last_addr;
		last_addr += 0x1000;
	}

	uint64_t* pd = (void *)(vm->mem + (pdpt[PDPT_ENTRY(addr)] & ENTRY_TABLE_ADDR_MASK));
	
	
	if ( page_sz == (4 << 10) ) {

		if ( !(pd[PD_ENTRY(addr)] & PDE64_PRESENT) ) {
			pd[PD_ENTRY(addr)] = PDE64_PRESENT | PDE64_RW | PDE64_USER | last_addr;
			last_addr += 0x1000;
		}

		// Since last level stores physical page index it has to be page size aligned
		uint64_t physical_page = addr & ~(page_sz - 1);

		uint64_t* pt = (void *)(vm->mem + (pd[PD_ENTRY(addr)] & ENTRY_TABLE_ADDR_MASK));

		pt[PT_ENTRY(addr)] = PDE64_PRESENT | PDE64_RW | PDE64_USER | physical_page;
	} else {
		uint64_t physical_page = addr & ~(page_sz - 1);
		pd[PD_ENTRY(addr)] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS | physical_page;
	}
}


static void setup_long_mode(struct vm *vm, struct kvm_sregs *sregs, size_t page_sz, size_t mem_sz)
{
	// Page table size is 512B and they have to be 4KB aligned
	// If page size is 2MB - 512 page tables can be fit inside of one page

	// If both memory size and page size are 2MB that means that i have to fit everything(code, stack and page tables) inside of that one page
	// so i start allocating space for page tables from 0x1000. This means that code segment can be only 4KB in size, which is not good
	uint64_t pml4_addr =  ( page_sz == ( 2 << 20 ) && mem_sz == ( 2 << 20 ) ? 0x1000 : page_sz); // Arbitrary address(we will be using address of page with index 1)
	uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

	uint64_t pdpt_addr = pml4_addr + 0x1000;
	uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

	uint64_t pd_addr = pdpt_addr + 0x1000;
	uint64_t *pd = (void *)(vm->mem + pd_addr);

	// Since next level table addresses in these entries are actualy physical page indices
	// these addreses are stored from 12-th bit in entries, so OR will do the job
	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;


	if ( page_sz == (4 << 10) ) {	// Page size is 4KB
		uint64_t pt_addr = pd_addr + 0x1000;
		uint64_t *pt = (void *)(vm->mem + pt_addr);

		pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pt_addr;

		// Map one page for code(starts from addr 0) and one for stack(starts from last addr(mem_size - 1) and grows downwards)
		pt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER;

		
		allocate_page(vm, pml4_addr, mem_sz - 1, pt_addr + 0x1000, page_sz);
	} else {	// Page size is 2MB
		
		pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;
		
		allocate_page(vm, pml4_addr, mem_sz - 1, pd_addr + 0x1000, page_sz);
	}

	// cr3 holds address of base pmt
	sregs->cr3  = pml4_addr; 
	sregs->cr4  = CR4_PAE; // "Physical Address Extension" has to be set for long mode
	sregs->cr0  = CR0_PE | CR0_PG; // Set "Protected Mode" and "Paging" bits 
	sregs->efer = EFER_LME | EFER_LMA; // Set "Long Mode Active" and "Long Mode Enable" bits

	setup_64bit_code_segment(sregs);
}



void* make_vm(void* arg)
{
	struct vm vm;
	struct kvm_sregs sregs;
	struct kvm_regs regs;
	int stop = 0;
	int ret = 0;
	FILE* img;
	struct vm_args args = *(struct vm_args*)arg;

	if (init_vm(&vm, args.mem_sz)) {
		printf("VM %d: Failed to init the VM\n", args.id);
		return (void*)-1;
	}

	if (ioctl(vm.vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		printf("VM %d: KVM_GET_SREGS", args.id);
		return (void*)-1;
	}

	setup_long_mode(&vm, &sregs, args.page_sz, args.mem_sz);

    if (ioctl(vm.vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
		printf("VM %d: KVM_SET_SREGS", args.id);
		return (void*)-1;
	}

	memset(&regs, 0, sizeof(regs));
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = args.mem_sz;

	if (ioctl(vm.vcpu_fd, KVM_SET_REGS, &regs) < 0) {
		printf("VM %d: KVM_SET_REGS", args.id);
		return (void*)-1;
	}

	img = fopen(args.guest_image, "r");
	if (img == NULL) {
		printf("VM %d: Can not open binary file\n", args.id);
		return (void*)-1;
	}

	char *p = vm.mem;
  	while(feof(img) == 0) {
    	int r = fread(p, 1, 1024, img);
    	p += r;
  	}
  	fclose(img);

	while(stop == 0) {
		ret = ioctl(vm.vcpu_fd, KVM_RUN, 0);
		if (ret == -1) {
			printf("VM %d: KVM_RUN failed\n", args.id);
			return (void*)1;
		}

		switch (vm.kvm_run->exit_reason) {
			case KVM_EXIT_IO:
				if (vm.kvm_run->io.direction == KVM_EXIT_IO_OUT && vm.kvm_run->io.size == 1 && vm.kvm_run->io.port == 0xE9) {
					char *p = (char *)vm.kvm_run;
					//putchar(*(p + vm.kvm_run->io.data_offset));
					printf("VM %d: IO port: %x, data: %d char: %c\n", args.id, vm.kvm_run->io.port, FETCH_IO_DATA_U8, FETCH_IO_DATA_U8);
				} else if ( vm.kvm_run->io.direction == KVM_EXIT_IO_IN && vm.kvm_run->io.size == 1 && vm.kvm_run->io.port == 0xE9 ) {
					char data;

					do {
						data = getchar();
					} while( data == '\n' );

					char *data_in = (((char*)vm.kvm_run)+ vm.kvm_run->io.data_offset);
					*(data_in) = data;
				} else if ( vm.kvm_run->io.direction == KVM_EXIT_IO_OUT && vm.kvm_run->io.size == 4 && vm.kvm_run->io.port == 0x278 ) {
					//printf("VM %d: IO port: %x, size: %d count: %d value: %x\n", args.id, vm.kvm_run->io.port, vm.kvm_run->io.size, vm.kvm_run->io.count, FETCH_IO_DATA_U32);
					struct io_struct *params = (struct io_struct*)VM_MEM_AT(FETCH_IO_DATA_U32); 
					switch(params->type) {
						case OPEN: {
							int opened = 0;
							for ( int i = 0; i < args.file_cnt; i++) {
								if ( !strcmp(args.files[i], (const char*)VM_MEM_AT(params->io_open.filename)) ) {
									if ( params->io_open.flags & O_WRONLY || params->io_open.flags & O_RDWR ) {
										char local_copy_path[256];
										sprintf(local_copy_path, "%s_VM%d_copy", (const char*)VM_MEM_AT(params->io_open.filename), args.id);
										int fd = open(local_copy_path, params->io_open.flags);
										if ( fd == -1 ) {
											if ( copy_file((const char*)VM_MEM_AT(params->io_open.filename), local_copy_path) != 0 ) {
												fd = open(local_copy_path, params->io_open.flags);
											} else {
												printf("VM %d: Error while making a local copy of a file\n", args.id);
												stop = 1;
											}
										}
										params->fd = fd;
									} else {
										// Possible errors will have to be handled in guest code(fd = -1)
										int fd = open(args.files[i], params->io_open.flags | O_CREAT, 0777);
										params->fd = fd;
									}
									opened = 1;
									break;
								}
							}
							if ( opened == 0 ) {
								printf("VM %d: Error: Access to file '%s' is denied. The file is not in the list of allowed files\n", args.id, (const char*)VM_MEM_AT(params->io_open.filename));
								stop = 1;
							}
							break;
						}
						case CLOSE: {
							if ( params->fd != -1 ) close(params->fd);
							break;
						}
						case WRITE: {
							ssize_t res = write(params->fd, VM_MEM_AT(params->io_rw.buffer), params->io_rw.n);
							params->ret = res;
							break;
						}
						case READ: {
							ssize_t res = read(params->fd, VM_MEM_AT(params->io_rw.buffer), params->io_rw.n);
							params->ret = res;
							break;
						}
						case SEEK: {
							off_t res = lseek(params->fd, params->io_seek.offset, params->io_seek.whence);
							params->ret = res;
							break;
						}
					}
				}
				continue;
			case KVM_EXIT_HLT:
				printf("VM %d: KVM_EXIT_HLT\n", args.id);
				stop = 1;
				break;
			case KVM_EXIT_INTERNAL_ERROR:
				printf("VM %d: Internal error: suberror = 0x%x\n", args.id, vm.kvm_run->internal.suberror);
				stop = 1;
				break;
			case KVM_EXIT_SHUTDOWN:
				printf("VM %d: Shutdown\n", args.id);
				stop = 1;
				break;
			default:
				printf("VM %d: Exit reason: %d\n", args.id, vm.kvm_run->exit_reason);
				break;
    	}
  	}
}
