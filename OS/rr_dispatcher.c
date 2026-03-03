// rr_dispatcher.c
// Round Robin Dispatcher (q = 1)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXARGS 10

// ----------------------
// PCB Structure
// ----------------------
typedef struct pcb {
    pid_t pid;              // child process PID
    char *args[MAXARGS];    // program + arguments
    int arrivaltime;        // arrival time
    int remainingcputime;   // remaining CPU time
    struct pcb *next;       // link for queues
} Pcb, *PcbPtr;


// ----------------------
// Queue helpers (for input and RR queues)
// ----------------------
PcbPtr inputHead = NULL, inputTail = NULL;
PcbPtr rrHead = NULL, rrTail = NULL;

void enqueue_input(PcbPtr p) {
    p->next = NULL;
    if (!inputTail) inputHead = inputTail = p;
    else {
        inputTail->next = p;
        inputTail = p;
    }
}

PcbPtr dequeue_input() {
    if (!inputHead) return NULL;
    PcbPtr p = inputHead;
    inputHead = inputHead->next;
    if (!inputHead) inputTail = NULL;
    return p;
}

PcbPtr peek_input() {
    return inputHead;
}

void enqueue_rr(PcbPtr p) {
    p->next = NULL;
    if (!rrTail) rrHead = rrTail = p;
    else {
        rrTail->next = p;
        rrTail = p;
    }
}

PcbPtr dequeue_rr() {
    if (!rrHead) return NULL;
    PcbPtr p = rrHead;
    rrHead = rrHead->next;
    if (!rrHead) rrTail = NULL;
    return p;
}

PcbPtr peek_rr() {
    return rrHead;
}


// ----------------------
// Create PCB from program description
// ----------------------
PcbPtr create_pcb(int arrival, int cputime, char *program) {
    PcbPtr newpcb = (PcbPtr)malloc(sizeof(Pcb));
    if (!newpcb) {
        perror("malloc");
        exit(1);
    }

    newpcb->pid = 0;
    newpcb->arrivaltime = arrival;
    newpcb->remainingcputime = cputime;

    char *token;
    char *rest = program;
    int argc = 0;

    while ((token = strtok_r(rest, " ", &rest)) != NULL && argc < MAXARGS - 1) {
        newpcb->args[argc] = strdup(token);
        argc++;
    }
    newpcb->args[argc] = NULL;

    newpcb->next = NULL;
    return newpcb;
}


// ----------------------
// Load processes into INPUT queue
// ----------------------
void load_dispatch_list(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) < 2) continue;

        int arrival, priority, cputime;
        int mem, r1, r2, r3, r4;

        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%d",
                   &arrival, &priority, &cputime, &mem,
                   &r1, &r2, &r3, &r4) != 8) {
            fprintf(stderr, "Bad line in dispatch list: %s", line);
            continue;
        }

        char argline[50];
        snprintf(argline, sizeof(argline), "./sleepy %d", cputime);

        PcbPtr p = create_pcb(arrival, cputime, argline);
        enqueue_input(p);
    }

    fclose(fp);
}


// ----------------------
// Start process
// ----------------------
void start_process(PcbPtr p) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(p->args[0], p->args);
        perror("exec failed");
        exit(1);
    }

    p->pid = pid;
    printf("Started process PID %d (%s) for %d seconds\n",
           pid, p->args[0], p->remainingcputime);
}

// ----------------------
// Resume suspended process
// ----------------------
void resume_process(PcbPtr p) {
    // If never started, start it; else send SIGCONT
    if (p->pid == 0) {
        start_process(p);
    } else {
        kill(p->pid, SIGCONT);
        printf("Resumed process PID %d (%s), remaining time %d\n",
               p->pid, p->args[0], p->remainingcputime);
    }
}


// ----------------------
// Main Round Robin Dispatcher (q = 1)
// ----------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s dispatchlist.txt\n", argv[0]);
        exit(1);
    }

    load_dispatch_list(argv[1]);

    int timer = 0;
    PcbPtr current = NULL;

    printf("Round Robin Dispatcher starting (q = 1)...\n");

    while (inputHead != NULL || rrHead != NULL || current != NULL) {

        // i. Move arrived jobs from INPUT to RR queue
        while (inputHead != NULL && peek_input()->arrivaltime <= timer) {
            PcbPtr p = dequeue_input();
            enqueue_rr(p);
            printf("Timer %d: Job arrived (arrivaltime=%d, cputime=%d)\n",
                   timer, p->arrivaltime, p->remainingcputime);
        }

        // ii. If a process is currently running, give it 1 time unit
        if (current != NULL) {
            current->remainingcputime--;

            printf("Timer %d: Running PID %d, remaining time %d\n",
                   timer, current->pid, current->remainingcputime);

            if (current->remainingcputime <= 0) {
                // finished: terminate
                printf("Timer %d: Finishing PID %d\n", timer, current->pid);
                kill(current->pid, SIGINT);
                waitpid(current->pid, NULL, 0);

                // free PCB
                for (int i = 0; current->args[i] != NULL; i++)
                    free(current->args[i]);
                free(current);
                current = NULL;
            }
            else if (rrHead != NULL) {
                // timeslice over, other processes waiting, preempt
                printf("Timer %d: Time slice over, preempting PID %d\n",
                       timer, current->pid);
                kill(current->pid, SIGTSTP);   // suspend
                enqueue_rr(current);           // back of RR queue
                current = NULL;
            }
        }

        // iii. If no process running and RR queue not empty, schedule next
        if (current == NULL && rrHead != NULL) {
            current = dequeue_rr();
            // if pid == 0, never started; else resume
            if (current->pid == 0) {
                start_process(current);
            } else {
                resume_process(current);
            }
        }

        // iv. Sleep one second
        sleep(1);

        // v. Increment dispatcher timer
        timer++;
    }

    printf("All processes completed (RR).\n");
    return 0;
}
