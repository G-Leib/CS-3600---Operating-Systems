#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

int main() {
    pid_t pid = getpid();

    while(1) {

        assert(printf("Awake in %d\n", pid) != 0);
        assert(sleep(1)>=0);
    }

    return(0);
}