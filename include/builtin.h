#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#ifndef BUILTIN_H
#define BUILTIN_H

#define TERMINATED -1
#define READY 0
#define RUNNING 1
#define WAITING 2
#define WAITING_FOR_RESOURCES 3
#define STACK_SIZE 8192
#define THREAD_NUM 128
#define FUNCTION_NUM 14
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int help(char **args);
int cd(char **args);
int echo(char **args);
int exit_shell(char **args);
int record(char **args);
int mypid(char **args);

int add(char **args);
int del(char **args);
int ps(char **args);
int start(char **args);

char *algo;

struct Scheduler {
	ucontext_t main;
	struct TCB *running;
	int thread_count;
	struct TCB *head;
};

struct TCB {
	int tid;
	int state;
	char *name;
    ucontext_t context;
	int running;
	int waiting;
	int turnaround;
	int *resources;
	int priority;
	int resources_count;
	char stack[STACK_SIZE];
    void (* fun_ptr)();
	struct TCB* next;
};

struct QueueNode {
    struct QueueNode* next;
    struct TCB *ut;
};

struct Queue {
    struct QueueNode* head;
    struct QueueNode* tail;
};

struct WaitingQueueNode {
    struct WaitingQueueNode* next;
    struct TCB *ut;
	int time;
};

struct WaitingQueue {
    struct WaitingQueueNode* head;
    struct WaitingQueueNode* tail;
};

struct TCB ut[THREAD_NUM];
struct Queue ready;
struct WaitingQueue waiting;

extern const char *builtin_str[];

extern const int (*builtin_func[]) (char **);

extern const char *function_str[];

extern const void (*function_func[]) ();

extern struct Scheduler us;

extern int num_builtins();

#endif
