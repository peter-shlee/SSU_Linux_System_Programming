#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(void)
{
	char operator;
	FILE *fp;
	int character;
	int number = 0;

	if ((fp = fopen("ssu_expr.txt", "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", "ssu_expr.txt");
		exit(1);
	}

	while (!feof(fp)) {
		while ((character = fgetc(fp)) != EOF && isdigit(character))
			number = number * 10 + character - 48;

		fprintf(stdout, " %d\n", number);
		number = 0;

		if (character != EOF) {
			ungetc(character, fp);
			operator = fgetc(fp);
			printf("Operator => %c\n", operator);
		}
	}

	fclose(fp);
	exit(0);
}

