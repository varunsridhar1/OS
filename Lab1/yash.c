#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT, FG, BG, JOBS };
int run;

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
	int fileNo;
	while(1) {	
		printf("%s ", ">yash");
		run = 1;

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
						fileNo = fileno(fp);
						dup2(fileNo, STDOUT_FILENO);
						fclose(fp);
						break;
					}
					else if(*p == '<') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "r");
						if(fp) {
							fileNo = fileno(fp);
							dup2(fileNo, STDIN_FILENO);
							char* arg;
							fscanf(fp, "%s", arg);
							myargs[i++] = arg;
							fclose(fp);
							break;
						}
						else {
							fprintf(stderr, "%s\n", "No such file or directory");
							run = 0;
							break;
						}
					}
					else if(*p == '2' && *(p++) == '>') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "w");
						fileNo = fileno(fp);
						dup2(fileNo, STDERR_FILENO);
						fclose(fp);
						break;
					}
					myargs[i++] = p;
					p = strtok(NULL, " ");
				}	
				myargs[i] = NULL;	
				if(run) {
					execvp(myargs[0], myargs);
				}
				run = 1;
			}
			else {
				wait(NULL);
			}
				
		}
	}
}


