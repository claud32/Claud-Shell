#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define PIPE_MAX 4
#define TOKEN_MAX 32
#define ARG_MAX 16
#define FD_NUM 2
#define TRUE 1
struct command{
        char cmd[TOKEN_MAX];
        char *args[ARG_MAX];
};

/* helper */

 static void free_array(int size, char* array[]){
     for(int i = 0; i < size; i++){
         free(array[i]);
     }
 }

static void malloc_array(int size, char* array[]){
        for(int i = 0; i < size; i++){
            array[i] = malloc(TOKEN_MAX);
        }
}

static void wait_status(int status, char* message){
        waitpid(-1, &status, 0);
        WEXITSTATUS(status);
        fprintf(stderr, "+ completed '%s' [%d]\n",
                        message, WEXITSTATUS(status));
}


static int parse_line(char* line, char* delim, char* store[]){
        char* token = strtok(line, delim);
        int index= 0;
        while (token != NULL) {
                strcpy(store[index], token);
                token = strtok(NULL,delim);
                index++;
        }
        return index;
}

static int count_arg(char* line, char* delim){
    int argnum = 0;
    char* token = strtok(line, delim);
    while (token!= NULL) {
            argnum++;
            token = strtok(NULL, delim);
    }
    return argnum;
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        while (TRUE) {
                char *nl;
                /* Print prompt */
                printf("claudShell@claud.pro$ ");
                fflush(stdout);
                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);
                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }
                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';
                /* Create multiple copy for strtok manipulation */
                char* cmdOutDir = malloc(CMDLINE_MAX);
                char* cmdFirst = malloc(CMDLINE_MAX);
                char* cmdPipe = malloc(CMDLINE_MAX);
                char* cmdPipeIn = malloc(CMDLINE_MAX);
                strcpy(cmdOutDir, cmd);
                strcpy(cmdFirst, cmd);
                strcpy(cmdPipe, cmd);
                strcpy(cmdPipeIn, cmd);
                /* Pipeline */
                char* pipeptr = strchr(cmdPipe, '|');
                int cmdnum = 0;
                int rindex = 100;
                int pindex = 0;
                int end = 0;
                if (pipeptr != NULL) {
                        int pipenum = 0;
                        /* Detect missing command */
                        for (unsigned int i = 0; i < strlen(cmd); i++) {
                                if (cmd[i] == '|') {  
                                        pindex = i;
                                        pipenum++;
                                }
                                if (cmd[i] == '>')
                                        rindex = i;
                                /* Check mislocation */
                                if (rindex < pindex) {
                                        fprintf(stderr, "Error: mislocated output redirection\n");
                                        end = 1;
                                        break;
                                }
                        }
                        if (end == 1)
                                continue;
                        /* Calculate the number of pipeline process */
                        cmdnum = count_arg(cmdPipe, "|");
                        if (cmdnum <= pipenum) {
                                fprintf(stderr, "Error: missing command\n");
                                continue;
                        }
                        char* cmdargs[cmdnum];
                        int check[cmdnum-1];
                        char* cmdin = strtok(cmdPipeIn, "|");
                        int index = 0;
                        /* Store processes into an array */
                        while (cmdin != NULL) {
                                if (cmdin[0] == '&') {
                                        cmdargs[index]= cmdin+1;
                                        check[index-1] = 1;
                                } else {
                                        cmdargs[index] = cmdin;
                                }
                                index++;
                                cmdin = strtok(NULL, "|");   
                        }
                        int k = 0;
                        struct command pipe_command[cmdnum];
                        char* pcmd;
                        char* plist[ARG_MAX];
                        malloc_array(ARG_MAX,plist);
                        /* Parse command into struct */
                        while (k < cmdnum) {
                                /* Parse pipeline command */
                                char* pProgramName = malloc(TOKEN_MAX);
                                strcpy(pProgramName, cmdargs[k]);
                                int pargnum = 0;
                                pargnum = parse_line(pProgramName, " ", plist);
                                pcmd = plist[0];
                                for (int i = 0; i < pargnum; i++) {
                                        pipe_command[k].args[i] = (char*)malloc(sizeof(char) * TOKEN_MAX);
                                        strcpy(pipe_command[k].args[i], plist[i]);
                                }
                                pipe_command[k].args[pargnum] = (char*)malloc(sizeof(char) * TOKEN_MAX);
                                pipe_command[k].args[pargnum] = NULL;
                                strcpy(pipe_command[k].cmd, pcmd);
                                k++;
                                free(pProgramName);  
                        }
                        free_array(ARG_MAX, plist);
                       /* Create multiple pipe */
                        int fd[k-1][FD_NUM];
                        for (int i = 0; i < k-1; i++) {
                                pipe(fd[i]);     
                        }
                        int pid;
                        /* Create multiple fork child processes with the same parent */
                        for (int i = 0; i < k; i++) {
                                pid = fork();
                                /* Child */
                                if (pid == 0) {
                                        if (i > 0 && i != k-1) {
                                               if (check[i] == TRUE) {
                                                        close(fd[i][0]);
                                                        dup2(fd[i][1], STDOUT_FILENO);
                                                        dup2(fd[i][1], STDERR_FILENO);
                                                        close(fd[i][1]);
                                                        close(fd[i-1][1]);
                                                        dup2(fd[i-1][0], STDIN_FILENO);
                                                        close(fd[i-1][0]);
                                                        execvp(pipe_command[i].cmd,  pipe_command[i].args);
                                                        exit(1);
                                               } else {
                                                        close(fd[i][0]);
                                                        dup2(fd[i][1], STDOUT_FILENO);
                                                        close(fd[i][1]);
                                                        close(fd[i-1][1]);
                                                        dup2(fd[i-1][0], STDIN_FILENO);
                                                        close(fd[i-1][0]);
                                                        execvp(pipe_command[i].cmd,  pipe_command[i].args);
                                                        free(pipe_command[i].args);
                                                        exit(1);
                                               }
                                        } else if (i == 0) 
                                        {                           
                                                    /* First process */
                                                 if (check[i] == 1) {
                                                        close(fd[i][0]);
                                                        dup2(fd[i][1], STDOUT_FILENO);
                                                        dup2(fd[i][1], STDERR_FILENO);
                                                        close(fd[i][1]);  
                                                } else {
                                                        close(fd[i][0]);
                                                        dup2(fd[i][1], STDOUT_FILENO);
                                                        close(fd[i][1]);
                                                }
                                                execvp(pipe_command[i].cmd,  pipe_command[i].args);
                                                free(pipe_command[i].args);
                                                exit(1);
                                        } else {                                           /* Last process */
                                                close(fd[i-1][1]);
                                                dup2(fd[i-1][0], STDIN_FILENO);
                                                close(fd[i-1][0]);
                                                execvp(pipe_command[i].cmd,  pipe_command[i].args);
                                                free(pipe_command[i].args);
                                                exit(1);
                                        }
                                } else {
                                        /* Parent */
                                        if (i > 0) { 
                                                close(fd[i-1][0]);
                                                close(fd[i-1][1]);     
                                        }        
                                }        
                        }
                        int status;
                        int sid[k];
                        /* Obtain status from each child */
                        for (int i = 0; i < k; i ++) {
                                waitpid(-1, &status,0);
                                WEXITSTATUS(status);
                                sid[i] = WEXITSTATUS(status);  
                        }
                        fprintf(stderr, "+ completed '%s' ",
                                cmd);
                        for (int i = 0; i < k; i ++) {
                                if (i == k-1) {
                                        fprintf(stderr, "[%d]\n", sid[i]);
                                } else {
                                        fprintf(stderr, "[%d]", sid[i]);
                                }
                        }
                        free(cmdPipeIn);
                        free(cmdPipe);
                        continue;
                }
                /* Check output redirection */
                char* file;
                int ifree = TRUE;
                file = strchr(cmdOutDir, '>');
                if (file!= NULL){
                        cmdFirst = strtok(cmdOutDir, ">");
                        ifree = 0;
                }     
                char* cmdProgramName = malloc(TOKEN_MAX);
                char* list[ARG_MAX];
                malloc_array(ARG_MAX, list);
                int argnum = parse_line(cmdFirst, " ", list);
                strcpy(cmdProgramName, list[0]);
                char* args[argnum+1];
                for (int i = 0; i < argnum; i++) {
                    args[i] = list[i];
                }
                args[argnum] = NULL;
                /* Builtin command */
                if (!strcmp(cmdProgramName, "exit")) {
                        if (argnum > 1) {
                                fprintf(stderr, "Error: too many process arguments\n");
                                continue;
                        }
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmdProgramName, 0);
                        break;
                }
                if (!strcmp(cmdProgramName, "pwd")) {
                        if (argnum > 1) {
                                fprintf(stderr, "Error: too many process arguments\n");
                                continue;
                        }
                        char* buffer = malloc(CMDLINE_MAX);
                        int exit;
                        buffer = getcwd(buffer, -1);
                        if (buffer != NULL) {
                                fprintf(stdout,"%s\n", buffer);
                                exit = 0;
                        } else {
                                perror("Error: ");
                                exit = 1;
                        }
                        free(buffer);
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, exit);
                        continue;
                        }
                if (!strcmp(cmdProgramName, "cd")) {
                        if (argnum > 2) {
                        fprintf(stderr, "Error: too many process arguments\n");
                        continue;
                        }            
                        int exit = chdir(args[1]);
                        if(exit == -1){
                            fprintf(stderr,"Error: cannot cd into directory\n");
                        }
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmdOutDir, exit*-1);
                        continue;
                        }              
                /* ls-like builtin command */
                if (!strcmp(cmdProgramName, "sls")) {
                       DIR *dp;
                       struct dirent *ep;
                       struct stat st;
                       dp = opendir("./");
                       if (dp != NULL ){
                               while((ep = readdir(dp))){
                                   if(!strcmp(ep->d_name, "..") || !strcmp(ep->d_name, "."))
                                            continue;
                                       stat(ep->d_name, &st);
                                       printf("%s (%ld bytes)\n", ep->d_name, st.st_size);
                               }
                                closedir (dp);
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmdProgramName, 0);
                       } else {
                               perror ("Error: ");
                       }
                       continue;
                }
                /* Regular command */
                int fd;
                if (!fork()) {
                         /* Child */
                         /* Check output redirection */
                        if (file != NULL) {
                                if (file[1] == '&') {
                                        file +=2;
                                        char* filename = strtok(file, " ");
                                        if( filename == NULL){
                                                fprintf(stderr, "Error: no output file\n");
                                                continue;
                                        }
                                        cmdProgramName = strtok(cmdOutDir, ">");
                                        fd = open(filename, O_WRONLY | O_CREAT, 0777);
                                        if (errno == 1) {
                                               fprintf(stderr, "Error: cannot open output file\n");
                                               continue;
                                       }
                                        dup2(fd, STDOUT_FILENO);
                                        dup2(fd, STDERR_FILENO);
                                        close(fd);   
                                } else {   
                                        ++file;
                                        char* filename = strtok(file, " ");
                                        cmdProgramName = strtok(cmdOutDir, ">");
                                        char* ch = strchr(cmdProgramName,' ');
                                        ch++;
                                        if (ch == filename || ch == NULL) {
                                                fprintf(stderr, "Error: missing command\n");
                                                continue;
                                        }
                                        if (filename == NULL) {
                                                fprintf(stderr, "Error: no output file\n");
                                                continue;
                                        }
                                        fd = open (filename, O_WRONLY | O_CREAT, 0777);
                                       if (errno == 1) {
                                               fprintf(stderr, "Error: cannot open output file\n");
                                               continue;
                                       }
                                        dup2(fd, STDOUT_FILENO);
                                        close(fd);
                                }
                        }
                        execvp(cmdProgramName, args);
                        fprintf(stderr, "Error: command not found\n");
                        exit(1);
                } else {
                    /* Parent */
                    int status = 0;
                    wait_status(status, cmd);
                }
                free(cmdProgramName);
                free(cmdOutDir);
                free_array(ARG_MAX, list);
                if(ifree == TRUE){
                    free(cmdFirst);
                }
        }
        return EXIT_SUCCESS;
}
