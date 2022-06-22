# Claud-Shell
An UNIX shell with custom features
## High-level Design Summary
I implement each of the shell functionaily separately. I parse the command 
line with the strtok function and parse command line differently base on
different input command. I use if statement to detect what the input command
is to decide the way of parsing.

I create a struct called **command** that is used to store the parsed command
line. It contains two members: cmd and args. cmd stores the command and args
stors the command and arguments after command. I design it in this way
because it fits the execvp arguments.

### Process Handling
To replace `system()`, I use `fork()` to create process. I use an if else
statement to separate out child and parent. I use the function `execvp()` to
execute the command. I parse the command line into arguments that fits the
`execvp function`. In parent, I use `waitpid()` to wait for the child
termination and get its status, and I print the completion message to
 `stderr`. In child, I execute command with `execvp()`.

### Parsing Command
I parse the command line in order to differentiate which is command and which
is argument, I use `strtok()` to separate arguments and command, I use
**white space** as delim. I store the arguments in a char array, so that 
it can directly be passed in to the `execvp()` function.

### Build-in Command
To implement build-in command, I first use if statements to detect whether the
input command line is calling one of the core feature built-in command, then
I use `chdir()` to implement cd and `getcwd()` for pwd.

### Output Redirection
For output redirection implementation, I use `strchr()` to check whether the 
command involves `>`. `strchr()` returns a pointer that contains the strings 
following the argument. I accomplish this by detecting whether the return 
pointer is NULL. Then, I get our destination file name from `strchr()` by
incrementing the return pointer. Then, I use `strtok()` again to separate out
the command and file name. I use `dup2()` to connect the file descripter of 
opening the file to `stdout`.

### Pipe Command
For pipe command implementation, I first count the number of processes in the
command line, by doing a for loop and count the number of `|`and then I 
create number of proceses minus one of pipes using atwo-d integer array `fd[][]`.
For the first process, I only connect its stdout to the first pipe. 
For the last process, I only connect its stdin to the last pipe. 
For any processes in the middle, I connect each process with
two pipes, connect the stdin to the previous pipe with stdout with the
previous process, and connect the stdout of the process to the next pipe
with stdin of the next process. I use a while loop to create multiple
children with the same parent.

### Extra Feature
For extra feature implementation, in pipe command or output redireciton
command, I detect whether `&` lies right after `|` or `>`, if the symbol is
found, I parse the command line differently. I also use `strtok()` but use
a different delim argument. This time, I use `|&` or `>&` to separate
command line. I then connect the stderr to the file descriptor just like
what I did to redirect output using `dup2()`. 

### Error Handeling
For Error handeling, I detect parsing error by adding for loops inside of the
command implementation to find out if there are missing command. For pipe
command, I detect missing command by comparing the nummber of `|` and the 
number of process. For library error handeling, I use a couple of `perror()`
under the function to catch error, and I also use `fprintf()` for customize 
error message printing.

## Code testing
I test our code by using printf and compare the output with the reference 
shell Bash for a variety of different command. For Pipe command testing, I first 
test by piping two processes. I print out the file descriptors in order to see 
if I connect everything right. 

## Sources
[strtok implementation](https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm) 

[create multiple child process](https://www.geeksforgeeks.org/creating-multiple-process-using-fork/)
