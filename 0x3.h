#define max_arguments 512
#define max_line_length 2048
#define sucess 0
#define failure 1


typedef struct { /* struct to store the structure of any command */
	char* command_name; /* unparsed command */
	char* arguments[max_arguments]; /* parse command in which arguments[0] will be the command and the rest are arguments */
	int num_args; /* number of arguments */
	char* outputFile; /* string to store outfile file */
	char* inputFile; /* string to store input file */
	int isBackground; /* bool to check if is background process  */
}command;


 /* function to get and process user input */
char* process_user_input();

 /* sends SIGINT signal to parent process upon CTRL-C, parent and children ignore sigint  */
void catchSIGINT(int signo);
	
/* catches CTRL-Z and prints message to user  */
void catchSIGTSTP(int signo);

/* when exit is called, cleanup kills processes that smallsh started */
void exit_cleanup(); 

/* function to change current directory, parameter is argument array, args[1] is directory to go to, if args[1] is NULL, then go to HOME  */
void shell_cd(char* args[]);

/* function to check status of last foreground command */
void shell_status(int exitstatus);

/* function to execute commands other than built in. takes command struct, and SIGINT, SIGTSTP sigaction structs as arguments  */
int execute_other(command* cmd, struct sigaction act, struct sigaction act2);

/* function to tokenize input line into a command struct, takes in the inpuut line from user as input  */
command* construct_command(char* line);
