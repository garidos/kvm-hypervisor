#include <stddef.h>
#include <stdint.h>
#include "../vmiolib.h"

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	const char* filename1 = "shared_text.txt";
    const char* filename2 = "shared_info.txt";

    int fd1 = open(filename1, O_RDWR);
    int fd2 = open(filename2, O_RDWR);

    lseek(fd1, 0, SEEK_END);
    char signature[53] = "\n\n-------------------VM_GUESTC21-------------------\n\n";

    char buffer[28];

    read(fd2, buffer, 28);

    write(fd1, signature, 53);

    close(fd1);
    close(fd2);

    for ( int i = 0; i < 28; i++) {
        putchar(buffer[i]);
    }

	exit();
}
