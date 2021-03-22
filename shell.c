#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define SIZE 1024
#define MAX_FILES 50
#define MAX_PIPES 50
#define MAX_ARGS 50


//global variables
int cmd_flag = 0;
int inredirect_flag = 0, outredirect_flag = 0;


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

	return buffer;
}

char** parse_pipe(char *line) {
	char **commands = (char **) malloc(sizeof(char *) * MAX_PIPES);	
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
	char **args = (char **) malloc( sizeof(char* ) * MAX_ARGS );
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

int parse_redirect(char *line, char ch, char **ptr) {
	char* token, filename[100];
	int i;
	while ((token = strchr(line, ch))) {
		i = 0;
		*token = ' ';
		token++;
		if (i == 0) {
			while(*token == ' ')
				token++;
		}
		while (*token != '\0' &&  *token != ' ' &&  *token != '\t') {
			filename[i++] = *token;
			*token = ' ';
			token++;
		}
		if (i == 0) {
			return 1;
		}
		filename[i] = '\0';
		if (ch == '>'){
			ptr[outredirect_flag] = (char *) malloc (strlen(filename) + 1);
			strcpy(ptr[outredirect_flag++], filename);
		}
		else {
			ptr[inredirect_flag] = (char *) malloc (strlen(filename) + 1);
			strcpy(ptr[inredirect_flag++], filename);
		}
			
	}
	return 0;
}

void redirect(char **infile, char **outfile) {
	int i, j, n, oldfd, newfd;
	char **files;
	for (j = 0; j < 2; j++) {
		if (j == 0) {
			files = infile;
			n = inredirect_flag;
			newfd = 0;
		}
		else {
			files = outfile;
			n = outredirect_flag;
			newfd = 1;
		}
		for (i = 0; i < n; i++) {
			if (j == 1) {
				//close(1);
				if((oldfd = open(files[i], O_WRONLY | O_CREAT, 0644)) == -1 )
					printf ("Invalid file");
				
			}
			else {
				//close(0);
				if((oldfd = open(files[i], O_RDONLY)) == -1)
						printf ("Invalid file");
			}
			//printf ("%d\n", oldfd);
			if(dup2(oldfd, newfd) == -1) {
				perror("dup failed");
			}
			close(oldfd);
			free(files[i]);
		}
	}
}

void create_process (char *command) {
	char **infile, **outfile, **args;
	int wstatus;
	int pid = fork();
	if (pid == 0) { 	//child process
		//set signal handlers to default
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);

		infile = (char **) malloc (sizeof(char *) * MAX_FILES);
		outfile = (char **) malloc (sizeof(char *) * MAX_FILES);

		inredirect_flag = 0;
		outredirect_flag = 0;
		if (parse_redirect(command, '<', infile) == 1) {
			printf ("Syntax error near '<' ");
			return;
		}

		if (parse_redirect(command, '>', outfile) == 1) {
			printf ("Syntax error near '>' ");
			return;
		}

		//
		args = parse_args(command);

		redirect(infile, outfile);
		//for cd command
		if (!strcmp(args[0],"cd")) {
			chdir(args[1]);
		}
		else if ( execvp(args[0], args) == -1 ) {
			perror("exec failed"); 		//if the command not found
		}


	}

	else if (pid > 0) { 	//parent process

		//wait for the child process to change state (term/stop)
		waitpid(-1, &wstatus, WUNTRACED);
		//if (WIFEXITED(wstatus)) {
                //       printf("exited, status=%d\n", WEXITSTATUS(wstatus));
		//} 
		if (WIFSIGNALED(wstatus)) {
                       printf("\n");
		} 
		else if (WIFSTOPPED(wstatus)) {
                       printf("Stopped\n");
		} 
		else if (WIFCONTINUED(wstatus)) {
                       printf("continued\n");
		}
	}

	else{
		perror("fork failed");
		return;
	}

	return;	
}

void create_pipe(char **commands) {
	int i, pid, nopipes, wstatus;
	int pipefd[MAX_PIPES];
	char **args, **infile, **outfile;

	for (i = 0; i < cmd_flag; i++) {
		//create pipe except the last process
		if (i != cmd_flag -1) { 
			pipe(&pipefd[2 * i]);
		}

		pid = fork();

		if (pid == 0) {
			//set signal handlers to default
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);

			infile = (char **) malloc (sizeof(char *) * MAX_FILES);
			outfile = (char **) malloc (sizeof(char *) * 10);

			inredirect_flag = 0;
			outredirect_flag = 0;
			if (parse_redirect(commands[i], '<', infile) == 1) {
				printf ("Syntax error near '<' ");
				return;
			}

			if (parse_redirect(commands[i], '>', outfile) == 1) {
				printf ("Syntax error near '>' ");
				return;
			}
			args = parse_args(commands[i]);

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
				if (dup2(pipefd[(2 * i)+ 1], 1) < 0) {
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
			
			redirect(infile, outfile);
			free (infile);
			free (outfile);

			//exec the new command
			if (execvp(args[0], args) == -1 ) {
				perror ("exec failed");
				break;
			}
	

		}

		else if (pid > 0){
			continue;
		}

		else {
			perror ("fork failed");
			return;
		}
	}
	
	//close all the pipes in the parent process
	for (nopipes = 0; nopipes <= ((2 * i) - 3); nopipes++)
		close(pipefd[nopipes]);

	// wait till all the child processes terminate/change state
	for (i = 0; i < cmd_flag; i++) {

		//wait for the child process to change state (term/stop)
		waitpid(-1, &wstatus, WUNTRACED);

		//if (WIFEXITED(wstatus)) {
                //       printf("exited, status=%d\n", WEXITSTATUS(wstatus));
		//} 
		if (WIFSIGNALED(wstatus)) {
                       printf("\n");
		} 
		else if (WIFSTOPPED(wstatus)) {
                       printf("Stopped\n");
		} 
		else if (WIFCONTINUED(wstatus)) {
                       printf("continued\n");
		}

	}

	return ;

}

void handle(int signum){
	char *prompt = "\r\n\033[0;33mprompt> \033[0m";
	write(1, prompt, strlen(prompt));
	//printf ("\n\033[0;33mprompt> \033[0m");
}

int main () {
	char *buf, **commands;
	char *prompt = "\033[0;33mprompt> \033[0m";
	
	//handle signals
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	while (1) {
		write(1, prompt, strlen(prompt));
		buf = readline();
		
		//when exit is entered --> terminate
		if (!(strcasecmp(buf, "exit")))
			break;

		cmd_flag = 0;
		commands = parse_pipe(buf);

		if (cmd_flag == 1) {
			 create_process(commands[0]);
		}
		else {
			 create_pipe(commands);
		}

		free(commands);
		free(buf);
	}	
	return 0;
}
