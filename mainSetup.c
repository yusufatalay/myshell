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

volatile sig_atomic_t tstop_flag = 1;

// the handler function for the sigaction
// this handler will be used by the child
void tstopHandler(int signal_number){
  tstop_flag = 0;
}
// Pressing ^Z should not effect the main proccesses
// it should only stop the forehround child processes
void maintstopHandler(int signal_number){
write(STDOUT_FILENO,"\n",1);
}

void check_bg_process(int* wstatus, int procNo, char* arg,int* backgroundProcessCounter){
while(1){
  if(waitpid(-1,wstatus,WNOHANG) == procNo){
    // if the bg child has terminated normally or just simply returned
    // to imitate the terminal emulator I will print a similar
    // message to notify the user.
    fprintf(stderr,"[%d] + %d done %s\n",*backgroundProcessCounter,procNo,arg);
    //decrement the counter since the child is done 
    backgroundProcessCounter--;                                   
    break;
  }
}
}

int main(void)
{
  char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
  int background; /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/2 + 1]; /*command line arguments */
  char* path = getenv ("PATH");
  char *paths[50]; 
  int pathAmount= 0;
  // to imitate the terminal emulator we need to count how many bg proccesses
  // are present, this value will be incremented by one after each execution
  // which its args contains ampersand, and will be decremented 
  // when bg process finishes.
  int backgroundProccessCounter = 0; 
  paths[pathAmount] = strtok(path, ":");
  // loop through the string to extract all other tokens
  while(paths[pathAmount] != NULL) {
      pathAmount++;
      paths[pathAmount]  = strtok(NULL, ":");
      }
  // TODO: Create a string array (binPath)  with size pathAmount DONE
  char **binPath;
  binPath= malloc(pathAmount * sizeof(char*));
  for (int i = 0; i < pathAmount; i++){
      binPath[i] = malloc((MAX_LINE/2 + 1) * sizeof(char));
      }
  // parent should be irresponsive to the ^Z signal, only child should response to that signal if the child is a foreground process
 // struct sigaction child_sigterm_action;
 // memset(&child_sigterm_action, 0, sizeof(child_sigterm_action));

 // child_sigterm_action.sa_handler = &maintstopHandler;
 //   child_sigterm_action.sa_flags= 0;
 // 
 //   // register the signal handler and send SIGTSTP when encountering with 
 //   if((sigemptyset(&child_sigterm_action.sa_mask) == -1)||
 //       (sigaction(SIGTSTP,&child_sigterm_action,NULL) == -1)){
 //         perror(NULL);
 //         exit(EXIT_FAILURE);
 //       }        
  signal(SIGTSTP,maintstopHandler);

    while (1){
        background = 0;
        
        printf("myshell: ");
        fflush(stdout);
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);
       
        // if backgruond is 1 then increment the backgroundProcessCounter
        if(background){
            backgroundProccessCounter++;
            // also remove ampersand(&) from the args list 
            // it causes errors because it literally gives it as a parameter
            // to the program that we want to execute
            for(int i =0; i<(sizeof(args)/sizeof(args[0]));i++){
                if(strcmp(args[i],"&")== 0) {
                    args[i]= NULL;
                    break;
                }
            }
        }
        // TODO: binPath[i] = paths[i] + '/'+ args[0] DONE
        for(int i = 0; i< pathAmount; i++){
            // I will build the string on this variable and assign it to a corresponding place at binPath
            char *elemHolder;
            elemHolder = malloc((MAX_LINE/2 +1)*sizeof(char*));
            snprintf(elemHolder,(MAX_LINE/2)+1,"%s/%s",paths[i],args[0]);
            strcpy(binPath[i],elemHolder);
            free(elemHolder);
        }
        
        pid_t parentPID = getpid();
        // create a fork()
        pid_t proc = 0;
        int wstatus;    // wait status of the child proccess
        proc = fork();
        // error check
        if(proc == -1){
            // use stderr
            perror("Error happened while forking operation\n");
        }
        if (proc  == 0 ){
            // run child

            // handle the SIGTSTP signal in child 
            // only stop the foreground processes
            struct sigaction sigtstop_action;
            memset(&sigtstop_action,0,sizeof(sigtstop_action));
            sigtstop_action.sa_handler = &tstopHandler;
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
            
            // if args[0] is 'exit' then terminate the whole process
            if(strcmp(args[0],"exit") == 0){
              kill(proc,SIGTERM);
            }
            // while there is no stop signal, run the child
            if(background == 0){
              while(tstop_flag){
               // for every entry in the binPath array 
               // try to run the execv command with given args
               // if the code ever exits out of the loop 
               // then the given argument cannot be found in the PATH
                for(int i =0 ;i < pathAmount; i++){
                  execv(binPath[i],args);
                }

              }
                perror("Command cannot be found in the PATH");
            }else{
              for(int i =0 ;i < pathAmount; i++){
                  execv(binPath[i],args);
              }
               perror("Command cannot be found in the PATH");
            }
            fprintf(stderr,"Proccess %d has stopped\n",(int)proc);
        }
        if(getpid() == parentPID){
            // parent here 
              // if background has not set then wait for the child 
            if(background == 0){
             
     
              int waitedproc = waitpid(proc,&wstatus,0);
                    while(waitedproc != proc);        
                //TODO: child proccess is exited check the reason with the W functions
                

            }else{
                // parent does not waits for its child here (aka the ampersand is not in use)
                // notify the user that the background process has created with is pid
                fprintf(stderr,"[%d] %d\n",backgroundProccessCounter,(int)proc);
                // check for the status of the background childs status
                
                // constantly check if child has exited
                // create a new process to check if background process has finished
                pid_t c_pid = fork();
                if(c_pid == -1){
                  perror(NULL);
                }else if (c_pid == 0) {
                  // check for the child proccess on the background 
                  check_bg_process(&wstatus,proc,args[0],&backgroundProccessCounter);
                }else{
                  setup(inputBuffer, args, &background);
                }
            }


            /** the steps are:
            (1) fork a child process using fork()
            (2) the child process will invoke execv()
            (3) if background == 0, the parent will wait,
            otherwise it will invoke the setup() function again. */
         }
    }
    // free the binPath 
    free(binPath);
}

