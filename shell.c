#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

#define SIZE 1024

char *readline() {
	char c;
	int n, pos = 0;
	char *buffer = (char *) malloc(SIZE);

	while (( n = read(0, &c, 1)) > 0 ) {
		if ( c == '\n' ) {
			buffer[pos] = '\0';
			break;
		}
		buffer[pos++] = c;
	}

	return buffer;
}

int main () {
	char *buf;;
	char *const arg[10];
		
	while (1) {
		int len;
		write(1, "> ", 2);
		buf = readline();
		int pid = fork();
		if (pid == 0) {  	//child process
			execl(buf, buf, "-l", NULL);
		}
		else if (pid > 0) { 	//parent process
			wait(NULL);
		}
		else{
			printf ("Error in fork!\n");
		}
		free(buf);
	}	

	return 0;
}

