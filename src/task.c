#include "../include/task.h"
#include "../include/builtin.h"

void task_sleep(int ms)
{

    struct WaitingQueueNode *tmp = malloc(sizeof(struct WaitingQueueNode *));
	tmp->next = NULL;
    tmp->ut = us.running;
    tmp->time = ms;
	if (!waiting.head) {
		waiting.head = tmp;
		waiting.tail = waiting.head;
	}
	else {
		waiting.tail->next = tmp;
	    waiting.tail = tmp;
	}
	us.running->state = WAITING;
    printf("Task %s goes to sleep.\n", us.running->name);
    if (swapcontext(&us.running->context, &us.main) == -1)
        handle_error("swapcontext");
}

void task_exit()
{
    us.running->state = TERMINATED;
    printf("Task %s has terminated.\n", us.running->name);
    if (swapcontext(&us.running->context, &us.main) == -1)
        handle_error("swapcontext");
}
