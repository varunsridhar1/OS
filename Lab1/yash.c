#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, FG, BG, JOBS };
int run1, run2;
int addargs1, addargs2;
int pipefd[2];
char* pipeCommand;
int pipeFlag;

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
	char *cmd = (char*) malloc(sizeof(char)*1001);						// command to execute
	char *pipecmd = (char*) malloc(sizeof(char)*20);
	int cpid;
	int cpid2;
	int fileNo;
	while(1) {	
		printf("%s ", "#");
		run1 = 1;
		addargs1 = 1;
		

		fgets(cmd, 2001, stdin);
		if(strcmp(cmd, "") != 0) {
			cmd[strlen(cmd) - 1] = '\0';
		}

		if(isBuiltInCommand(cmd) == FG) {
			exit(0);
		}
		else {
			if(pipe(pipefd) == -1) {
				perror("pipe");
				exit(EXIT_FAILURE);
			}
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
						addargs1 = 0;
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
							addargs1 = 0;
						}
						else {
							fprintf(stderr, "%s %s\n", "Invalid file:", p);
							addargs1 = 0;
							run1 = 0;
						}
					}
					else if(*p == '2' && *(++p) == '>') {
						p = strtok(NULL, " ");
						FILE* fp = fopen(p, "w");
						fileNo = fileno(fp);
						dup2(fileNo, STDERR_FILENO);
						fclose(fp);
						addargs1 = 0;
					}

					else if(*p == '|') {
						setsid();
						close(pipefd[0]);
						dup2(pipefd[1], STDOUT_FILENO);
						pipeFlag = 1;
						addargs1 = 0;
						// set global substring with all text on right side of |
						char* temp = strstr(cmd, "|");
						pipecmd = strtok(temp, " ");

					}

					if(addargs1) {
						myargs[i++] = p;
					}
					p = strtok(NULL, " ");
				}	
				myargs[i] = NULL;
				if(run1) {
					if(execvp(myargs[0], myargs) < 0) {
						printf("%s: %s: %s\n", "yash", myargs[0], "command not found");
					}
				}
			}
			else {	// Parent
				if(pipeFlag) { 
					cpid2 = fork();
					if(cpid2 == 0) { // child 2
						// use global substring and loop through and put in args array
						close(pipefd[1]);
						dup2(pipefd[0], STDIN_FILENO);
						run2 = 1;
						addargs2 = 1;
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
								addargs2 = 0;
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
									addargs2 = 0;
								}
								else {
									fprintf(stderr, "%s %s\n", "Invalid file:", p);
									addargs2 = 0;
									run2 = 0;
								}
							}
							else if(*p == '2' && *(++p) == '>') {
								p = strtok(NULL, " ");
								FILE* fp = fopen(p, "w");
								fileNo = fileno(fp);
								dup2(fileNo, STDERR_FILENO);
								fclose(fp);
								addargs2 = 0;
							}

							if(addargs2) {
								myargs[i++] = p;
							}
							p = strtok(NULL, " ");
						}	
						myargs[i] = NULL;
						if(run2) {
							if(execvp(myargs[0], myargs) < 0) {
								printf("%s: %s: %s\n", "yash", myargs[0], "command not found");
							}
						}
					}
					else {
						pipeFlag = 0;
						wait(NULL);
					}
				}
				if(run1) {
					wait(NULL);
				}
				
			}
				
		}
	}
}


