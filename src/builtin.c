#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include "../include/builtin.h"
#include "../include/command.h"
#include "../include/function.h"
#include "../include/resource.h"

struct itimerval value, ovalue;

struct Scheduler us = {
	.thread_count = 0,
	.head = ut,
};

int help(char **args)
{
	int i;
    printf("--------------------------------------------------\n");
  	printf("My Little Shell!!\n");
	printf("The following are built in:\n");
	for (i = 0; i < num_builtins(); i++) {
    	printf("%d: %s\n", i, builtin_str[i]);
  	}
	printf("%d: replay\n", i);
    printf("--------------------------------------------------\n");
	return 1;
}

int cd(char **args)
{
	if (args[1] == NULL) {
    	fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  	} else {
    	if (chdir(args[1]) != 0)
      		perror("lsh");
	}
	return 1;
}

int echo(char **args)
{
	bool newline = true;
	for (int i = 1; args[i]; ++i) {
		if (i == 1 && strcmp(args[i], "-n") == 0) {
			newline = false;
			continue;
		}
		printf("%s", args[i]);
		if (args[i + 1])
			printf(" ");
	}
	if (newline)
		printf("\n");

	return 1;
}

int exit_shell(char **args)
{
	return 0;
}

int record(char **args)
{
	if (history_count < MAX_RECORD_NUM) {
		for (int i = 0; i < history_count; ++i)
			printf("%2d: %s\n", i + 1, history[i]);
	} else {
		for (int i = history_count % MAX_RECORD_NUM; i < history_count % MAX_RECORD_NUM + MAX_RECORD_NUM; ++i)
			printf("%2d: %s\n", i - history_count % MAX_RECORD_NUM + 1, history[i % MAX_RECORD_NUM]);
	}
	return 1;
}

bool isnum(char *str)
{
	for (int i = 0; i < strlen(str); ++i) {
    	if(str[i] >= 48 && str[i] <= 57)
			continue;
        else
		    return false;
  	}
  	return true;
}

int mypid(char **args)
{
	char fname[BUF_SIZE];
	char buffer[BUF_SIZE];
	if(strcmp(args[1], "-i") == 0) {

	    pid_t pid = getpid();
	    printf("%d\n", pid);
	
	} else if (strcmp(args[1], "-p") == 0) {
	
		if (args[2] == NULL) {
      		printf("mypid -p: too few argument\n");
      		return 1;
    	}

    	sprintf(fname, "/proc/%s/stat", args[2]);
    	int fd = open(fname, O_RDONLY);
    	if(fd == -1) {
      		printf("mypid -p: process id not exist\n");
     		return 1;
    	}

    	read(fd, buffer, BUF_SIZE);
	    strtok(buffer, " ");
    	strtok(NULL, " ");
	    strtok(NULL, " ");
    	char *s_ppid = strtok(NULL, " ");
	    int ppid = strtol(s_ppid, NULL, 10);
    	printf("%d\n", ppid);
	    
		close(fd);

  	} else if (strcmp(args[1], "-c") == 0) {

		if (args[2] == NULL) {
      		printf("mypid -c: too few argument\n");
      		return 1;
    	}

    	DIR *dirp;
    	if ((dirp = opendir("/proc/")) == NULL){
      		printf("open directory error!\n");
      		return 1;
    	}

    	struct dirent *direntp;
    	while ((direntp = readdir(dirp)) != NULL) {
      		if (!isnum(direntp->d_name)) {
        		continue;
      		} else {
        		sprintf(fname, "/proc/%s/stat", direntp->d_name);
		        int fd = open(fname, O_RDONLY);
        		if (fd == -1) {
          			printf("mypid -p: process id not exist\n");
          			return 1;
        		}

        		read(fd, buffer, BUF_SIZE);
        		strtok(buffer, " ");
        		strtok(NULL, " ");
        		strtok(NULL, " ");
		        char *s_ppid = strtok(NULL, " ");
		        if(strcmp(s_ppid, args[2]) == 0)
		            printf("%s\n", direntp->d_name);

        		close(fd);
     		}
	   	}
    	
		closedir(dirp);
	
	} else {
    	printf("wrong type! Please type again!\n");
  	}
	
	return 1;
}

int rr = 3, cpuidle = TRUE;

void sigroutine() {
	// printf("%s\n", ready.tail->ut->name);
	for (int i = 1; i <= us.thread_count; i++) {
		// printf("%d %d %d\n", ut[i].waiting, ut[i].running, ut[i].turnaround);
		if (ut[i].state != TERMINATED){
			ut[i].turnaround++;	//printf("%s %d\n", ut[i].name, ut[i].turnaround);
			if (ut[i].state == READY)	ut[i].waiting++;
			if (ut[i].state == RUNNING)	ut[i].running++;
			
		}
	}

	// for (struct QueueNode *it = ready.head; it != NULL; it = it->next) {
	// 	printf("%s ", it->ut->name);
	// }
	// printf("\n");
	for (struct WaitingQueueNode *it = waiting.head; it != NULL; it = it->next) {
		
		if (it->ut->state == WAITING) {
			
			if (it->time > 0) {
				
				it->time--;
				
			}
			
			if (it->time <= 0){
				
				// printf("%s waits for %d seconds.\n", it->ut->name, it->time);
				struct QueueNode *tmp = malloc(sizeof(struct QueueNode *));
				tmp->next = NULL;
				if (!ready.head) {
					ready.head = tmp;
					ready.tail = ready.head;
				}
				else {
					ready.tail->next = tmp;
					ready.tail = tmp;
				}
				tmp->ut = it->ut;
				it->ut->state = READY;
				
				
			}
			
		}
		else if (it->ut->state == WAITING_FOR_RESOURCES) {
			// printf("HI\n");
			// printf("%s waits for %d seconds.\n", it->ut->name, it->time);
			// printf("%s %d\n", it->ut->name, it->ut->state);
			int ok = TRUE;
			for (int i = 0; i < it->ut->resources_count; i++) {
				
				if (all_resources[it->ut->resources[i]] == FALSE) {
					ok = FALSE;
					break;
				}
			}
			if (ok == FALSE)	continue;

			struct QueueNode *tmp = malloc(sizeof(struct QueueNode *));
			tmp->next = NULL;
			if (!ready.head) {
				ready.head = tmp;
				ready.tail = ready.head;
			}
			else {
				ready.tail->next = tmp;
				ready.tail = tmp;
			}
			tmp->ut = it->ut;
			it->ut->state = READY;
			// printf("%s %d\n", it->ut->name, it->ut->state);
		}
	}
	// printf("aaa ");
	struct WaitingQueueNode *it = waiting.head;

	while (it && it->ut->state == READY) {
		
		it = it->next;
	}
	
	if (!it) {
		// printf("empty.\n");
		waiting.head = NULL;
		waiting.tail = NULL;	
	}
	else {
		waiting.head = it;
		waiting.tail = waiting.head;
		
		it = it->next;
		
		while (it != NULL) {
			
			if (it->ut->state != READY) {
				waiting.tail->next = it;
				waiting.tail = waiting.tail->next;
			}
			it = it->next;
		}
		waiting.tail->next = NULL;
		
	}
	// printf("bbb ");
	if (cpuidle == FALSE && !strcmp(algo, "RR")) {
		// printf("%d\n", rr);
		rr--;
		
		if (rr == 0) {
			rr = 3;
			// printf("%s\n", us.running->name);
			struct QueueNode *tmp = malloc(sizeof(struct QueueNode *));
			tmp->next = NULL;
			if (!ready.head) {
				ready.head = tmp;
				ready.tail = ready.head;
			}
			else {
				
				ready.tail->next = tmp;
				ready.tail = tmp;
				
			}
			
			tmp->ut = us.running;
			us.running->state = READY;
			// printf("%s\n", ready.tail->ut->name);
			if (swapcontext(&us.running->context, &us.main) == -1)
        		handle_error("swapcontext");
		}
	}

	if (!strcmp(algo, "PP")) {
		int maxp = __INT32_MAX__;
		for (struct QueueNode* jt = ready.head; jt != NULL; jt = jt->next) {
			if (jt->ut->state == READY)	maxp = maxp < jt->ut->priority ? jt->ut->priority : maxp;
		}

		if (maxp < us.running->priority) {
			if (swapcontext(&us.running->context, &us.main) == -1)
        		handle_error("swapcontext");
		}
	}
}

int stop = FALSE;

void stproutine() {
	stop = TRUE;

	struct itimerval new;
	new.it_value.tv_sec = 0;
   	new.it_value.tv_usec = 0;
   	new.it_interval.tv_sec = 0;
   	new.it_interval.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &new, NULL);

	signal(SIGTSTP, SIG_DFL);

	if (cpuidle == FALSE) {
		if (swapcontext(&us.running->context, &us.main) == -1)
        	handle_error("swapcontext");
	}
}

int add(char **args)
{
	us.thread_count++;
	struct TCB *current = malloc(sizeof(struct TCB));
	getcontext(&current->context);
	current->context.uc_stack.ss_sp   = current->stack;
    current->context.uc_stack.ss_size = sizeof(current->stack);
    current->context.uc_link          = &us.main;
	current->tid = us.thread_count;
	current->name = malloc(sizeof(args[1]));
	current->running = 0;
	current->waiting = 0;
	current->turnaround = 0;
	current->resources_count = 0;
	current->resources = NULL;
	strcpy(current->name, args[1]);
	current->state = READY;
	current->priority = atoi(args[3]);
	for (int i = 0; i < FUNCTION_NUM; i++){
		if (!strcmp(args[2], function_str[i])) {
			
			current->fun_ptr = function_func[i];
		}
	}
	*(&ut[us.thread_count]) = *current;
	// printf("%s\n", ut[us.thread_count].name);
	printf("Task %s is ready.\n", args[1]);
	
	return 1;
}

int del(char **args)
{
	struct TCB *thread;
	for (int i = 1; i <= us.thread_count; i++){
		if (!strcmp(args[1], ut[i].name)) {
			ut[i].state = TERMINATED;
			thread = &ut[i];
		}
	}

	if (thread->state != WAITING_FOR_RESOURCES) {
		for (int i = 0; i < thread->resources_count; i++) {
			all_resources[thread->resources[i]] = TRUE;
		}
	}

	thread->resources = NULL;

	printf("Task %s is killed.\n", args[1]);
	return 1;
}

int ps(char **args)
{
	printf(" TID|      name|      state| running| waiting| turnaround| resources| priorty\n");
	printf("-----------------------------------------------------------------------------\n");
	
	for (int i = 1; i <= us.thread_count; i++){
		// printf("%d %s\n", i, ut[i].name);
		printf("%4d|", ut[i].tid);
		printf("%10s|", ut[i].name);
		char state[11];
		switch (ut[i].state) {
			case READY:
				strcpy(state, "READY");
				break;
			case RUNNING:
				strcpy(state, "RUNNING");
				break;
			case WAITING:
			case WAITING_FOR_RESOURCES:
				strcpy(state, "WAITING");
				break;
			case TERMINATED:
				strcpy(state, "TERMINATED");
				break;
			default:
				break;
		}
		printf("%11s|", state);
		printf("%8d|", ut[i].running);
		printf("%8d|", ut[i].waiting);
		if (ut[i].state == TERMINATED) {
			printf("%11d|", ut[i].turnaround);
		}
		else	printf("%11s|", "none");

		if (!ut[i].resources || ut[i].state == WAITING_FOR_RESOURCES || ut[i].state == READY || ut[i].state == TERMINATED)
			printf("%10s|", "none");
		else {
			// printf("%d ", ut[i].resources[0]);
			char t[ut[i].resources_count * 2 + 1];
			for (int j = 0; j < ut[i].resources_count; j++) {
				// printf("%d\n", ut[i].resources[j]);
				// printf("%d ", ut[i].resources[j]);
				t[2 * j] = ' ';
				t[2 * j + 1] = ut[i].resources[j] + '0';
			}
			t[ut[i].resources_count * 2] = '\0';
			printf("%10s|", t);
		}

		printf("%8d\n", ut[i].priority);
	}
	return 1;
}

int start(char **args)
{
	stop = FALSE;
	for (int i = 1; i <= us.thread_count; i++) {
		
		if (ut[i].state == READY) {
			struct QueueNode *tmp = malloc(sizeof(struct QueueNode *));
			tmp->next = NULL;
			if (!ready.head) {
				ready.head = tmp;
				ready.tail = ready.head;
			}
			else {
				ready.tail->next = tmp;
				ready.tail = tmp;
			}
			tmp->ut = &ut[i];
			// ut[i].turnaround = 0;
			makecontext(&ut[i].context, ut[i].fun_ptr, 0);
			
		}
	}
	
	printf("Start simulation.\n");

	signal(SIGVTALRM, sigroutine);
	value.it_value.tv_sec = 0;
   	value.it_value.tv_usec = 100000;
   	value.it_interval.tv_sec = 0;
   	value.it_interval.tv_usec = 100000;
   	setitimer(ITIMER_VIRTUAL, &value, NULL);
	signal(SIGTSTP, stproutine);

	if (!strcmp(algo, "FCFS") || !strcmp(algo, "RR")) {
		
		while (TRUE) {
			cpuidle = TRUE;

			int end = TRUE;
			for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
				if (it->ut->state != TERMINATED) {
					end = FALSE;
				}
			}
				
			if (end == TRUE)	break;
			
			int count = FALSE;
			
			while (cpuidle == TRUE) {
				if (stop == TRUE)	return 1;
				for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
					if (it->ut->state == READY || it->ut->state == RUNNING) {
						// printf("%s\n", it->ut->name);
						cpuidle = FALSE;
					}
				}
				
				if (!count && cpuidle == TRUE) {
					count = TRUE;
					printf("CPU idle.\n");
				}
			}

			int running = FALSE;
			for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
				struct TCB *current = it->ut;
				if (current->state == RUNNING) {
					us.running = current;
					printf("Task %s is running.\n", current->name);
					rr = 3;
					if (swapcontext(&us.main, &current->context) == -1)
						handle_error("swapcontext");
					if (stop == TRUE)	return 1;
					running = TRUE;
				}
			}
			if (running == TRUE)	continue;

			// printf("check\n");
			for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
				struct TCB *current = it->ut;
				// printf("%s\n", current->name);
				if (current->state == READY) {
					current->state = RUNNING;
					// printf("%d %d %d\n", ut[1].waiting, ut[1].running, ut[1].turnaround);
					
					us.running = current;
					printf("Task %s is running.\n", current->name);

					// for (struct QueueNode* jt = ready.head; jt != NULL; jt = jt->next) {
					// 	if (jt->ut->state != TERMINATED)	printf("%s ", jt->ut->name);
					// }
					// printf("\n");
					rr = 3;
					if (swapcontext(&us.main, &current->context) == -1)
						handle_error("swapcontext");
					if (stop == TRUE)	return 1;
					// for (struct QueueNode* jt = ready.head; jt != NULL; jt = jt->next) {
					// 	if (jt->ut->state != TERMINATED)	printf("%s ", jt->ut->name);
					// }
					// printf("\n");
				}
			}

			
			
		}
	}
	

	if (!strcmp(algo, "PP")) {


		while (TRUE) {

			int idle = TRUE, count = FALSE;
			while (idle == TRUE) {
				for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
					if (it->ut->state == READY || it->ut->state == RUNNING) {
						// printf("%s\n", it->ut->name);
						idle = FALSE;
					}
				}
				
				if (!count && idle == TRUE) {
					count = TRUE;
					printf("CPU idle.\n");
				}
			}

			int maxp = __INT16_MAX__;
			for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
				
				if (it->ut->state == READY){
					if (maxp > it->ut->priority) {
						
						maxp = it->ut->priority;
					}
				}	
				
			}
			
			for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
				struct TCB *current = it->ut;
				if (current->priority == maxp && current->state == READY) {
					current->state = RUNNING;
					us.running = current;
					printf("Task %s is running.\n", current->name);
					
					if (swapcontext(&us.main, &current->context) == -1)
						handle_error("swapcontext");
				}
			}

			int end = TRUE;
			for (struct QueueNode* it = ready.head; it != NULL; it = it->next) {
				if (it->ut->state != TERMINATED) {
					end = FALSE;
				}
			}
			
			if (end == TRUE)	break;
		}
	}

	printf("Simulation over.\n");
	return 1;
}

const char *builtin_str[] = {
 	"help",
 	"cd",
	"echo",
 	"exit",
 	"record",
	"mypid",
	"add",
	"del",
	"ps",
	"start"
};

const int (*builtin_func[]) (char **) = {
	&help,
	&cd,
	&echo,
	&exit_shell,
  	&record,
	&mypid,
	&add,
	&del,
	&ps,
	&start
};

const char *function_str[] = {
	"test_exit",
	"test_sleep",
	"test_resource1",
	"test_resource2",
	"idle",
	"task1",
	"task2",
	"task3",
	"task4",
	"task5",
	"task6",
	"task7",
	"task8",
	"task9"
};

const void (*function_func[]) () = {
	&test_exit,
	&test_sleep,
	&test_resource1,
	&test_resource2,
	&idle,
	&task1,
	&task2,
	&task3,
	&task4,
	&task5,
	&task6,
	&task7,
	&task8,
	&task9
};

int num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}
