
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "hypervisor.h"

extern void* make_vm(void* arg);

void print_usage() {
    fprintf(stderr, "Usage: ./mini_hypervisor [OPTIONS]\n");
    fprintf(stderr, "   -m, --memory    <2|4|8>         (required) Value represents size in MB.\n");
    fprintf(stderr, "   -p, --page      <2|4>           (required) Value is either 2MB or 4KB.\n");
    fprintf(stderr, "   -g, --guest     <guest-images>  (required)\n");
    fprintf(stderr, "   -f, --file      <shared-files>  (non-required)\n");
    fprintf(stderr, "   -h, --help\n");
}


int main(int argc, char **argv) {
  
    int page_sz = -1, mem_sz = -1;
    int g_start_ind = -1, g_num = 0;
    int f_start_ind = -1, f_num = 0;

    while (1)
    {
      static struct option long_options[] =
        {
          {"memory", required_argument, 0, 'm'},
          {"page", required_argument, 0, 'p'},
          {"guest", no_argument, 0, 'g'},
          {"file", no_argument, 0, 'f'},
          {"help", no_argument, 0, 'h'},
          {0, 0, 0, 0}
        };

      int option_index = 0;

      int c = getopt_long (argc, argv, "m:p:fg", long_options, &option_index);

      if (c == -1)
        break;

      switch (c) {
        case 'm': {
            mem_sz = atoi(optarg);

            if ( mem_sz != 2 && mem_sz != 4 && mem_sz != 8 ) {
                fprintf(stderr, "Invalid value for option -m.\n");
                print_usage();
                exit(EXIT_FAILURE);
            }

            break;
        }
        case 'p': {
            page_sz = atoi(optarg);
            
            if ( page_sz != 2 && page_sz != 4 ) {
                fprintf(stderr, "Invalid value for option -p.\n");
                print_usage();
                exit(EXIT_FAILURE);
            }

            break;
        }
        case 'f': {
            f_start_ind = optind;
            
            while ( optind < argc && argv[optind][0] != '-' ) {
                f_num++;
                optind++;
            }

            if ( f_num == 0 ) {
                fprintf(stderr, "Option requires at least one argument -- f\n");
                print_usage();
                exit(EXIT_FAILURE);
            }

            break;
        }
        case 'g': {
            g_start_ind = optind;
            
            while ( optind < argc && argv[optind][0] != '-' ) {
                g_num++;
                optind++;
            }

            if ( g_num == 0 ) {
                fprintf(stderr, "Option requires at least one argument -- g\n");
                print_usage();
                exit(EXIT_FAILURE);
            }

            break;
        }
        case 'h': {
            print_usage();
            exit(EXIT_SUCCESS);
        }
        case '?': {
            /* getopt_long already printed an error message. */ 
            print_usage();
            exit(EXIT_FAILURE);
        } 
        default: exit(EXIT_FAILURE);
        }
    }

    if ( mem_sz == -1 ) {
        fprintf(stderr, "Option -m is required.\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if ( page_sz == -1 ) {
        fprintf(stderr, "Option -p is required.\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if ( g_start_ind == -1 ) {
        fprintf(stderr, "Option -g is required.\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    size_t mem_size = ( mem_sz == 2 ? 2 << 20 : ( mem_sz == 4 ? 4 << 20 : 8 << 20 ) );
	size_t page_size = ( page_sz == 2 ? 2 << 20 : 4 << 10 );

    pthread_t vm_threads[g_num];
    struct vm_args args[g_num];
    
    for ( int i = 0; i < g_num; i++ ) {
        args[i].mem_sz = mem_size;
        args[i].page_sz = page_size;
        args[i].guest_image = argv[i + g_start_ind];
        args[i].file_cnt = f_num;
        args[i].files = &argv[f_start_ind];
        args[i].id = i;
        pthread_create(&vm_threads[i], NULL, make_vm, &args[i]);
    }

    for ( int i = 0; i < g_num; i++) {
        pthread_join(vm_threads[i], NULL);
    }

    exit (EXIT_SUCCESS);
}