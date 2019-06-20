#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <error.h>
#include <stdlib.h>
#include <assert.h>


void handler();

int main() {

    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset ( &action.sa_mask );
    action.sa_flags = SA_RESTART;
    assert( sigaction( SIGUSR1, &action, NULL ) == 0 );
    assert( sigaction( SIGUSR2, &action, NULL ) == 0 );
    assert( sigaction( SIGURG, &action, NULL ) == 0 );

    pid_t fork_status = fork();

    if( fork_status > 0) {
        // Parent
        assert( waitpid( -1, &fork_status, 0 ) >= 0 );
    } else if( fork_status < 0 ) {
        // Error
        perror("fork error");
        exit(-1);
    } else {
        // Child
        assert( execl( "./child", "child", NULL ) != -1 );
    }
    return(0);

}

void handler( int signal_type ) {
    if( signal_type == 10 ) {
        // SIGUSR1
        assert( printf( "Signal 10: SIGUSR1\n" ) != 0 );
    } else if( signal_type == 12 ) {
        // SIGUSR2
        assert( printf( "Signal 12: SIGUSR2\n" ) != 0 );
    } else if (signal_type == 23 ) {
        // SIGURG
        assert( printf( "Signal 23: SIGURG\n" ) != 0 );
    }
}