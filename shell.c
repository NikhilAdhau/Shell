#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

#define SIZE 1024

char* readline() {
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
	if (n == 0)
		write(1, "\n", 1);

	return buffer;
}

char** parse(char *line) {
	char *delim = " \t", *token;
	char **args = (char **) malloc( sizeof(char* ) * 100 );
	int i = 0;
	token = strtok(line, delim);
	if (!token) {
		args[i++] = line;
	}
	while (token) {
		args[i++] = token;
		token = strtok(NULL, delim);
	}
	args[i] = NULL;
	return args;

}

int main () {
	char *buf, **args;
		
	while (1) {
		int len;
		write(1, "> ", 2);
		buf = readline();
		args = parse(buf);
		int pid = fork();
		if (pid == 0) {  	//child process
			execve(buf, args, NULL);
		}
		else if (pid > 0) { 	//parent process
			wait(NULL);
		}
		else{
			printf ("Error in fork!\n");
		}
		free(buf);
		free(args);
	}	

	return 0;
}

