#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <wait.h>
#include <stdlib.h>
#include <errno.h>
#include "syscall.h"
#include "eye2eh.c"

void handler();

int main() {

    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    assert(sigaction(SIGCHLD, &action, NULL) == 0 );

    pid_t fork_status = fork();

    if(fork_status > 0) {
        // Parent
        for(int i = 0; i < 5; i++){
            WRITESTRING("Going to SIGSTOP child\n");
            assert(kill(fork_status, SIGSTOP) == 0);
            assert(sleep(2)>=0);
            WRITESTRING("Going to SIGCONT child\n");
            assert(kill(fork_status, SIGCONT) == 0);
            assert(sleep(2)>=0);
        }
        assert(kill(fork_status, SIGINT) == 0);
        assert(pause() != -1);
    } else if(fork_status < 0){
        // Error
        perror("fork error");
        exit(-1);
    } else {
        // Child
        assert(execl("./child", "child", NULL) != -1);
    }
    return(0);
}


void handler(int signal) {
    int CHLDSIG = 17;
    if(signal == CHLDSIG) {
        pid_t exit_status;
        assert(waitpid(-1, &exit_status, 0) >= 0);
        WRITESTRING("Child's status: ");
        WRITEINT(exit_status, sizeof(exit_status));
        WRITESTRING("\n");
        exit(0);
    }
}