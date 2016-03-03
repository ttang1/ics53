/*
	Thomas Tang
	82502633
	ICS53 HW6
	my_shell.c
*/
#include <fcntl.h> // for open()
#include <stdio.h>	
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // read() write() close()
#include <sys/types.h>
#include <limits.h>


#define FOREGROUND 'F'
#define BACKGROUND 'B'
#define WRPIPE 1
#define RDPIPE 0

void execute(char** args);
int is_space(char ch);
int is_redirection(char ch);
int is_bg_pipe(char ch);
void parse(char* input, char** args);
void runpipe(int fdpipe[], char** p_args, char** ch_args);
void handle_input_redirection(int fdin, char* infile)
void handle_output_redirection(int fdout, char* outfile)

void shell()
{	
	char input[1024];
	char* args[64] = {NULL};

	while(1)
	{
		fputs("% ", stdout); // print prompt
		fgets(input, 1024, stdin); // get input

		parse(input, args); // parse input

		int i = 0;
		if(args[0] != NULL)
		{
			if(strcmp(args[0], "exit") == 0)
				exit(0);
			execute(args);
		}
	}
}

int main(int argc, char* argv[], char* envp[])
{	
	shell();
}

void parse(char* input, char** args)
{
	while(*input != '\0' )
	{
		while (is_space(*input) || is_redirection(*input) || is_bg_pipe(*input))
		{
			switch(*input)
			{
				case '<':
					*args++ = "<\0";
					break;
				case '>':
					*args++ = ">\0";
					break;
				case '|':
					*args++ = "|\0";
					break;
				case '&':
					*args++ = "&\0";
					break;
			}
			*input++ = '\0';
		}
		*args = input;
		while (!is_space(*input) && *input != '\0' && !is_redirection(*input) && !is_bg_pipe(*input)) 
			input++;
		if(strlen(input) == 0)
			*args = NULL;
		args++;
	}
	*args = NULL;
}



int is_space(char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\n';
}



int is_redirection(char ch)
{
	return ch == '<' || ch == '>';
}



int is_bg_pipe(char ch)
{
	return ch == '&' || ch == '|';
}



void handle_input_redirection(int fdin, char* infile)
{
	if ((fdin = open(infile, O_RDONLY)) < 0)
	{
		perror(infile);			
		exit(1);
	}
	dup2(fdin, 0);
	close(fdin);
}



void handle_output_redirection(int fdout, char* outfile)
{
	if ((fdout = open(outfile, O_WRONLY| O_CREAT, S_IRUSR| S_IRGRP| S_IROTH| S_IWUSR)) < 0)
	{
		perror(outfile);			
		exit(1);
	}
	dup2(fdout, 1);
	close(fdout);
}



void runpipe(int fdpipe[], char** p_args, char** ch_args)
{
	pid_t pid;
	switch (pid = fork()) 
	{
		case -1: 	// error
			perror("fork");
			exit(1);
		case 0:		// child
			close(fdpipe[WRPIPE]); // close WRITE PIPE
			dup2(fdpipe[RDPIPE], 0); // READ FROM INPUT
			close(fdpipe[RDPIPE]);
			if (execvp(*ch_args, ch_args) < 0)
			{
				perror(*ch_args);
				exit(1);
			}
		default:	// parent
			close(fdpipe[RDPIPE]); // close READ PIPE
			dup2(fdpipe[WRPIPE], 1);
			close(fdpipe[WRPIPE]);
			if (execvp(*p_args, p_args) < 0)
			{
				perror(*p_args);
				exit(1);
			}
	}
}





void execute(char** args)
{	
	pid_t pid;
	int status, i = 0, fdin, fdout, fdpipe[2];
	int is_in = 0, is_out = 0, is_pipe = 0, bg = 0;
	char infile[64], outfile[64], **pipeptr;
	char** cmd_ptr[64] = {NULL};
	int cmd_cnt = 0;
	

	while (args[i] != NULL && !is_pipe)
	{
		if (strcmp(args[i], "<") == 0)
		{
			args[i++] = NULL;
			strcpy(infile, args[i]);
			is_in = 1;
		}
		else if (strcmp(args[i], ">") == 0)
		{
			args[i++] = NULL;
			strcpy(outfile, args[i]);
			is_out = 1;
		}
		else if (strcmp(args[i], "|") == 0)
		{
			int ppfd[2];
			pipe(ppfd);
			args[i++] = NULL;
			cmd_ptr[cmd_cnt++] = &args[i];
			pipeptr = &args[i];
			is_pipe = 1;
		}
		else if (strcmp(args[i], "&") == 0)
		{
			printf("background\n");
			fflush(stdout);
			args[i++] = NULL;
			bg = 1;
		}
		else
			i++;
	}
	cmd_ptr[cmd_cnt] = NULL;
	
	if (is_pipe)
	{
		if (pipe(fdpipe) == -1)
		{
			perror("pipe");
			exit(1);
		}
	}

	switch(pid = fork())
	{
		case -1:		// error
			perror("fork");
			exit(1);
		case 0:			// child
			if (bg)
				setpgid(0, 0);

			if (is_in)
				handle_input_redirection(fdin, infile);
			
			if (is_out)
				handle_output_redirection(fdout, outfile);
			
			if (is_pipe)
			{
				//int j = 0;
				//while(cmd_ptr[j] != NULL)
				//{
				//	printf("%s ", cmd_ptr[j++][0]);
				//	fflush(stdout);
				//	runpipe(fdpipe, args, cmd_ptr[j++]);
				//}
				runpipe(fdpipe, args, pipeptr);
			}
			else
			{
				if (execvp(args[0], args) < 0)
				{
					perror(args[0]);
					exit(1);
				}
			}
		default:		// parent
			if (bg) 
				exit(0);
			else
				while (wait(&status) != pid)
					continue;
			break;
	}
}
