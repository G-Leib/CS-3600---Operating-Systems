#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

int main(int argc, char*argv[]) {

    char* end;
    long int NUM_LOOPS = strtol(argv[1], &end, 10);
    
    for(int i=1; i <= NUM_LOOPS; i++) {
            assert(printf("Process: %d %d \n", getpid(), i) != 0);
    }

    return NUM_LOOPS;
}