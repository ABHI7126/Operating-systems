/*
 * FCFS Batch Process Dispatcher
 * Implements First-Come-First-Served scheduling
 *
 * Author: <Your Name>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXARGS 10
// <arrival time>,<priority>,<cputime>,<memory alloc>,<res1 alloc>,<res2>,<res3>,<res4>
// ----------------------
// PCB Structure
// ----------------------
typedef struct pcb {
    pid_t pid;              // child process PID
    char *args[MAXARGS];    // program + arguments
    int arrivaltime;        // time process is allowed to start
    int remainingcputime;   // time algorithm should run before killing
    struct pcb *next;       // linked list next pointer
} Pcb, *PcbPtr;


// ----------------------
// Queue Helper Functions
// ----------------------

PcbPtr head = NULL;
PcbPtr tail = NULL;

void enqueue(PcbPtr process) {
    process->next = NULL;
    if (tail == NULL) {
        head = tail = process;
    } else {
        tail->next = process;
        tail = process;
    }
}

PcbPtr dequeue() {
    if (head == NULL) return NULL;
    PcbPtr temp = head;
    head = head->next;
    if (head == NULL) tail = NULL;
    return temp;
}

PcbPtr peek() {
    return head;
}


// ----------------------
// Create PCB from input line
// ----------------------
PcbPtr create_pcb(int arrival, int cputime, char *program) {
    PcbPtr newpcb = (PcbPtr)malloc(sizeof(Pcb));
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


// ----------------------
// Load processes from file
// ----------------------
void load_dispatch_list(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("File open error");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // ignore empty lines
        if (strlen(line) < 2) continue;

        int arrival, priority, cputime;
        int mem, r1, r2, r3, r4;

        // parse line (second project input format)
        sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%d", 
                &arrival, &priority, &cputime, &mem, &r1, &r2, &r3, &r4);

        // create args: "sleepy cputime"
        char argline[50];
        snprintf(argline, sizeof(argline), "sleepy %d", cputime);

        PcbPtr p = create_pcb(arrival, cputime, argline);
        enqueue(p);
    }

    fclose(fp);
}


// ----------------------
// Start process using fork+exec
// ----------------------
void start_process(PcbPtr process) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return;
    }
    if (pid == 0) {
        // Child
        execvp(process->args[0], process->args);
        perror("exec failed");
        exit(1);
    }

    // Parent
    process->pid = pid;
    printf("Started process PID %d (%s) for %d seconds\n",
           pid, process->args[0], process->remainingcputime);
}


// ----------------------
// Main Dispatcher Logic
// ----------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s dispatchlist.txt\n", argv[0]);
        exit(1);
    }

    load_dispatch_list(argv[1]);

    int timer = 0;
    PcbPtr current = NULL;

    printf("FCFS Dispatcher starting...\n");

    while (head != NULL || current != NULL) {

        // 1. If process is running
        if (current != NULL) {
            current->remainingcputime--;

            printf("Timer %d: Running PID %d, remaining time %d\n",
                   timer, current->pid, current->remainingcputime);

            // 2. If time's up, kill process
            if (current->remainingcputime <= 0) {
                printf("Timer %d: Killing PID %d\n", timer, current->pid);
                kill(current->pid, SIGINT);
                waitpid(current->pid, NULL, 0);

                // free PCB
                for (int i = 0; current->args[i] != NULL; i++)
                    free(current->args[i]);
                free(current);

                current = NULL;
            }
        }

        // 3. If no process running and next process arrived
        if (current == NULL && head != NULL && peek()->arrivaltime <= timer) {
            current = dequeue();
            start_process(current);
        }

        // 4. Sleep 1 second
        sleep(1);

        // 5. Increment timer
        timer++;
    }

    printf("All processes completed.\n");
    return 0;
}
