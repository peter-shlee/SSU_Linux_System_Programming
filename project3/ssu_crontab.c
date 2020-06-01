#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define MAX_COMMAND_COUNT 100

const char *PROMPT_STR = "20160548>";
const char *CRONTAB_FILE_NAME = "ssu_crontab_file";

char crontab_commands[MAX_COMMAND_COUNT][BUFFER_SIZE];
int crontab_commands_count;

void printCrontabCommands(void);
void getCrontabCommands(void);
void doAddCommand(void);
void doRemoveCommand(void);

int main(void)
{
	char input_str[BUFFER_SIZE];
	char *input_command;

	if (access(CRONTAB_FILE_NAME, F_OK) < 0) {
		fclose(fopen(CRONTAB_FILE_NAME, "w"));
	}

	getCrontabCommands();
	printCrontabCommands();
	while (1) {

		printf("%s", PROMPT_STR);
		fgets(input_str, sizeof(input_str), stdin);
		if (input_str[0] == '\n') continue;
		if (strlen(input_str) > 0) input_str[strlen(input_str) - 1] = '\0';
		else continue;

		input_command = strtok(input_str, " ");
		if (!strcmp(input_command, "add") || !strcmp(input_command, "ADD")) {
			doAddCommand();
		} else if (!strcmp(input_command, "remove") || !strcmp(input_command, "REMOVE")) {
			doRemoveCommand();
		} else if (!strcmp(input_command, "exit") || !strcmp(input_command, "EXIT")) {
			break;
		} else {
			continue;
		}

		getCrontabCommands();
		printCrontabCommands();
	}

}

void printCrontabCommands(void) {
	int i;

	for (i = 0; i < crontab_commands_count; ++i) {
		printf("%d. %s\n", i, crontab_commands[i]);
	}

	printf("\n");
}

void getCrontabCommands(void) {
	FILE *fp;
	int i;

	if ((fp = fopen(CRONTAB_FILE_NAME, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	for (i = 0; i < MAX_COMMAND_COUNT && !feof(fp); ++i) {
		if (fgets(crontab_commands[i], sizeof(crontab_commands[i]), fp) == NULL) {
			break;
		} else {
			if (strlen(crontab_commands[i]) > 0)
				crontab_commands[i][strlen(crontab_commands[i]) - 1] = '\0';
		}
	}
	crontab_commands_count = i;

	fclose(fp);

	return;
}

void doAddCommand(void){

}

void doRemoveCommand(void){

}
