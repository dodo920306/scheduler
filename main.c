#include <stdlib.h>
#include <string.h>
#include "include/shell.h"
#include "include/command.h"
#include "include/builtin.h"

int main(int argc, char *argv[])
{
	history_count = 0;
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
    	history[i] = (char *)malloc(BUF_SIZE * sizeof(char));
	algo = malloc(sizeof(argv[1]));
	strcpy(algo, argv[1]);
	shell();
	free(algo);
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
    	free(history[i]);

	return 0;
}
