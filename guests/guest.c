#include <stddef.h>
#include <stdint.h>
#include "../hypervisor.h"

extern void exit();
extern void print_nt_string(const char* str);
extern char getchar();

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	int n1, n2;
	// 48 - 57
	print_nt_string("Write a first number in range 0-9: ");
	n1 = getchar();

	if ( n1 < 48 || n1 > 57 ) { 
		print_nt_string("Invalid input.\n");
		exit();
	}

	print_nt_string("Write a second number in range 0-9: ");
	n2 = getchar();

	if ( n2 < 48 || n2 > 57 ) { 
		print_nt_string("Invalid input.\n");
		exit();
	}

	print_nt_string("Larger number is: ");

	char res[3] = { ( n1 > n2 ? n1 : n2 ), '\n', '\0'};
	print_nt_string(res);

	exit();

}
