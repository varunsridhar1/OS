#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT, FG, BG, JOBS };
int pipefd[2];

int isBuiltInCommand(char *cmd) {
	if(strncmp(cmd, "exit", strlen("exit")) == 0) {
		return EXIT;	
	}
	if(strncmp(cmd, "fg", strlen("fg")) == 0) {
		return FG;
	}
	if(strncmp(cmd, "bg", strlen("bg")) == 0) {
		return BG;
	}
	if(strncmp(cmd, "jobs", strlen("jobs")) == 0) {
		return JOBS;
	}
	return NO_SUCH_BUILTIN;
}

int main(int argc, char **argv) {
	char cmd[256];						// command to execute
	int cpid;
	while(1) {	
		printf("%s ", ">yash");

		fgets(cmd, sizeof(cmd), stdin);
		if(strcmp(cmd, "") != 0) {
			cmd[strlen(cmd) - 1] = '\0';
		}

		if(isBuiltInCommand(cmd) == EXIT) {
			exit(0);
		}
		else {
			cpid = fork();
			if(cpid == 0) {
				char *myargs[256];
				int i = 0;
				char *p = strtok(cmd, " ");
				
				while(p != NULL) {
					if(*p == '>') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "w");					
					}
					myargs[i++] = p;
					p = strtok(NULL, " ");
				}
				myargs[i] = NULL;	
				execvp(myargs[0], myargs);
			}
			else {
				wait(NULL);
			}
				
		}
	}
}


