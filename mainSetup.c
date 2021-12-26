#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */



// when a foreground child receives SIGTSTP it will set this flag value to 0 
volatile sig_atomic_t tstop_flag = 1;
// ^Z signal handler for the foreground child
void fgTSTOPHandler(int signal_number){
  tstop_flag = 0;
}


// Some processes should ignore the SIGTSTP (main and the background processes)
// in my case it only prints new-line (like linux's original terminal)
void ignoreTSTOPHandler(int signal_number){
//  write(STDOUT_FILENO,"\n",1);
}

// I consider aliases as structs 
typedef struct alias{
  char *original;
  char *replacement;
}alias;

int main(void)
{
      char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
      int background; /* equals 1 if a command is followed by '&' */
      char *args[MAX_LINE/2 + 1]; /*command line arguments */
      char *path= getenv("PATH"); // get the PATH variable 
      char *paths[50];            // paths will hold all the PATH entries 
      int pathAmount=0;           // amount of path entries
      alias aliases[MAX_LINE/2+1]; // User's aliases will sored here      
      int aliasCount = 0;          // amount of aliases user currently have  


      // start tokenising the PATH variable with the delimiter of ':'
      paths[pathAmount] = strtok(path,":");
      while(paths[pathAmount] != NULL){
        pathAmount++;
        paths[pathAmount] = strtok(NULL,":");
      }
      // binPath will hold all the PATH entries on the heap
      // so we can do some string operations on it.
      char **binPath;
      binPath = malloc(pathAmount * sizeof(char*));
      for(int i = 0 ; i< pathAmount ; i++){
        // ... * sizeof(char) is literally equals to multiplying with 1, but for clean code reasons i will do it like this
        binPath[i] = malloc((MAX_LINE/2 + 1) * sizeof(char));
      }
      
      signal(SIGTSTP, ignoreTSTOPHandler);

      while (1){
        background = 0;
        tstop_flag = 1;
        printf("myshell: ");
        fflush(stdout);
        /*setup() calls exit() when Control-D is entered */

        setup(inputBuffer, args, &background);
      
        // check if input is empty, (e.g user is pressing 'return' without entering a command)
        if(args[0] == NULL){
          continue;
        }
        
        if(background){
          // remove ampersand(&) from the args list 
          // it causes errors because it literally gives it as a parameter
          // to the program that we want to execute
          for(int i =0; i<(sizeof(args)/sizeof(args[0]));i++){
              if(strcmp(args[i],"&")== 0) {
                  args[i]= NULL;
                  break;
              }
          }
 
        }
        // before creating any process, check wether if the first argument is exit or not
        // if so, then check if there are any running process 
        // (there is no need for explicitly checking for background process 
        // since user cannot enter any command if there is a foreground process is running)
        // if there is then alert the user, otherwise terminate the ingoing shell
        if(strcmp(args[0],"exit")==0){
          if(waitpid(-1,NULL,WNOHANG) == 0){
            fprintf(stderr, "There are process(es) running at background, cannot exit\n");
          }else{
            kill(0,SIGKILL);
          }
        }else if(strcmp(args[0],"alias")==0){
          if(strcmp(args[1],"-l")==0){
            for(int i=0; i<aliasCount;i++){
              fprintf(stderr,"\t%s %s\n",aliases[i].replacement,aliases[i].original);
            }
          }else{
            // construct a string from args variable 
            alias tempAlias;
            tempAlias.original = malloc(strlen(args[1])) ;
            tempAlias.replacement= malloc(strlen(args[2])) ;

            strcpy(tempAlias.original,args[1]);
            strcpy(tempAlias.replacement,args[2]);
            aliases[aliasCount] = tempAlias;
            aliasCount++;
          }
            continue;
        }else if(strcmp(args[0],"unalias")==0){
          // just decrement the aliasCount so listing will be done accordingly 
          aliasCount--;
        }
        // Check aliases first before creating a child 
        // run aliases with system() function, that is allowed 
        for(int i = 0 ; i<aliasCount;i++){
          if(strcmp(args[0],aliases[i].replacement)== 0){
            // alias found
            if(system(aliases[i].original)== -1){
              perror(NULL);
            }else{
              // alias executed successfully
              continue;
            }
          }
        }
        // Some string operations here, since execv's first parameter requires the binary name appended to the path 
        for(int i = 0; i<pathAmount;i++){
          char *elemHolder;
          elemHolder = malloc((MAX_LINE/2 +1)*sizeof(char*));
          // create just enough space for the each path + arg
          snprintf(elemHolder,(MAX_LINE/2) +1,"%s/%s",paths[i],args[0]);
          strcpy(binPath[i],elemHolder);
          free(elemHolder);
          }

        // get parent's pid for comparing purposes
        pid_t parentPID = getpid();

        // create a child
        pid_t childPID =0;
        int wstatus;   // wait status of the child process
        childPID = fork();
        // check for forking error
        if(childPID == -1){
          // since fork() sets global errno I will use perror() to get system defined error message on the stderr
          perror("Error while forking");
        }
        if(childPID == 0){
          // we are in the child process

          // handle signals only when bacground == 0
          // handle the SIGTSTP signal in child 
          // only stop the foreground processes
          struct sigaction sigtstop_action;
          memset(&sigtstop_action,0,sizeof(sigtstop_action));
          sigtstop_action.sa_handler = &fgTSTOPHandler;
          // register the signal handler also set the mask 
          // if they fail they will set the global errno 
          // I will use perror(NULL) to show corresponding message
          if((sigemptyset(&sigtstop_action.sa_mask) == -1) ||
              (sigaction(SIGTSTP,&sigtstop_action,NULL) == -1)){
            // give system error message
              perror(NULL);
              // exit out of the system
              exit(EXIT_FAILURE);
          }


          if(background == 0){
            while(tstop_flag){
              for(int i =0 ; i<pathAmount; i++){
                execv(binPath[i],args);
              }
              // if the command has not found in the PATH, or if there is some other error happened then print to stderr
              perror("Error while executing"); 
              break;
            }
          }else{
              for(int i =0 ; i<pathAmount; i++){
                execv(binPath[i],args);
              }
              // if the command has not found in the PATH, or if there is some other error happened then print to stderr
              perror("Error while executing"); 
          }
          
        }
        if(getpid() == parentPID){
          // we are in the parent process

          // if the process is set to run in the background
          if(background){
            // do not wait for the child process to finish 
            // but for notifying purposes, make another process to wait for the child's process to finis
            // so I can let the user know that background process has finished
            // notify the user that the background process has created with is pid
            fprintf(stderr,"Process %d has been backgrounded\n",(int)childPID);
            
            // I wish to let the user know that the child process has finished (like the linux terminal does), I've tried numereous things with new child processes but none of them has worked
            // go to the beginning of the loop
            continue;
          }else{
            // wait for the process since its running on the foreground
            
            // if there is no error comes from the child process check for the wstatus code
            // Also check if the process has been stopped (WUNTRACED)
            if(waitpid(childPID,&wstatus,WUNTRACED) != -1){
              if(WIFEXITED(wstatus)){
                // since waitpid does not set the global errno, I will use fpritf to write to the stderr
                if(WEXITSTATUS(wstatus) != 0){ // successfull executions return 0, we don't need to notify the user if the process is successfull (its expected)
                fprintf(stderr,"The process %d has returned with the exit code: %d\n",(int)childPID,WEXITSTATUS(wstatus));
                }
              }
                // check if the process has terminated by a signal and notify the user.
              else if(WIFSIGNALED(wstatus)){
                fprintf(stderr,"The process %d has terminated by the signal: %d\n",(int)childPID,WTERMSIG(wstatus));
              }
                // check if the process has stopped by a signal 
              }else if(WIFSTOPPED(wstatus)){
                fprintf(stderr,"The process %d has stopped bt the signal: %d\n",(int)childPID,WSTOPSIG(wstatus));
              }
            }
        }

          /** the steps are:
          (1) fork a child process using fork()
          (2) the child process will invoke execv()
          (3) if background == 0, the parent will wait,
          otherwise it will invoke the setup() function again. */
      }
      // free the binPath
      free(binPath);
}
