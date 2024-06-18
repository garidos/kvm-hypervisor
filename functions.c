#include <stddef.h>
#include "hypervisor.h"

static const uint16_t file_port = 0x278;
static const uint16_t terminal_port = 0xe9;

void  
__attribute__((noreturn))
exit() {
	for (;;)
		asm("hlt");
}

static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

static void outdw(uint16_t port, uint32_t value) {
    asm (
        "movw %w1, %%dx\n\t"   
        "movl %0, %%eax\n\t"   
        "outl %%eax, %%dx"     
        :
        : "r" (value), "r" (port)
        : "dx", "eax", "memory"
    );
}

static uint8_t inb(uint16_t port) {
    uint8_t value;
    asm ("inb %1, %0" : "=a" (value) : "Nd" (port) : "memory");
    return value;
}


int open(const char* filename, int flags) {
    struct io_struct params;

    params.io_open.filename = filename;
    params.io_open.flags = flags;
    params.type = OPEN;

    outdw(file_port, (uint32_t)&params);

    return params.fd;
}

int close(int fd) {
    struct io_struct params;

    params.fd = fd;
    params.type = CLOSE;

    outdw(file_port, (uint32_t)&params);
    
    return params.ret;
}

ssize_t write(int fd, const void* buf, size_t count) {
    struct io_struct params;

    params.fd = fd;
    params.io_rw.buffer = buf;
    params.io_rw.n = count;
    params.type = WRITE;

    outdw(file_port, (uint32_t)&params);

    return params.ret;
}

ssize_t read(int fd, const void* buf, size_t count) {
    struct io_struct params;

    params.fd = fd;
    params.io_rw.buffer = buf;
    params.io_rw.n = count;
    params.type = READ;

    outdw(file_port, (uint32_t)&params);

    return params.ret;
}

off_t lseek(int fd, off_t offset, int whence) {
    struct io_struct params;

    params.fd = fd;
    params.io_seek.offset = offset;
    params.io_seek.whence = whence;
    params.type = SEEK;

    outdw(file_port, (uint32_t)&params);

    return params.ret;
}

void print_string(const char* str, int n) {
	for ( int i = 0; i < n; i++) {
		outb(terminal_port, str[i]);
	}
}

// For null temrinated strings
void print_nt_string(const char* str) {
	for ( const char* p = str; *p; p++) {
		outb(terminal_port, *p);
	}
}

char getchar() {
	return inb(terminal_port);
}

