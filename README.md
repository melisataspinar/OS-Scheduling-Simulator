In this project, I implement a multi-threaded scheduling simulator. There are N threads running concurrently and generating cpu bursts (workload). 

My simulator simulates the following scheduling algorithms:
• FCFS
• SJF
• PRIO
• VRUNTIME

Makefile is included.

If you wish to compile my program directly, you can compile it with the command:
  gcc -o schedule schedule.c -lpthread -lm
