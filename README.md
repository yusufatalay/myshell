# My Shell
This project implements a simple shell program written in C. It allows users to enter commands, navigate the file system, and execute programs.

## Key Features

* Command Parsing: The shell parses user-entered commands, separating arguments and identifying redirection operators like '<', '>', and '>>'.
* Background Execution: Users can run commands in the background by appending an ampersand (&) to the command line.
* I/O Redirection: The shell can redirect standard input (stdin) and standard output (stdout) to and from files. For example, ls > output.txt redirects the output of ls to a file named output.txt.
* Signal Handling: The shell handles the SIGTSTP (Ctrl+Z) signal differently for foreground and background processes. Foreground processes are suspended upon receiving this signal, while background processes continue execution.

## Code Structure

The code utilizes several functions:

* setup: Reads the user's command line input, parses it into arguments, and identifies background processes.
* locateRedirectOperators: Identifies the locations of redirection operators within the argument list.
* setmin: Determines the position of the first encountered redirection operator.
* childPID: Manages child processes created using fork.
* waitpid: Waits for child processes to finish and retrieves their exit status.

## Getting Started

1. Clone this repository.
2. Compile the source code using a C compiler (e.g., GCC).
3. Run the executable file.

