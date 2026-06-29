# FCFS Process Dispatcher

## Overview

This project implements a **First Come First Serve (FCFS) Process Dispatcher** in C as part of an Operating Systems laboratory. The dispatcher simulates CPU process scheduling by executing processes according to their arrival time and allocated CPU burst time.

The scheduler reads process information from a dispatch list, creates Process Control Blocks (PCBs), launches processes, monitors execution, and terminates them once their CPU time expires.

---

## Features

- First Come First Serve (FCFS) scheduling algorithm
- Process Control Block (PCB) implementation
- Queue-based process scheduling
- Process creation using `fork()`
- Program execution using `execvp()`
- Process termination using signals (`SIGINT`)
- Arrival-time based scheduling
- CPU burst time simulation
- Linked-list implementation for ready queue

---

## Technologies Used

- C Programming
- Linux Operating System
- GCC Compiler
- POSIX System Calls
- Process Management APIs

---

## Project Structure

```
OS/
│── fcfs_dispatcher.c      # FCFS scheduler implementation
│── sleepy.c               # Dummy process executed by dispatcher
│── sleepy.exe             # Compiled executable (optional)
```

---

## How It Works

1. Reads the dispatch list file.
2. Creates a PCB for every process.
3. Stores processes in a ready queue.
4. Starts processes based on arrival time.
5. Executes one process at a time following FCFS scheduling.
6. Monitors remaining CPU time.
7. Terminates the process when its CPU burst completes.
8. Continues until all processes have finished execution.

---

## Process Scheduling Algorithm

**Algorithm:** First Come First Serve (FCFS)

Processes are executed strictly in the order of their arrival.

- Non-preemptive scheduling
- Simple queue implementation
- Lowest scheduling overhead

---

## Compilation

Compile the dispatcher:

```bash
gcc fcfs_dispatcher.c -o dispatcher
```

Compile the sample process:

```bash
gcc sleepy.c -o sleepy
```

---

## Execution

Run the dispatcher using a dispatch list file.

```bash
./dispatcher dispatchlist.txt
```

---

## Sample Dispatch List Format

```
Arrival,Priority,CPUTime,Memory,R1,R2,R3,R4
0,0,5,64,0,0,0,0
2,0,3,64,0,0,0,0
5,0,4,64,0,0,0,0
```

---

## Key Concepts Demonstrated

- Operating System Process Scheduling
- Process Control Block (PCB)
- Queue Data Structure
- Process Creation
- Process Scheduling
- CPU Burst Management
- Signal Handling
- Process Synchronization

---

## Learning Outcomes

- Understand FCFS scheduling
- Learn Linux process management
- Implement CPU scheduling algorithms
- Work with POSIX system calls
- Simulate operating system scheduling behavior

---

## Future Enhancements

- Round Robin Scheduling
- Priority Scheduling
- Shortest Job First (SJF)
- Multilevel Queue Scheduling
- Memory Management Simulation
- Gantt Chart Visualization
- Performance Metrics (Waiting Time, Turnaround Time)

---

## Author

**Abhishek Koppuravuri**

---

## License

This project is developed for educational purposes as part of an Operating Systems laboratory.
