#include <stddef.h>
#include <stdint.h>
#include "../hypervisor.h"

extern void exit();
extern int open(const char* filename, int flags);
extern int close(int fd);
extern ssize_t read(int fd, const void* buf, size_t count);
extern void print_string(const char* str, int n);
off_t lseek(int fd, off_t offset, int whence);


void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	const char* filename = "./files/test.txt";

    int fd = open(filename, O_RDONLY);

    lseek(fd, 5*90, SEEK_SET);

    char buffer[90];
    int cnt;

    while ( cnt = read(fd, buffer, 90) == 90 ) {
        print_string(buffer, 90);
    }

    print_string(buffer, cnt);

    close(fd);

	exit();

}
