/********************************************************************
 * Hugh O'Brien 
 * Carroll 
 * CS570
 * Due: Monday March 7th
 * Simple Shell
********************************************************************/

#include "p2.h" 
#define CD 1 
#define LS 2 
#define PRINTENV 3 
#define SETENV 4

/* Global Flags */
int PARSE_ERROR;
int REDIRECT_IN;
int REDIRECT_OUT;
int NO_WAIT;
int PIPE;
int BUILT_IN;
int ESCAPED_$;

int next_proc = 0;

int main () {
    char *newargv[MAXITEM] = {};
    int line_length;
    int status;
    pid_t child_pid;
    int in_fd, out_fd;
    char buf[2048];
    char infile[255];
    char outfile[255];
     
    signal(SIGTERM, MyHandler);

    for(;;) {
        // Zero out flags so nothing is left from previous
        BUILT_IN = REDIRECT_IN  = REDIRECT_OUT = NO_WAIT = 0; 
        PIPE = next_proc = PARSE_ERROR = ESCAPED_$ = 0;
        newargv[0] = (char *) NULL;
        printf("p2: ");
        

        // Flush stdout buffer to make sure no leftover words are output
        // on next command.
        fflush(stderr);
        fflush(stdout);    
        line_length = Parse(buf, newargv, infile, outfile); 
        
        // EOF reached
        if (line_length == -1) break;
    
        // Empty Line or parsing error
        if (line_length == 0 || PARSE_ERROR) continue;
            
        
        // Checking if built-in command was input
        if (BUILT_IN != 0) {
            // If redirecting output, create file only if it does not exist. 
            switch (BUILT_IN) {
                case (CD):
                    // Check if more than one argument was passed, output error if so.
                    // If no arguments change to home directory else, change to arg.
                    if (line_length > 2) perror("Too Many Arguments"); // Change
                    else if (line_length == 1) {
                        // One entry get home directory and change
                        char *home = getenv("HOME");
                        chdir(home);
                    } else { 
                        // There is an argument, chdir to first arg
                        chdir(newargv[1]);
                    }
                break;
                case (LS):
                    // Pass args to built-in function.
                    BuiltInLS(newargv, line_length);
                break;
                case (PRINTENV):
                    if (line_length == 1)  
                        fprintf(stderr, "printenv: zero args\n");
                    else
                        PrintEnv(newargv[1]);
                break;
                case (SETENV):
                    if (line_length == 3)  
                        SetEnv(newargv);
                    else 
                        fprintf(stderr, "setenv: invalid arguments\n");
                break;
            }
            BUILT_IN = 0;
        } else {         
            fflush(stdout);
            if (-1 == (child_pid = fork())) {
                perror("Cannot Fork.");     /* Fork Failed */
                exit(1);  
            } else if (0 == child_pid) {    /* Child Process */


                // If redirecting output, create file only if it does not exist. 
                if(REDIRECT_OUT == 1) { 
                    if ((out_fd = open(outfile, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR)) < 0) {
                        perror("Failed to open");
                    }
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                    REDIRECT_OUT = 0;
                }

                // If redirecting input, open file file and read input.
                if(REDIRECT_IN == 1) {
                    if ((in_fd = open(infile, O_RDONLY)) < 0) {
                        perror("Failed to open.");
                    }
                    dup2(in_fd, STDIN_FILENO);
                    close(in_fd);
                    REDIRECT_IN = 0;
                }
            
                // If child is a background process, redirect input to 
                // /dev/null so it doenst compete with foreground process for input
                if (NO_WAIT == 1) { /* Background Process */
                    int dev_null;
                    if ((dev_null = open("/dev/null", O_WRONLY)) < 0) {
                        perror("Failed to open.");
                    }
                    dup2(dev_null, STDIN_FILENO);
                    close(dev_null);
                }

                // If pipe flag is set we need to create pipe and fork processes
                // Else we continue as usual
                if(PIPE == 1) 
                    ProcessPipe(newargv);
                else  {
                    execvp(newargv[0], newargv);
                    fprintf(stderr, "%s: Command not found. \n", newargv[0]);
                    exit(9);
                }
           } else {
                // If background job flag not set, wait for child.
                // Else print childs process id and reset flag. 
                if (NO_WAIT == 0) {
                    for (;;) {
                        pid_t pid;
                        CHK(pid = wait(NULL));

                        if (pid == child_pid) break;
                    }
                }  else {
                    printf("\n%s [%d]\n", newargv[0], child_pid);
                    NO_WAIT = 0;
                }
            }
        }
    }    
    killpg(getpid(), SIGTERM);
    printf("p2 terminated.\n");

    exit(0);         
    }    

    
int Parse(char *buffer, char *args[], char* input, char *output) {
    int w_count = 0;
    int w_length = 0;
    int index = 0;
    int end = 0;
    char word[255];
    char *tmp;
    
    while ((w_length = getword(word)) > 0) {
        
            
        // Set input redirect and raise flag
        if (strcmp(word, "<") == 0) {
            REDIRECT_IN = 1;
            if ((w_length = getword(word)) > 0) {
                if ((tmp = CheckDollarSign(word)) == NULL) 
                    PARSE_ERROR = 1;
                strcpy(input, tmp);
            }
            continue; 
        // Set output redirect and raise flag
        } else if (strcmp(word, ">") == 0) {
            REDIRECT_OUT = 1;
            if ((w_length = getword(word)) > 0) {
                if ((tmp = CheckDollarSign(word)) == NULL) 
                    PARSE_ERROR = 1;
                strcpy(output, tmp);
            }
            continue;
        // Set pipe flag 
        } else if (strcmp(word, "|") == 0) {
            if (PIPE == 1) {
                PARSE_ERROR = 1;
                fprintf(stderr, "Invalid Amount of Pipes\n");
            } 
            PIPE = 1;
            args[w_count++] = '\0';
            next_proc = w_count;
            continue;
        } else if (strcmp(word, "&") == 0) {
            // "&" treated as a line terminator
            // Set flag not to wait and prepare buffers
            // to return back to main
            NO_WAIT = 1;
            buffer[index] = '\0';
            args[w_count] = '\0';
            if (w_count == 0) 
                args[0] = '\0'; 
            return w_count;
        // Check if first char is $. Remove the first char and getenv
        } else if (ESCAPED_$ != 1 && word[0] == '$')  {
            memmove(word, word+1, strlen(word));
            // If there isn't anything following the $ print an error
            if ((args[w_count++] = getenv(word)) == NULL) {
                PARSE_ERROR = 1;
                fprintf(stderr, "%s: Undefined Variable\n", word);
            }
            ESCAPED_$ = 0;
        } else {
            // Add the address of the beginning of word stored in 
            // the buffer to the args char* array. Add '\0' to 
            // terminate word and prepare for next word to be parsed
            args[w_count++] = &buffer[index]; 
        
            int i;
            for (i = 0; i < w_length; i++) {
            buffer[index++] = word[i]; 
            }
            buffer[index++] = '\0';
        }
    }

    /* EOF on first word */
    if (w_length == -1 && w_count == 0)
        return -1;
    /* Built-in */ 
    if (index != 0) {
        if (strcmp(args[0], "cd") == 0) 
            BUILT_IN = CD;
        if (strcmp(args[0], "ls-F") == 0) 
            BUILT_IN = LS;
        if (strcmp(args[0], "printenv") == 0)
            BUILT_IN = PRINTENV;
        if (strcmp(args[0], "setenv") == 0)
            BUILT_IN = SETENV;
    }

    buffer[index] = '\0';
    args[w_count] = '\0';
    if (w_count == 0)
        args[0] = '\0';

    return w_count;
}

void BuiltInLS(char *args[], int argcount) {
    DIR *dirp;
    struct dirent *dp;
    struct stat statbuf; 
    char buf[255];
    
    if (argcount == 1) {
        // Open current directory if no args to ls-F
        dirp = opendir(".");
    } else {
        // Check if arg is a file or directory
        if (lstat(args[1], &statbuf) == -1) {
            perror("lstat");
        }

        // If directory, open for reading
        // Else parrot back file 
        if (S_ISDIR(statbuf.st_mode)) {
            if ((dirp = opendir(args[1])) == NULL) 
                fprintf(stderr, "%s\n\n%s Unreadable\n", args[1], args[1]);
        } else {
            printf("%s\n", args[1]);
            return; 
        }
    }

    while (dirp) {
        if ((dp = readdir(dirp)) != NULL) {
            // show . and .. but skip over hidden files.
            if (strcmp(dp->d_name, ".") == 0)
                strcpy(buf, dp->d_name);
            else if (strcmp(dp->d_name, "..") == 0)
                strcpy(buf, dp->d_name);
            else
                if (strncmp(dp->d_name, ".", 1) == 0) continue;
            
           

            // Append the fullpath to d_name to properly format 
            // for stat and lstat
            if (args[1] != NULL)
                sprintf(buf, "%s/%s", args[1], dp->d_name);
            else
                strcpy(buf, dp->d_name);



            printf("%s", dp->d_name);
            lstat(buf, &statbuf);
            
            // If executable in any form, append *
            if (S_ISREG(statbuf.st_mode) && statbuf.st_mode & 0111) {
                printf("*\n");
            } else {
                // Append according to st_mode
                switch(statbuf.st_mode & S_IFMT) {
                    case S_IFDIR: printf("/\n");     
                        break;
                    case S_IFLNK: 
                            // Check if link is broken 
                            if (stat(buf, &statbuf) == -1)
                                printf("&\n");
                            else
                                printf("@\n");
                        break;
                    default:      printf("\n");
                        break;
                }
            }
        } else {
            closedir(dirp);
            break;
        }
   } 
} 
        

void MyHandler(int signum) {
}


void PrintEnv(char* arg) {
    char *env = getenv(arg);
    
    // if env isnt null print the env variable
    // Else its empty and print nothing 
    if (env) 
        printf("%s\n", env);
    else 
        printf("\n");
}
    
void SetEnv(char **args) {
    CHK(setenv(args[1], args[2], 1));
}
   
void ProcessPipe (char **args) {
    int pipefds[2];
    pid_t first, second;

    CHK(pipe(pipefds));
    
    /* Forking first of two processes */
    CHK(first = fork());
    if (0 == first) {
        CHK(dup2(pipefds[1], STDOUT_FILENO));
        CHK(close(pipefds[0]));
        CHK(close(pipefds[1]));
        CHK(execvp(args[0], args));
    }
    
    /* Forking second */
    CHK(second = fork());
    if (0 == second) {
        CHK(dup2(pipefds[0], STDIN_FILENO));
        CHK(close(pipefds[0]));
        CHK(close(pipefds[1]));
        CHK(execvp(args[next_proc], args+next_proc));
    }
    
    CHK(close(pipefds[0]));
    CHK(close(pipefds[1]));
    
    /* Waiting for the two processes to complete */
    for (;;) {
        pid_t pid;
        CHK(pid = wait(NULL));
        if (pid == second) {
            break;
        }
    }

    /* terminating the calling process */
    exit(EXIT_SUCCESS);
} 


char* CheckDollarSign(char* w) {
    if (w[0] == '$') {
        memmove(w, w+1, strlen(w));
        return getenv(w);
    }
    
    return w;
}
