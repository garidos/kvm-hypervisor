#include <stddef.h>
#include <stdint.h>
#include "../hypervisor.h"

extern void exit();
extern int open(const char* filename, int flags);
extern int close(int fd);
extern ssize_t read(int fd, const void* buf, size_t count);
extern void print_string(const char* str, int n);


void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	const char* filename = "./files/test.txt";

    int fd = open(filename, O_RDONLY);

    char buffer[90];
    int cnt = 0;

    while ( cnt != 5*90 ) {
        cnt += read(fd, buffer, 90);
        print_string(buffer, 90);
    }

    close(fd);

	exit();

}
