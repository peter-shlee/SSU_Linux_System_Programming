#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

void doDeleteCommand();
void doExitCommand();
void doHelpCommand();

int main(void)
{
	char command_buf[BUFFER_SIZE];
	char *command;

	while(1) {
		printf("20160548>");
		fgets(command_buf, sizeof(command_buf), stdin);
		command_buf[strlen(command_buf) - 1] = '\0';
		command = strtok(command_buf, " ");

		if (!strcmp(command, "DELETE") || !strcmp(command, "delete")) {
			doDeleteCommand();
		} else if (!strcmp(command, "EXIT") || !strcmp(command, "exit")) {
			doExitCommand();
			break;
		} else {
			doHelpCommand();
		}
	}

	exit(0);
}

void doDeleteCommand(){
	char *filename;
	char *endTime;
	char *option;

	filename = strtok(NULL, " ");
	endTime = strtok(NULL, " ");
	option = strtok(NULL, " ");

	printf("%s  %s  %s\n", filename, endTime, option);
	return;
}

void doExitCommand(){

}

void doHelpCommand(){

}
