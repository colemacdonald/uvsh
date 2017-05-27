/* uvsh.c

Requirements: 	https://connex.csc.uvic.ca/access/content/attachment/9ad57451-aeeb-44ea-9900-3e4871d16fa4/Assignments/29d50c2c-9759-4035-a9f0-5694475f2278/2017_summer_a1_writeup.pdf

Github Link: 	https://github.com/colemacdonald/shell

Created On: 	Wed May 24
By:				Cole Macdonald

*/

/******************************* LIBRARIES *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/******************************* CONSTANTS *******************************/

#define MAX_NUM_ARGS 			9
#define MAX_LINE_LENGTH 		80
#define MAX_PROMPT_LENGTH		10
#define MAX_NUM_DIRS_IN_PATH 	10

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
int 		run_cmd(char* cmd);
int 		tokenize_cmd(char ** token, char * cmd);

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
	terminate_line(cmd);

	char *token[MAX_NUM_ARGS];
	int num_tokens = tokenize_cmd(token, cmd);

	/* Note that an address stored in some token[i] is actually an address
     * to a char location within the input array.
     */
    int i = 0;;
    /*for (i = 0; i < num_tokens; i++) {
        printf("%d: %s\n", i, token[i]);
    } */

	char to_run[MAX_LINE_LENGTH];
	to_run[0] = '\0';
	// for(i = 0; i < _num_dirs; i++)
	// { //look for binary, if exists, execve
		//printf("dir: %s\n", _dirs[i]);
		strcat(to_run, _dirs[i]);
		strcat(to_run, "/");
		strcat(to_run, token[0]);

		//token[0] = to_run;
		char* envp[] = { 0 };

		if( (_pid = fork()) == 0)
		{
			//child process
			token[0] = to_run;
			token[num_tokens] = 0;
			if(execve(token[0], token, envp) == -1)
			{
				fprintf(stderr, "Error: execve failed.\n");
				exit(0);
			}
		}
		while (wait(&_pid) > 0); //{ /* child process finished */ } 
	// }
	return 1;
}

int tokenize_cmd(char ** token, char * cmd)
{
    char *t;
    int  num_tokens = 0;

    t = strtok(cmd, " ");
    while (t != NULL && num_tokens < MAX_NUM_ARGS) {
        token[num_tokens] = t;
        num_tokens++;
        t = strtok(NULL, " ");
    }

    token[num_tokens] = NULL;
    
    return num_tokens;
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