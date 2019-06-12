#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>

int main() {
    int fork_status = fork();
    int fd1[2];

    if (fork_status > 0) {
        // Positive = Parent process
        assert(waitpid(-1, &fork_status, 0)!=0);
        assert(printf("Process exited with status: %d\n", fork_status)!=0);
    } else if (fork_status < 0) {
        // Negative = Error
        perror("fork error");
        exit(-1);
    } else {
        // 0 = Child process
        int child_pid = getpid();
        int parent_pid = getppid();
        assert(printf("Child PID: %d\nParent PID: %d\n", child_pid, parent_pid) !=0);
        assert(execl("./counter", "counter", "5", (char *) NULL)!=0);
    }
}