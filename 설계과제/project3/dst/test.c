#include <stdio.h>
#include <time.h>

int main(){
	time_t current_time;
	time(&current_time);
	printf("%ld\n",current_time);
	printf("%s", ctime(&current_time));
}
