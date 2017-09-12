#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT, FG, BG, JOBS };

void printPrompt(void) {
	char cwd[256];
	char *username = getenv("USER");

	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);

	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s@%s:~%s$ " , username, hostname, cwd);
	} else {
		fprintf(stderr, "%s", "Error getting directory.\n");
	}
}

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
				char *myargs[2];
				myargs[0] = strdup(cmd);
				myargs[1] = NULL;	
				execvp(myargs[0], myargs);
			}
			else {
				wait(NULL);
			}
				
		}
	}
}


