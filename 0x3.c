#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "0x3.h"
#define fg_pid getpid()
char* result = NULL;
static char* buffer = NULL;
int children_pids[10];
int total_child_pids = 0;
int foreground_only_mode = 0;


/* custom handler for catching CTRL-C */
void catchSIGINT(int signo){

	char* message = "\n Terminated by signal 2\n";	
	write(STDOUT_FILENO, message, 25);

		
}


/* custom handler for catching CTRL-Z */
void catchSIGTSTP(int signo){
	

	/* if not in foreground only mode, upon cathing this signal, switch to foreground only mode  */
	if(foreground_only_mode == 0){
		char* message1 = "\nEntering foreground-only mode (& is now ignored)\n:  ";
		write(STDOUT_FILENO, message1, 53);
		foreground_only_mode = 1;
	

	/* if already in foreground only mode, upon catching this signal , switch to non foreground only mode */
} else if(foreground_only_mode == 1){
		char* message2 = "\nExiting foreground-only mode\n:  ";
		write(STDOUT_FILENO, message2, 33);
		foreground_only_mode = 0;
	}

}

/* builtin - changes directory */
void shell_cd(char* args[]){
	//printf("Reached shell_cd()\n");	


	/* if cd with no args, then go to HOME  */
	if(args[1] == NULL){
		char* HOME = getenv("HOME");
		if(chdir(HOME) != 0){
			perror("smallsh:");
			fflush(stdout);
		}		
	}else if(chdir(args[1]) != 0){
		perror("smallsh: chdir()");
		fflush(stdout);
	}
	
	
}

/* prints exit staus of last foreground command  */
void shell_status(int exitstatus){

	//check if status of last command is signal or exit code
	if(WIFEXITED(exitstatus) != 0){
		exitstatus = WEXITSTATUS(exitstatus);
	 	printf("exit status: %d\n", exitstatus);
		fflush(stdout);
	}else{
		int termsignal = WTERMSIG(exitstatus);
		printf("terminated by signal: %d\n", exitstatus);
		fflush(stdout);
	}		

	return;
}

/* kills bg processes and frees buffer  */
void exit_cleanup(){

	//printf("reached exit()\n");	
	free(buffer);
	free(result);
	if(total_child_pids  > 0){
		int i;
		for(i = 0; i < total_child_pids; i++){
			kill(children_pids[i], SIGKILL);
		}
	}
		
	exit(sucess);
}


/* executes command other than three builtin  */
int execute_other(command* cmd, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action){
	//printf("Reahced execute_other()\n");		

	pid_t child_pid = -1;
	int exitstatus;
	int stdINFD, stdOUTFD, result;	


	/* if foreground only mode = true, then set current comamand to foreground if initialyl background  */
	if(foreground_only_mode == 1){
		cmd->isBackground = 0;
	}

	
	/* sigint is ignored in foreground  */ 	
	if(cmd->isBackground == 0 || (foreground_only_mode == 1)){

		SIGINT_action.sa_handler = catchSIGINT;
		sigfillset(&SIGINT_action.sa_mask);
		SIGINT_action.sa_flags = 0;
		sigaction(SIGINT, &SIGINT_action, NULL);	
	}



	child_pid = fork();

	switch(child_pid){
	case 0:


	SIGTSTP_action.sa_handler = SIG_IGN;	
	sigfillset(&SIGINT_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);	


		/* if user doesn't redirect standard input for background command, redirect to /dev/null */
		if(!cmd->inputFile && (cmd->isBackground == 1)){
			stdINFD = open("/dev/null", O_RDONLY);
			if(stdINFD == -1){
				perror("smallsh: open() - stdINFD (bg)");
				fflush(stdout);
			}else{
				if(dup2(stdINFD, 0) == -1){
					perror("smallsh: dup2() - stdINFD (bg)");
					fflush(stdout);
				}	
			}		
		}

	
		/* if user doesn't redirect standard output for background command, redirect to /dev/null/  */
		if(!cmd->outputFile && (cmd->isBackground == 1)){
			stdOUTFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if(stdOUTFD == -1){
				perror("smallsh: open() - stdOUTFD (bg)");
				fflush(stdout);
			}else{
				if(dup2(stdOUTFD, 1) == -1){
					perror("smallsh: dup2() - stdOUTFD (bg)");
					fflush(stdout);		
				}
			}
		}
	
		/* if inputfile exists then redirect to inputFIle  */
		if(cmd->inputFile){

			stdINFD = open(cmd->inputFile, O_RDONLY);
			if(stdINFD == -1){
				perror("smallsh: open() - stdINFD");
				fflush(stdout);
				exit(1);
			}
				
			result = dup2(stdINFD, 0);
			if(result == -1){
				 perror("smallsh: dup2() - stdINFD");
				 fflush(stdout);
				exit(1);
			}


			close(stdINFD);
		} 
		
		/* if output file exists, then redirect output to outputFile  */
		if(cmd->outputFile){



		
			stdOUTFD = open(cmd->outputFile, O_WRONLY | O_CREAT | O_TRUNC , 0644);

			if(stdOUTFD == -1){
				perror("smallsh: open() - stdOUTFD");
				fflush(stdout);
				exit(1);
			}	
				
			result = dup2(stdOUTFD, 1);
			if(result == -1){
				perror("smallsh: dup2() - stdOUTFD ");
				fflush(stdout);
				exit(1);
			}

			close(stdOUTFD);					
		}
	

		//printf("about to execute command\n\n");
		if(execvp(cmd->command_name, cmd->arguments) < 0){
			perror("exec failure");
			fflush(stdout);
			exit(1);
		}
		break;		
	default:

		//printf("in the parent \n\n");
	
		if(cmd->isBackground == 1){		

			printf("Background pid is %d\n", child_pid);
			fflush(stdout);
			children_pids[total_child_pids++] = child_pid;
			break;
		}else{
			//printf("waiting for child, waitpid is next line\n\n");
			 waitpid(child_pid, &exitstatus, 0);	
		}
		break;
	}


	//printf("Returning exitstatus:(%d) \n", exitstatus);
	return exitstatus;
}

/* tokenize input into command struct */
command* construct_command(char* input){
	//printf("Reached contsruct_command()\n");

	
	char* delimeters = " \n";
	char* token;
	char* saveptr;
	int argc = 0;
		
	command* new_cmd = calloc(sizeof(command)+ max_arguments * sizeof(char* ), 1);
	
	token = strtok_r(input, delimeters, &saveptr);

	while(token && (argc < max_arguments)){
			
		/* check for < or > input or output file */
		if((strcmp(token, "<") == 0)){

			token = strtok_r(NULL, delimeters, &saveptr);
			new_cmd->inputFile = token;

		}else if(strcmp(token, ">") == 0){
	
			token = strtok_r(NULL, delimeters, &saveptr);
			new_cmd->outputFile = token;
			
		/* if ampersand , assume its the end and treat as background */
		}else{

		/* check for double $ and deal with expansion if needed  */
		result = calloc(1, 1000 * sizeof(char));
		int i, len;
		int pid = getpid();
		char string_pid[20];
		sprintf(string_pid, "%d", pid);
		len = strlen(token);
		for(i = 0; i < len;i++){
			if((i + 1 != len) && ((token[i] == '$') && (token[i + 1] == '$'))){
				
				strcat(result,string_pid);
				i++;
			}else{
				
				strncat(result, &token[i], 1);

			}
		}					
			
		
			new_cmd->arguments[argc++] = result;
		}
		token = strtok_r(NULL, delimeters, &saveptr);				
	}
	

		
	if(strcmp(new_cmd->arguments[argc - 1], "&") == 0){
		new_cmd->isBackground = 1;
		argc--;
		//printf("argc= %d\n", argc);	
		new_cmd->arguments[argc] = NULL;
	}			


	new_cmd->command_name = new_cmd->arguments[0];
	new_cmd->num_args = argc;
	return new_cmd;

}

/* function to ask user for input and store into 'buffer'  */
char* process_user_input(){
	
	buffer = malloc(max_line_length * sizeof(char));

	fgets(buffer, max_line_length, stdin);

	int charsRead = (int)strlen(buffer);
	
	if(charsRead == 1 | charsRead == 0 | charsRead == -1){
		free(buffer);
		return NULL;
	}

	//printf("buffer = %s\n", buffer);
	return buffer;	

}

int main(){

	
	/* delcare and set default handlers on signals  */
	struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};
	
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;

	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;

	
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	sigaction(SIGINT, &SIGINT_action, NULL);




	while(1){


	int exitstatus;
	//printf("resetting the sigaction structs\n");;

	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;

	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;

	
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	sigaction(SIGINT, &SIGINT_action, NULL);


	
		/* before asking for user input, check for bg processes and print whether or not they have finished  */
		if(total_child_pids > 0){
			int i, status;
			for(i = 0; i < total_child_pids; i++){
				status = waitpid(children_pids[i],&exitstatus, WNOHANG);
		
				if(status > 0){	
					if(WIFSIGNALED(exitstatus) != 0){
						printf("background pid %d is done: terminated by signal %d\n", status,exitstatus);
						children_pids[i] = -1;
						total_child_pids--;
					}else{
						printf("background pid %d is done: exit value %d\n", status, exitstatus);
						children_pids[i] = -1;
						total_child_pids--;
				}
			}
			
			}
		
		}

		printf(":  ");
		fflush(stdout);	


		/* take is user input and process into buffer */
		char* in =  process_user_input();
				

		if(in == NULL){
			continue;
		}	

		/* tokenize string into command struct */
		command* user_cmd = construct_command(in);
		
		
		/* check if first arg is comment character  */
		if(strcmp(user_cmd->arguments[0], "#") == 0 || (user_cmd->command_name[0] == '#')){
			continue;
		}
		
		/* check for built in commands - 'cd', 'exit', 'status' */

		if(strcmp(user_cmd->command_name, "cd") == 0){
			shell_cd(user_cmd->arguments);
			continue;
		}

		if(strcmp(user_cmd->command_name, "exit") == 0){
			exit_cleanup();
		}

		if(strcmp(user_cmd->command_name, "status") == 0){
			shell_status(exitstatus);
			continue;
		}
	
		

		/* set exitstatus to status of last command  */
		exitstatus = execute_other(user_cmd, SIGINT_action, SIGTSTP_action);
	
		
					
	}

	return 0;
}
