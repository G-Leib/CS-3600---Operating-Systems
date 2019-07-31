#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include "eye2eh.c"
#include <time.h>

/*
This program does the following.
1) Creates handlers for two signals.
2) Create an idle process which will be executed when there is nothing
   else to do.
3) Create a send_signals process that sends a SIGALRM every so often.

The output looks approximately like:

[...]
Sending signal:  14 to process: 35050
Stopping: 35052
---- entering scheduler
Continuing idle: 35052
---- leaving scheduler
Sending signal:  14 to process: 35050
at the end of send_signals
Stopping: 35052
---- entering scheduler
Continuing idle: 35052
---- leaving scheduler
---- entering process_done
Timer died, cleaning up and killing everything
Terminated: 15

---------------------------------------------------------------------------
Add the following functionality.
1) Change the NUM_SECONDS to 20. -- DONE

2) Take any number of arguments for executables and place each on the
   processes list with a state of NEW. The executables will not require
   arguments themselves. -- DONE

3) When a SIGALRM arrives, scheduler() will be called; it currently simply
   restarts the idle process. Instead, do the following.
   a) Update the PCB for the process that was interrupted including the
      number of context switches and interrupts it had and changing its
      state from RUNNING to READY. -- DONE
   b) If there are any NEW processes on processes list, change its state to
      RUNNING, and fork() and execl() it.
   c) If there are no NEW processes, round robin the processes in the
      processes queue that are READY. If no process is READY in the
      list, execute the idle process. -- DONE

4) When a SIGCHLD arrives notifying that a child has exited, process_done() is
   called.
   a) Add the printing of the information in the PCB including the number
      of times it was interrupted, the number of times it was context
      switched (this may be fewer than the interrupts if a process
      becomes the only non-idle process in the ready queue), and the total
      system time the process took. -- DONE
   b) Change the state to TERMINATED. -- DONE
   c) Restart the idle process to use the rest of the time slice. -- DONE
*/

#define NUM_SECONDS 10
#define ever ;;

enum STATE { NEW, RUNNING, WAITING, READY, TERMINATED, EMPTY };
enum SEARCH { SEARCH_NEW, SEARCH_READY, SEARCH_EXIT };

enum SEARCH loop;

struct PCB {
    enum STATE state;
    const char *name;   // name of the executable
    int pid;            // process id from fork();
    int ppid;           // parent process id
    int interrupts;     // number of times interrupted
    int switches;       // may be < interrupts
    int started;        // the time this process started
};

typedef enum { false, true } bool;

#define PROCESSTABLESIZE 10
struct PCB processes[PROCESSTABLESIZE];

struct PCB idle;
struct PCB *running;

int num_processes = 0;
int proc_counter = 0;

int sys_time;
int timer;
struct sigaction alarm_handler;
struct sigaction child_handler;

void proc_switch(int next_proc) {
    running->switches++;
    running = &processes[next_proc];
    running->state = RUNNING;
}

void bad(int signum) {
    WRITESTRING("bad signal: ");
    WRITEINT(signum, 4);
    WRITESTRING("\n");
}

// cdecl> declare ISV as array 32 of pointer to function(int) returning void
void(*ISV[32])(int) = {
/*       00   01   02   03   04   05   06   07   08   09 */
/*  0 */ bad, bad, bad, bad, bad, bad, bad, bad, bad, bad,
/* 10 */ bad, bad, bad, bad, bad, bad, bad, bad, bad, bad,
/* 20 */ bad, bad, bad, bad, bad, bad, bad, bad, bad, bad,
/* 30 */ bad, bad
};

void ISR (int signum) {
    if (signum != SIGCHLD) {
        kill (running->pid, SIGSTOP);
        WRITESTRING("Stopping: ");
        WRITEINT(running->pid, 6);
        WRITESTRING("\n");
    }

    ISV[signum](signum);
}

void send_signals(int signal, int pid, int interval, int number) {
    for(int i = 1; i <= number; i++) {
        sleep(interval);
        WRITESTRING("Sending signal: ");
        WRITEINT(signal, 4);
        WRITESTRING(" to process: ");
        WRITEINT(pid, 6);
        WRITESTRING("\n");
        systemcall(kill(pid, signal));
    }

    WRITESTRING("At the end of send_signals\n");
}

void create_handler(int signum, struct sigaction action, void(*handler)(int)) {
    action.sa_handler = handler;

    if (signum == SIGCHLD) {
        action.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    } else {
        action.sa_flags =  SA_RESTART;
    }

    systemcall(sigemptyset(&action.sa_mask));
    systemcall(sigaction(signum, &action, NULL));
}

void scheduler (int signum) {
    WRITESTRING("---- entering scheduler\n");
    assert(signum == SIGALRM);

    running->state = READY;
    running->interrupts++;

    int first_pid = running->pid;
    int current_proc;
    
    loop = SEARCH_NEW;

    running->interrupts++;
    
    do {
        current_proc = proc_counter % num_processes;
        if (loop == SEARCH_NEW) {
            WRITESTRING("looking for new");
            if(processes[current_proc].state == NEW) {
                WRITESTRING("found new -- running");
                proc_switch(current_proc);
                running->started = time(NULL);
                int fork_status = fork();
                
                if (fork_status > 0) {
                    // Positive = Parent process
                } else if (fork_status < 0) {
                    // Negative = Error
                    perror("fork error");
                    exit(-1);
                } else {
                    // 0 = Child process
                    running->pid = getpid();
                    running->ppid = getppid();
                    assert(execl(running->name, running->name, (char *) NULL)!=0);
                }

                break;
            }
        } else if (loop == SEARCH_READY) {
            WRITESTRING("looking for ready\n");
            if(processes[current_proc].state == READY) {
                WRITESTRING("found ready -- running next");
                proc_switch(current_proc);
            }
        }
        
        proc_counter++;
        current_proc = proc_counter % num_processes;
        if (processes[current_proc].pid == first_pid) {
            if (loop == SEARCH_NEW) {
                loop = SEARCH_READY;
            } else if (loop == SEARCH_READY) {
                loop = SEARCH_EXIT;
            }
        }

        if (loop == SEARCH_EXIT) {
            running = &idle;
        }

    } while(loop != SEARCH_EXIT);

    if (strcmp(running->name, "IDLE")==0) {
        WRITESTRING ("Continuing idle: ");
        WRITEINT (idle.pid, 6);
        WRITESTRING ("\n");
    }

    systemcall (kill (idle.pid, SIGCONT));

    WRITESTRING("---- leaving scheduler\n");
}

void process_done (int signum) {
    WRITESTRING("---- entering process_done\n");
    assert (signum == SIGCHLD);

    if(running!=&idle) {
        WRITESTRING("Process: ");
        WRITEINT(running->pid, 6);
        WRITESTRING("\tInterrupts: ");
        WRITEINT(running->interrupts, 4);
        WRITESTRING("\tSwitches: ");
        WRITEINT(running->switches, 4);
        WRITESTRING("\tTime taken: ");
        WRITEINT(time(NULL) - running->started, 4);
        WRITESTRING(" seconds\n");
        running->state = TERMINATED;
        running = &idle;
}

    WRITESTRING ("Timer died, cleaning up and killing everything\n");
    systemcall(kill(0, SIGTERM));

    WRITESTRING ("---- leaving process_done\n");
}

void boot()
{
    sys_time = 0;

    ISV[SIGALRM] = scheduler;
    ISV[SIGCHLD] = process_done;
    create_handler(SIGALRM, alarm_handler, ISR);
    create_handler(SIGCHLD, child_handler, ISR);

    assert((timer = fork()) != -1);
    if (timer == 0) {
        send_signals(SIGALRM, getppid(), 1, NUM_SECONDS);
        exit(0);
    }
}

void create_idle() {
    idle.state = READY;
    idle.name = "IDLE";
    idle.ppid = getpid();
    idle.interrupts = 0;
    idle.switches = 0;
    idle.started = sys_time;

    assert((idle.pid = fork()) != -1);
    if (idle.pid == 0) {
        systemcall(pause());
    }
}

int main(int argc, char **argv) {

    boot();
    create_idle();
    running = &idle;

    for(int i = 1; i < argc; i++){
        num_processes++;
        int proc_num = i - 1;
        processes[proc_num].name = argv[i];
        processes[proc_num].state = NEW;
        processes[proc_num].pid = getpid();
    }




    for(ever) {
        pause();
    }

}
