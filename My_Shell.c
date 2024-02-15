#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#define MAX_COMMAND_SIZE 1024


void redirect(const char *file_name, int mode)
{
	int in_fd;
    int out_fd;
	if(mode==0){
        //overwrite
    	out_fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT, 0666);
	}else if(mode==1){
        //append
    	out_fd = open(file_name, O_RDWR | O_APPEND | O_CREAT, 0666);
	}else if(mode==2){
        //redirect stdin to file
	    in_fd = open(file_name, O_RDONLY);
        dup2(in_fd, 0);
        close(in_fd);
        //overwrite
        out_fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT, 0666);
    }
    //redirect stdout to file
    dup2(out_fd, 1); 
    close(out_fd);
}

void execute_command(char *arguments[],int *running)
{
    pid_t pid; 
    if (strcmp(arguments[0], "exit") != 0)
    {
        if ((pid = fork()) < 0) 
        {
            fprintf(stderr, "Fork failed!");
        }
        else if (pid == 0)
        {
            //child calls execvp to execute command
            if (execvp(arguments[0], arguments) < 0)
            {
				fprintf(stderr, "EXECVP failed!");
			}
        }
        else 
        {
            //parent waits for child process to finish
            waitpid(pid, NULL, 0);
        }
        //redirect to terminal
        redirect("/dev/tty", 2);
    }
    else
    {
        //exit command
        *running = 0;
    }
}

void create_pipe(char **arguments, int *running)
{
    int fd[2];
    pipe(fd); 
    //redirect stdout to write end of pipe
    dup2(fd[1], 1); 
    close(fd[1]);
    
    execute_command(arguments, running);

    //redirect stdin to read end of pipe
    dup2(fd[0], 0);
    close(fd[0]);
}

char* add_spaces(char* in_command, int* second_symbol)
{
    int i, end, j = 0;

    char *spaced_command = (char *)malloc((MAX_COMMAND_SIZE * 2) * sizeof(char));

    for (i = 0; i < strlen(in_command); i++)
    {
        //if a special character is not found
        if (in_command[i] != '>' && in_command[i] != '|')
        {
            //add character to array
            spaced_command[j++] = in_command[i];
        }
        else
        {
            //if the first > is found
			if (in_command[i] == '>' && *second_symbol==0)
			{
                //if there is a second one
				if(in_command[i+1] == '>'){
                    //increment flag
					*second_symbol = *second_symbol + 1;
				}
                //add spaces 
                spaced_command[j++] = ' ';
				spaced_command[j++] = in_command[i];
                spaced_command[j++] = ' ';
			}
            //if | is found
		    else if (in_command[i] == '|')
			{
                //add spaces
				spaced_command[j++] = ' ';
				spaced_command[j++] = in_command[i];
				spaced_command[j++] = ' ';
			}
        }
    }

    //add null at the end of the string
    end = strlen(spaced_command) - 1;

    spaced_command[end] = '\0';

    return spaced_command;
}

void analyse_command(char *command, int *running)
{
    int i = 0;
	char *arguments[MAX_COMMAND_SIZE];
	char *spaced_command;
	int second_symbol = 0; //works as a flag
	spaced_command = add_spaces(command, &second_symbol);
    //get first part of command
	char *token = strtok(spaced_command, " ");
	while (token){
        printf("%s \n", token);
        //if > is found call redirect
		if (*token == '>')
		{
			redirect(strtok(NULL, " "), second_symbol); 
		}
        //if | is found create pipe
		else if (*token == '|')
		{
            arguments[i] = NULL;
            //call create pipe for the current arguments
			create_pipe(arguments, running);
            //to overwrite arguments array
			i = 0;
		}
		else
		{
            //add token to arguments array
			arguments[i] = token;
			i++;
		}
        //get next part of the command
		token = strtok(NULL, " ");
	}
    //add null at the end of the arguments string
	arguments[i] = NULL; 
	execute_command(arguments, running);
}

int main(int argc, char *argv[])
{
	char user_command[MAX_COMMAND_SIZE];
	int running = 1;
    while (running)
    {
        printf("My shell: ");
        fflush(stdout);
        //read command
		fgets(user_command, MAX_COMMAND_SIZE, stdin);

		analyse_command(user_command, &running);
	}

    return 0;
}