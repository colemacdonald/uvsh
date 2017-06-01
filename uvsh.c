/* uvsh.c

Requirements: 	https://connex.csc.uvic.ca/access/content/attachment/9ad57451-aeeb-44ea-9900-3e4871d16fa4/Assignments/29d50c2c-9759-4035-a9f0-5694475f2278/2017_summer_a1_writeup.pdf

Github Link: 	https://github.com/colemacdonald/rng-name-maker

Created On: 	May 24, 2017
By:				Cole Macdonald

*/

/******************************* LIBRARIES *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

/******************************* CONSTANTS *******************************/

#define MAX_NUM_ARGS 			15 				// 9
#define MAX_LINE_LENGTH 		100				// 80
#define MAX_PROMPT_LENGTH		15 				// 10
#define MAX_NUM_DIRS_IN_PATH 	15 				// 10

/******************************** GLOBALS *******************************/

char 	_prompt[MAX_PROMPT_LENGTH];
char  	_dirs[MAX_NUM_DIRS_IN_PATH][MAX_LINE_LENGTH];
int 	_num_dirs;
int 	_pid;

/****************************** PROTOTYPES ******************************/

int 		read_config();
int 		get_prompt(FILE * fp);
int 		get_dirs(FILE * fp);
void 		prompt_user(char * prompt);
int 		read_user_input(char* input);

/* Takes string cmd and determines whether it is a standard, 
do-out, or do-pipe cmd and runs accordingly*/
int 		run_cmd(char* cmd);
/* Breaks cmd into words */
int 		tokenize_cmd(char ** token, char * cmd);
/* Spawns a new process and attempts to run binary with args */
int 		fork_exec(char * binary, char ** args, int num_tokens);
/* Spawns a new process and attempts to run binary with args, redirecting to output */
int 		fork_exec_out(char * binary, char ** args, int num_tokens, char * output);
/* Spawns a new process and attempts to pipe the output of binary 1 with args1 to binary2 with args 2 */
int 		fork_exec_pipe(char * binary1, char ** args1, char * binary2, char ** args2);
/* Called by run_cmd */
int 		run_std(char * cmd);
/* Called by run_cmd */
int 		run_out(char * cmd);
/* Called by run_cmd */
int 		run_pipe(char * cmd);
/* Determines if cmd exists, and saves the full path to the binary in fullpath */
int 		find_binary(char * bin_name, char * fullpath);

/*************************** IMPLEMENTATIONS ****************************/

// HELPERS

void terminate_line(char * str)
{
	if (str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = '\0';
    }
}

// END HELPERS

int read_config()
{
	FILE * fp;
	if( !(fp = fopen(".uvshrc", "r")) )
	{
		printf("Config file '.uvshrc' could not be found in current directory.\n");
		return 0;
	}

	get_prompt(fp);
	get_dirs(fp);

	if(fp)
		fclose(fp);
	return 1;
}

int get_prompt(FILE * fp)
{
	fgets(_prompt, MAX_PROMPT_LENGTH, fp);
	terminate_line(_prompt);

	return 1;
}

int get_dirs(FILE * fp)
{
	_num_dirs = 0;
	while(fgets(_dirs[_num_dirs], MAX_LINE_LENGTH, fp) && _num_dirs < MAX_NUM_DIRS_IN_PATH)
	{
		terminate_line(_dirs[_num_dirs]);
		_num_dirs++;
	}

	return 1;
}

void prompt_user(char * prompt)
{
	fprintf(stdout, "%s ", prompt);
	fflush(stdout);
}

int read_user_input(char* input)
{
	fgets(input, MAX_LINE_LENGTH, stdin);
	terminate_line(input);

    if(strcmp(input, "exit") == 0)
    {
    	printf("Bye!\n");
		exit(0);
	}
    return 1;
}

int run_cmd(char* cmd)
{
	if(strcmp(cmd, "") == 0)
		return 1;

	terminate_line(cmd);

	if(strncmp(cmd, "do-pipe", 7) == 0)
	{
		//handle pipe
		return run_pipe(cmd);
	}
	if(strncmp(cmd, "do-out", 6) == 0)
	{
		//handle out
		return run_out(cmd);
	}
	return run_std(cmd);
}

int run_std(char * cmd)
{
	char *token[MAX_NUM_ARGS];
	int num_tokens = tokenize_cmd(token, cmd);

	char to_run[MAX_LINE_LENGTH];
	to_run[0] = '\0';

	int found_binary = find_binary(token[0], to_run);

	if(found_binary)
	{
		fork_exec(to_run, token, num_tokens);
		return 1;
	}
	else
	{
		fprintf(stderr, "Invalid command.\n");
		return 0;
	}
	return 1;
}

int run_out(char * cmd)
{
	char *token[MAX_NUM_ARGS];
	int num_tokens = tokenize_cmd(token, cmd);

	char to_run[MAX_LINE_LENGTH];
	to_run[0] = '\0';

	if(strcmp(token[num_tokens - 2], "::") != 0 || num_tokens < 4)
	{
		fprintf(stderr, "Incorrect syntax. Called as follows:\ndo-out <cmd1> :: <output-file>\n");
		return 0;
	}

	char * token1[num_tokens - 2];
	int i;
	for(i = 1; i < num_tokens - 2; i++)
	{
		token1[i - 1] = token[i];
	}
	token1[num_tokens - 3] = NULL;

	char * output = token[num_tokens - 1];
	num_tokens -= 3;

	int found_binary = find_binary(token1[0], to_run);
	if(found_binary)
	{
		fork_exec_out(to_run, token1, num_tokens, output);
		return 1;
	}
	else
	{
		fprintf(stderr, "Invalid command.\n");
		return 0;
	}
	return 1;
}

int run_pipe(char * cmd)
{
	char *token[MAX_NUM_ARGS];
	int num_tokens = tokenize_cmd(token, cmd);

	//find ::
	int i;
	for(i = 1; i < num_tokens; i++)
	{
		if(strcmp(token[i], "::") == 0)
			break;
	}

	if(i == 1)
	{
		fprintf(stderr, "Missing command before pipe.\n");
		return 0;
	}
	else if(i == num_tokens - 1)
	{
		fprintf(stderr, "Missing command after pipe.\n");
		return 0;
	}

	int j;
	
	char *token1[i];
	for(j = 1; j < i; j++)
		token1[j-1] = token[j];
	token1[i - 1] = 0;

	char *token2[num_tokens - i];
	for(j = i + 1; j < num_tokens; j++)
		token2[j - (i + 1)] = token[j];
	token2[num_tokens - i - 1] = 0;

	char binary1[MAX_LINE_LENGTH];
	char binary2[MAX_LINE_LENGTH];

	int found_binary1 = find_binary(token1[0], binary1);
	int found_binary2 = find_binary(token2[0], binary2);

	if(!found_binary1)
	{
		fprintf(stderr, "Invalid command before pipe.");
		return 0;
	}
	else if(!found_binary2)
	{
		fprintf(stderr, "Invalid command after pipe.");
		return 0;
	}

	return fork_exec_pipe(binary1, token1, binary2, token2);
}

int tokenize_cmd(char ** token, char * cmd)
{
    char *t;
    int  num_tokens = 0;

    t = strtok(cmd, " ");
    while (t != NULL && num_tokens < MAX_NUM_ARGS) {
        token[num_tokens] = t;
        t = strtok(NULL, " ");

        num_tokens++;
    }

    token[num_tokens] = NULL;

    int i;
    for(i = 0; i < num_tokens; i++)
    {
    	char * t = token[i];
    	if(strncmp(t, "~", 1) == 0)
    	{
    		char tmp[strlen(t) + 1];
    		memcpy(tmp, t, strlen(t));
    		tmp[strlen(t)] = '\0';

    		t[0] = '\0';
    		strcat(t, getenv("HOME"));
    		strcat(t, &tmp[1]);
    		strcat(t, "\0");
    	}
    	else if(strncmp(t, "./", 2) == 0)
    	{
    		char tmp[strlen(t) + 1];
    		memcpy(tmp, t, strlen(t));
    		tmp[strlen(t)] = '\0';

    		// https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
    		char cwd[1024];
    		getcwd(cwd, sizeof(cwd));

    		t[0] = '\0';
    		strcat(t, cwd);
    		strcat(t, &tmp[1]);
    		strcat(t, "\0");
    	}
    }
    return num_tokens;
}

int fork_exec(char * binary, char ** args, int num_tokens)
{
	char* envp[] = { 0 };

	if( (_pid = fork()) == 0)
	{
		args[0] = binary;
		args[num_tokens] = 0;

		if(execve(args[0], args, envp) == -1)
		{
			fprintf(stderr, "Error: execve failed.\n");
			exit(0);
		}
	}
	while (wait(&_pid) > 0); //{ /* child process finished */ } 

	return 1;
}

int fork_exec_out(char * binary, char ** args, int num_tokens, char * output)
{
	char* envp[] = { 0 };

	if( (_pid = fork()) == 0)
	{
		//child process
		int fd;
		fd = open(output, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
		if(!fd)
		{
			fprintf(stderr, "Cannot open output file for writing.\n");
			return 0;
		}

		// redirect stdout and stderr respectively
		dup2(fd, 1);
		dup2(fd, 2);

		args[0] = binary;
		args[num_tokens] = 0;
		if(execve(args[0], args, envp) == -1)
		{
			fprintf(stderr, "Error: execve failed.\n");
			exit(0);
		}
	}
	while (wait(&_pid) > 0); //{ /* child process finished */ } 

	return 1;
}

int fork_exec_pipe(char * binary1, char ** args1, char * binary2, char ** args2)
{
	char * envp[] = { 0 };
	
	int status;
	int pid1, pid2;
	int fd[2];

	args1[0] = binary1;
	args2[0] = binary2;

	pipe(fd);

    if((pid1 = fork()) == 0)
    {
        dup2(fd[1], 1);
        close(fd[0]);
        if(execve(args1[0], args1, envp) == -1)
        {
        	fprintf(stderr, "Error: execve 1 failed.\n");
			exit(0);
        }
    }

    if((pid2 = fork()) == 0)
    {
        dup2(fd[0], 0);
        close(fd[1]);
        if(execve(args2[0], args2, envp) == -1)
        {
        	fprintf(stderr, "Error: execve 2 failed.\n");
			exit(0);
        }
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0); 

    return 1;
}

int find_binary(char * bin_name, char * fullpath)
{
	int i = 0;

	char to_run[MAX_LINE_LENGTH];

	//check for bin_name already being full path
	FILE * bfp = fopen(bin_name, "r");
	if(bfp)
	{
		memcpy(fullpath, bin_name, strlen(bin_name) + 1);
		fclose(bfp);
		return 1;
	}

	for(i = 0; i < _num_dirs; i++)
	{
		to_run[0] = '\0';
		strcat(to_run, _dirs[i]);
		strcat(to_run, "/");
		strcat(to_run, bin_name);
		strcat(to_run, "\0");

		//look for binary
		bfp = fopen(to_run, "r");

		if(bfp)
		{
			memcpy(fullpath, to_run, strlen(to_run) + 1);
			fclose(bfp);
			return 1;
		}
	}
	memcpy(fullpath, "\0", 1);
	return 0;
}

/********************************* MAIN *********************************/

int main(void)
{
	read_config();
	for(;;)
	{
		prompt_user(_prompt);

		char input[MAX_LINE_LENGTH];
		read_user_input(input);

		run_cmd(input);
	}

	return 0;
}