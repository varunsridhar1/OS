#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, FG, BG, JOBS };
int run;
int addargs;

int isBuiltInCommand(char *cmd) {
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
	char *cmd = (char*) malloc(sizeof(char)*2001);						// command to execute
	int cpid;
	int fileNo;
	while(1) {	
		printf("%s ", "#");
		run = 1;
		addargs = 1;

		fgets(cmd, 2001, stdin);
		if(strcmp(cmd, "") != 0) {
			cmd[strlen(cmd) - 1] = '\0';
		}

		if(isBuiltInCommand(cmd) == FG) {
			exit(0);
		}
		else {
			cpid = fork();
			if(cpid == 0) {
				char **myargs = (char**) malloc(sizeof(char*) * 10);
				int i = 0;
				char *p = strtok(cmd, " ");
				
				while(p != NULL) {
					if(*p == '>') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "w");
						fileNo = fileno(fp);
						dup2(fileNo, STDOUT_FILENO);
						fclose(fp);
						addargs = 0;
					}
					else if(*p == '<') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "r");
						if(fp) {
							//fileNo = fileno(fp);
							//dup2(fileNo, STDIN_FILENO);
							char* arg;
							int a = fscanf(fp, "%s", arg);
							myargs[i++] = arg;
							fclose(fp);
							addargs = 0;
						}
						else {
							fprintf(stderr, "%s %s\n", "Invalid file:", p);
							addargs = 0;
							run = 0;
						}
					}
					else if(*p == '2' && *(++p) == '>') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "w");
						fileNo = fileno(fp);
						dup2(fileNo, STDERR_FILENO);
						fclose(fp);
						addargs = 0;
					}
					if(addargs) {
						myargs[i++] = p;
					}
					p = strtok(NULL, " ");
				}	
				myargs[i] = NULL;
				if(run) {
					if(execvp(myargs[0], myargs) < 0) {
						printf("%s: %s: %s\n", "yash", myargs[0], "command not found");
					}
				}
			}
			else {
				if(run) {
					wait(NULL);
				}
			}
				
		}
	}
}


