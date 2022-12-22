#include "../include/resource.h"
#include "../include/builtin.h"

int all_resources[8] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};

void get_resources(int count, int *resources)
{
    // printf("KKKKKKK");
    us.running->resources_count = count;
    us.running->resources = malloc(count * sizeof(int));
    for (int i = 0; i < count; i++) {
        us.running->resources[i] = resources[i];
    }
    
    int block = TRUE;

    while (block == TRUE) {
        block = FALSE;
        for (int i = 0; i < count; i++) {
            if (all_resources[resources[i]] == FALSE) {
                struct WaitingQueueNode *tmp = malloc(sizeof(struct WaitingQueueNode *));
                tmp->next = NULL;
                if (!waiting.head) {
                    waiting.head = tmp;
                    waiting.tail = waiting.head;
                }
                else {
                    waiting.tail->next = tmp;
                    waiting.tail = tmp;
                }
                tmp->ut = us.running;
                us.running->state = WAITING_FOR_RESOURCES;
                printf("Task %s is waiting resource.\n", us.running->name);
                block = TRUE;
                if (swapcontext(&us.running->context, &us.main) == -1)
                    handle_error("swapcontext");
                break;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        all_resources[resources[i]] = FALSE;
        printf("Task %s gets resource %d.\n", us.running->name, resources[i]);
    }
}

void release_resources(int count, int *resources)
{
    us.running->resources = NULL;
    for (int i = 0; i < count; i++) {
        all_resources[resources[i]] = TRUE;
        printf("Task %s releases resource %d.\n", us.running->name, resources[i]);
    }
}
