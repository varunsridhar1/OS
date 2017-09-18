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
int outputRedirect1, outputRedirect2;
int errorRedirect1, errorRedirect2;
FILE* fp1;
FILE* fp2;
int cpid;
int cpid2;

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
	while(1) {	
		printf("%s ", "#");
		run1 = 1;		
		run2 = 1;
		addargs1 = 1;
		addargs2 = 1;
		pipeFlag = 0;
		outputRedirect1 = -1;
		errorRedirect1 = -1;
		outputRedirect2 = -1;
		errorRedirect2 = -1;

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
			char **myargs = (char**) malloc(sizeof(char*) * 10);
			char **otherargs = (char**) malloc(sizeof(char*) * 10);
			int i = 0;
			char *p = strtok(cmd, " ");
				
			while(p != NULL) {
				if(*p == '>') {
					p = strtok(NULL, " ");
					fp1 = fopen(p, "w");
					outputRedirect1 = fileno(fp1);
					addargs1 = 0;
				}
				else if(*p == '<') {
					p = strtok(NULL, " ");
					FILE* fp = fopen(p, "r");
					if(fp1) {
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
					fp1 = fopen(p, "w");
					errorRedirect1 = fileno(fp1);
					addargs1 = 0;
				}

				else if(*p == '|') {
					pipeFlag = 1;
					addargs1 = 0;
					addargs2 = 1;
					// set global substring with all text on right side of |
					char* temp = strstr(cmd, "|");
					pipecmd = strtok(temp, " ");
					int j = 0;
					while(pipecmd != NULL) {
						if(*pipecmd == '>') {
							pipecmd = strtok(NULL, " ");
							fp2 = fopen(pipecmd, "w");
							outputRedirect2 = fileno(fp2);
							addargs2 = 0;
						}
						else if(*pipecmd == '<') {
							pipecmd = strtok(NULL, " ");
							FILE* fp = fopen(p, "r");
							if(fp) {
								char* arg;
								int a = fscanf(fp, "%s", arg);
								otherargs[j++] = arg;
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
							fp2 = fopen(p, "w");
							errorRedirect2 = fileno(fp2);
							addargs2 = 0;
						}

						if(addargs2) {
							otherargs[j++] = pipecmd;
						}
						pipecmd = strtok(NULL, " ");
					}
					otherargs[j] = NULL;
				}

				if(addargs1) {
					myargs[i++] = p;
				}
				p = strtok(NULL, " ");
			}	

			myargs[i] = NULL;
			if(run1) {
				cpid = fork();
				if(cpid == 0) { // child 1
					if(pipeFlag) {
						setsid();
						close(pipefd[0]);
						dup2(pipefd[1], STDOUT_FILENO);
					}
					if(outputRedirect1 > 0) {
						dup2(outputRedirect1, STDOUT_FILENO);
						fclose(fp1);
					}		
					if(errorRedirect1 > 0) {
						dup2(errorRedirect1, STDERR_FILENO);
						fclose(fp1);
					}
					if(execvp(myargs[0], myargs) < 0) {
						fprintf(stderr, "%s: %s: %s\n", "yash", myargs[0], "command not found");
					}
				}
				else {
					if(pipeFlag) {
						if(run2) {
							cpid2 = fork();
							if(cpid2 == 0) { // child 2
								setpgid(0, cpid);
								dup2(pipefd[0], STDIN_FILENO);
								close(pipefd[1]);
								if(outputRedirect2 > 0) {
									dup2(outputRedirect2, STDOUT_FILENO);
									fclose(fp2);
								}
								if(errorRedirect2 > 0) {
									dup2(errorRedirect2, STDERR_FILENO);
									fclose(fp2);
								}
								if(execvp(otherargs[0], otherargs) < 0) {
									fprintf(stderr, "%s: %s: %s\n", "yash", otherargs[0], "command not found");
								}
							}
							else {
								wait(NULL);
							}
						}
					}
					wait(NULL);
				}
			}
		}
	}
}


