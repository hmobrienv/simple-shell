Final program for Operating Systems class

/*****Design*****/
echo null > & 
Due to the design of how output redirects work in my implementation. The 
next word/metacharacter returned getword will be the output file.

Piping is handled by forking two processes from the initial child that is forked
in main, I used pipe.c as reference and the code is almost exactly the same. 

If the design was ambigious I attempted to follow the behavior of tcsh. 

/*****Bugs*****/
-When a background process is executed, the foreground process hangs
after each command is executed and does not align with the prompt.
-Background redirects do not work.
-There is in issue when processing input 1, it will loop through the input
 multiple times
-If there is a name missing for redirection, you have to hit enter to 
 get another prompt.

