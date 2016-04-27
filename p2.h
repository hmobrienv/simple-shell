#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "getword.h"
#include "CHK.h"

#define MAXITEM 100 /* max number of words per line */
#define MAXWORDLENGTH 256 /* max amount of characters in word */

int Parse(char *buffer, char *args[], char *input, char *output);
void MyHandler(int signum);
void ChangeDirectory();
void BuiltInLS(char *args[], int argcount);
void PrintEnv(char *arg);
void SetEnv(char **args);
void ProcessPipe(char **args);
char* CheckDollarSign(char *word);
