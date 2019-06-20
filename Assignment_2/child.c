#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int main() {
    pid_t parent = getppid();

    assert( kill( parent, SIGUSR1 ) == 0 );
    assert( kill( parent, SIGUSR2 ) == 0 );
    assert( kill( parent, SIGURG ) == 0 );
    return(0);
}