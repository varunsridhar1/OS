#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAXINPUT 2000
#define MAXJOBS 100

#define RUNNING 1
#define STOPPED 2
#define DONE 3

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, FG, BG, JOBS };
int run1, run2;
int addargs1, addargs2;
int pipefd[2];
char* pipeCommand;
int pipeFlag;
int outputRedirect1, outputRedirect2;
int errorRedirect1, errorRedirect2;
int inputRedirect1, inputRedirect2;
FILE* fp1;
FILE* fp2;
FILE* finput1;
FILE* finput2;
int cpid;
int cpid2;
int status;
int bg;
int jobCount = 0;

typedef struct job {
	pid_t pid;
	int jid;
	int state;
	int fg;
	char cmdLine[MAXINPUT];
	struct job * next;
} job_t;

job_t *head = NULL;
job_t *current = NULL;

static void sig_int(int signo) {
	printf("Sending signals to group:%d\n", cpid);
	kill(-cpid, SIGKILL);
}

static void sig_tstp(int signo) {
	printf("Sending SIGTSTP to group:%d\n", cpid);
	kill(-cpid, SIGTSTP);
}

static void sig_chld(int signo) {
	printf("Sending SIGCHLD to group:%d\n", cpid);
	kill(-cpid, SIGCHLD);
}

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
	char *cmdCopy = (char*) malloc(sizeof(char)*2001);
	char *pipecmd = (char*) malloc(sizeof(char)*20);
	while(1) {	
		printf("%s ", "#");
		run1 = 1;		
		run2 = 1;
		addargs1 = 1;
		addargs2 = 1;
		pipeFlag = 0;
		bg = 0;
		outputRedirect1 = -1;
		errorRedirect1 = -1;
		outputRedirect2 = -1;
		errorRedirect2 = -1;

		fgets(cmd, 2001, stdin);
		if(strcmp(cmd, "") != 0) {
			cmd[strlen(cmd) - 1] = '\0';
		}

		strcpy(cmdCopy, cmd);

		if(isBuiltInCommand(cmd) == JOBS) {
			job_t *temp = head;
			while(temp != NULL) {
				printf("[%d]", temp->jid);
				if(temp->fg) {
					printf("+ ");
				}
				else {
					printf("- ");
				}

				if(temp->state == RUNNING) {
					printf("Running\t");
				}
				else if(temp->state = STOPPED) {
					printf("Stopped\t");
				}
				else {
					printf("Done\t");
				}
				printf("%s\n", temp->cmdLine);
				temp = temp->next;
			}
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
					finput1 = fopen(p, "r");
					if(finput1) {
						inputRedirect1 = fileno(finput1);
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

				else if(*p == '&') {
					bg = 1;
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
							finput2 = fopen(pipecmd, "r");
							if(finput2) {
								inputRedirect2 = fileno(finput2);
								addargs2 = 0;
							}
							else {
								fprintf(stderr, "%s %s\n", "Invalid file:", p);
								addargs2 = 0;
								run2 = 0;
							}
						}
						else if(*pipecmd == '2' && *(++pipecmd) == '>') {
							pipecmd = strtok(NULL, " ");
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
					if(inputRedirect1 > 0) {
						dup2(inputRedirect1, STDIN_FILENO);
						fclose(finput1);
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
					job_t *job = malloc(sizeof(job_t));				// add job to struct list
					job->pid = cpid;
					job->jid = ++jobCount;
					if(bg) {
						job->fg = 0;
					}
					else {
						job->fg = 1;
					}
					job->state = RUNNING;
					job->next = NULL;
					strcpy(job->cmdLine, cmdCopy);
					if(head == NULL) {
						head = job;
						current = head;
					}
					else {
						current->next = job;
						current = current->next;
					}

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
								if(inputRedirect2 > 0) {
									dup2(inputRedirect2, STDIN_FILENO);
									fclose(finput2);
								}
								if(errorRedirect2 > 0) {
									dup2(errorRedirect2, STDERR_FILENO);
									fclose(fp2);
								}
								if(execvp(otherargs[0], otherargs) < 0) {
									fprintf(stderr, "%s: %s: %s\n", "yash", otherargs[0], "command not found");
								}
							}
						}
					}
					//if(signal(SIGINT, sig_int) == SIG_ERR) {
					//	printf("Signal(SIGINT) error\n");
					//}
					if(signal(SIGTSTP, sig_tstp) == SIG_ERR) {
						printf("Signal(SIGTSTP) error\n");
					}
					if(!bg) {
						waitpid(-1, &status, WUNTRACED | WCONTINUED);
					}
					else {
						waitpid(-1, &status, WNOWAIT | WCONTINUED);
					}
				}
			}
		}
	}
}


