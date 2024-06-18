#include <stddef.h>
#include <stdint.h>
#include "../hypervisor.h"

extern void exit();
extern int open(const char* filename, int flags);
extern int close(int fd);
extern ssize_t write(int fd, const void* buf, size_t count);

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	const char* filename = "./files/text.txt";

    int fd = open(filename, O_WRONLY);

    char buffer[20];
    int cnt = 0;

    while(cnt != 20) {
        buffer[cnt++] = getchar();
    }

    write(fd, buffer, 20);

    
    close(fd);

	exit();

}
