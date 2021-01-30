#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>

#define SIZE 1024

//global variables
int cmd_flag = 0;

typedef struct redirect {
	int fd;
	char *filename;
}redirect;

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
	//if (n == 0)
	//	write(1, "\n", 1);

	return buffer;
}

char** parse_pipe(char *line) {
	char **commands = (char **) malloc(sizeof(char *) * 10);	
	char *delim = "|", *token;
	int i = 0;
	
	token = strtok(line, delim);
	if (!token) {
		commands[i++] = line;
	}
	while (token) {
		cmd_flag++;
		commands[i++] = token;
		token = strtok(NULL, delim);
	}
	commands[i] = NULL;
	return commands;
}


char** parse_args(char *line) {
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

int create_process (char *command) {
	char **args = parse_args(command);
	int pid = fork();
	if (pid == 0) { 	//child process
		if ( execvp(args[0], args) == -1 ) {
			perror("exec failed"); 		//if the command not found
		}

	}
	else if (pid > 0) { 	//parent process
		wait(NULL);
	}
	else{
		perror("fork failed");
	}
	free(args);
	return 0;	
}

int create_pipe(char **commands) {
	int i, pid, nopipes;
	int pipefd[100];
	char **args;
	for (i = 0; i < cmd_flag; i++) {
		args = parse_args(commands[i]);
		if (i != cmd_flag -1) { 
			pipe(&pipefd[2 * i]);
			printf ("pipefd %d %d\n", pipefd[2*i], pipefd[2*i+1]);
		}
		pid = fork();
		if (pid == 0) {

			//if not the first command, open pipe for reading
			if (i != 0) {
				//old read fd to 0
				if (dup2(pipefd[(2 * i) - 2], 0) < 0) {
					perror ("dup2 failed");
					break;
				}
			}

			//if not the last command, open pipe for writing
			if (i != cmd_flag - 1) {
				if (dup2(pipefd[(2 * i )+ 1], 1) < 0) {
					perror ("dup2 failed");
					break;
				}

			}

			//close all pipefds
			for (nopipes = 0; nopipes <= ((2 * i) + 1); nopipes++) {
				if (i == cmd_flag - 1 && nopipes == 2 * i)
					break;	
				close(pipefd[nopipes]);
			}

			//exec the new command
			if (execvp(args[0], args) == -1 ) {
				perror ("exec failed");
				break;
			}
	

		}

		else if (pid > 0){
			//if (i == cmd_flag - 1) {
			//	for (nopipes = 0; nopipes < cmd_flag; nopipes++) {
			//		wait(NULL);
			//		printf ("%d child\n", nopipes);
			//	}
			//	//wait(NULL);
			//}
			//
			continue;
		}
		else {
			perror ("fork failed");
		}
	}

	//while (wait(NULL) > 0);

	for (nopipes = 0; nopipes <= ((2 * i) - 3); nopipes++)
		close(pipefd[nopipes]);
	return 0;

}


int main () {
	char *buf, **commands;
	int i;
		
	while (1) {
		printf("\n");
		write(1, "prompt> ", strlen("prompt> "));
		buf = readline();
		cmd_flag = 0;
		commands = parse_pipe(buf);
		if (cmd_flag == 1) {
			i = create_process(commands[0]);
		}
		else {
			i = create_pipe(commands);
			//free(commands);
			//free(buf);
			//break;
		}
		//for (i = 0; i < nocmd; i++) {
		//	if (cmd_flag - 1) {
		//		printf ("%d\n", nocmd);
		//		//pipe(&pipefd[2 * nocmd]);
		//	}
		//	else {
		//		cmd_flag--;
		//		create_process(commands[i]);	
		//	}
		//}
		free(commands);
		free(buf);
		//args = parse_args();
		//int pid = fork();
		//if (pid == 0) { 	//child process
		//	if ( execvp(args[0], args) == -1 ) {
		//		perror("exec failed"); 		//if the command not found
		//	}

		//}
		//else if (pid > 0) { 	//parent process
		//	wait(NULL);
		//}
		//else{
		//	perror("fork failed");
		//}
		//free(buf);
		//free(args);
		//free(commands);
	}	
	return 0;
}
