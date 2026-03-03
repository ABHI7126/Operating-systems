#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAXARGS 10

typedef struct pcb {
    pid_t pid;             
    char *args[MAXARGS];    
    int arrivaltime;        
    int remainingcputime;  
    struct pcb *next;     
} Pcb, *PcbPtr;

PcbPtr create_pcb(int arrival, int cputime, char *program) {
    PcbPtr newpcb = (PcbPtr)malloc(sizeof(Pcb));
    if (!newpcb) {
        perror("malloc");
        exit(1);
    }
    newpcb->pid = 0;
    newpcb->arrivaltime = arrival;
    newpcb->remainingcputime = cputime;
    // split program into args
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
void resume_process(PcbPtr p) {
    if (p->pid == 0) {
        // never started before
        start_process(p);
    } else {
        kill(p->pid, SIGCONT);
        printf("Resumed process PID %d (%s), remaining time %d\n",
               p->pid, p->args[0], p->remainingcputime);
    }
}
void free_pcb(PcbPtr p) {
    if (!p) return;
    for (int i = 0; p->args[i] != NULL; i++) {
        free(p->args[i]);
    }
    free(p);
}

PcbPtr fcfsHead = NULL;
PcbPtr fcfsTail = NULL;

void fcfs_enqueue(PcbPtr p) {
    p->next = NULL;
    if (!fcfsTail) {
        fcfsHead = fcfsTail = p;
    } else {
        fcfsTail->next = p;
        fcfsTail = p;
    }
}

PcbPtr fcfs_dequeue() {
    if (!fcfsHead) return NULL;
    PcbPtr p = fcfsHead;
    fcfsHead = fcfsHead->next;
    if (!fcfsHead) fcfsTail = NULL;
    return p;
}
PcbPtr fcfs_peek() {
    return fcfsHead;
}

void load_dispatch_list_fcfs(const char *filename) {
    fcfsHead = fcfsTail = NULL;

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

        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%d", &arrival, &priority, &cputime, &mem,  &r1, &r2, &r3, &r4) != 8) {
            fprintf(stderr, "Bad line in dispatch list (FCFS): %s", line);
            continue;
        }
        char argline[50];
        snprintf(argline, sizeof(argline), "./sleepy %d", cputime);

        PcbPtr p = create_pcb(arrival, cputime, argline);
        fcfs_enqueue(p);
    }
    fclose(fp);
}
void run_fcfs(const char *filename) {
    load_dispatch_list_fcfs(filename);

    int timer = 0;
    PcbPtr current = NULL;
    printf("\n===== FCFS Dispatcher starting =====\n");

    while (fcfsHead != NULL || current != NULL) {
        // If a process is currently running
        if (current != NULL) {
            current->remainingcputime--;

            printf("Timer %d: Running PID %d, remaining time %d\n",
                   timer, current->pid, current->remainingcputime);

            if (current->remainingcputime <= 0) {
                printf("Timer %d: Killing PID %d\n", timer, current->pid);
                kill(current->pid, SIGINT);
                waitpid(current->pid, NULL, 0);

                free_pcb(current);
                current = NULL;
            }
        }
        // If no process running and next process arrived
        if (current == NULL && fcfsHead != NULL &&
            fcfs_peek()->arrivaltime <= timer) {
            current = fcfs_dequeue();
            start_process(current);
        }
        sleep(1);
        timer++;
    }
    printf("===== All processes completed (FCFS) =====\n");
}
//   ROUND ROBIN SECTION (q = 1)
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
void load_dispatch_list_rr(const char *filename) {
    inputHead = inputTail = NULL;
    rrHead = rrTail = NULL;

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

        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%d", &arrival, &priority, &cputime, &mem,    &r1, &r2, &r3, &r4) != 8)
                {
            fprintf(stderr, "Bad line in dispatch list (RR): %s", line);
            continue;
        }
        char argline[50];
        snprintf(argline, sizeof(argline), "./sleepy %d", cputime);

        PcbPtr p = create_pcb(arrival, cputime, argline);
        enqueue_input(p);
    }
    fclose(fp);
}
void run_rr(const char *filename) {
    load_dispatch_list_rr(filename);

    int timer = 0;
    PcbPtr current = NULL;

    printf("\n===== Round Robin Dispatcher starting (q = 1) =====\n");

    while (inputHead != NULL || rrHead != NULL || current != NULL) {

        //  Move arrived jobs from INPUT to RR queue
        while (inputHead != NULL && peek_input()->arrivaltime <= timer) {
            PcbPtr p = dequeue_input();
            enqueue_rr(p);
            printf("Timer %d: Job arrived (arrival=%d, cputime=%d)\n",
                   timer, p->arrivaltime, p->remainingcputime);
        }
        //If a process is currently running, give it 1 time unit
        if (current != NULL) {
            current->remainingcputime--;
            printf("Timer %d: Running PID %d, remaining time %d\n",
                   timer, current->pid, current->remainingcputime);

            if (current->remainingcputime <= 0) {
                printf("Timer %d: Finishing PID %d\n", timer, current->pid);
                kill(current->pid, SIGINT);
                waitpid(current->pid, NULL, 0);

                free_pcb(current);
                current = NULL;
            }
            else if (rrHead != NULL) {
                printf("Timer %d: Time slice over, preempting PID %d\n",
                       timer, current->pid);
                kill(current->pid, SIGTSTP);
                enqueue_rr(current);
                current = NULL;
            }
        }
        // If no process running and RR queue not empty, schedule next
        if (current == NULL && rrHead != NULL) {
            current = dequeue_rr();
            if (current->pid == 0) {
                start_process(current);
            } else {
                resume_process(current);
            }
        }
        sleep(1);
        timer++;
    }
    printf("===== All processes completed (RR) =====\n");
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s dispatchlist.txt\n", argv[0]);
        return 1;
    }
    int choice;
    printf("Choose Scheduling Algorithm:\n");
    printf("  1. FCFS\n");
    printf("  2. Round Robin (q = 1)\n");
    printf("Enter choice: ");
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Invalid input.\n");
        return 1;
    }
    switch (choice) {
        case 1:
            run_fcfs(argv[1]);
            break;
        case 2:
            run_rr(argv[1]);
            break;
        default:
            printf("Invalid choice.\n");
            return 1;
    }
    return 0;
}
