// Program 1 - Lexical Analyzer
// Due: February 10
// CS570 - Carroll

#include <stdio.h>
#include "getword.h"

// Global Flags
extern int ESCAPED_$;
extern int PARSE_ERROR;

int getword(char *w)
{
    int iochar;
    int count = 0;
    // Flags used during word processing
    int backslashFlag = 0;
    int singleQuoteFlag = 0;

    while ((iochar = getchar()) != EOF)
    {
        // Checking if new char will go over buffer
        // If it does it will put it back on the input stream
        // and return to continue with a new buffer.
        if (count == (STORAGE - 1)) {
            ungetc(iochar, stdin);
            w[count] = '\0';
            return count;
        }
  
        switch(iochar) {
            case '$': 
                if (backslashFlag == 1 || singleQuoteFlag == 1) {
                    ESCAPED_$ = 1;
                }
                w[count++] = iochar;
            break;
            // Handling blank spaces
            case ' ':
                // If we are in a single quote add to w
                // Else if its a backslash do the same and toggle flag off
                if (singleQuoteFlag == 1) {
                    w[count++] = iochar;
                }
                else if (backslashFlag == 1) {
                    w[count++] = iochar;
                    backslashFlag = 0;
                    continue;
                }
            break;
            // processing single quote
            case '\'':
                // If backslash add to w and toggle flag off
                // Else toggle flag
                if (backslashFlag == 1) {
                    w[count++] = iochar;
                    backslashFlag = 0;
                } else if (singleQuoteFlag == 0) {
                    singleQuoteFlag = 1;
                } else {
                    singleQuoteFlag = 0;
                    continue;
                }
            break;
            // Processing backslash
            case '\\':
                // Lookahead to see if single quote flag is on
                // and a single quote is next, add just single quote
                // Else backslash is part of quote if metacharacter
                // Follows. If not put back onto input stream
                iochar = getchar();

                /* Lookahead added for p4
                if (iochar == '$') {
                    ESCAPED_$ = 1;
                } */ 
    
                if ((iochar == '\'') && (singleQuoteFlag == 1)) {
                    w[count++] = iochar;
                    continue;
                } else if (singleQuoteFlag == 1) {
                    w[count++] = '\\';
                    w[count++] = iochar;
                }
                else {
                    ungetc(iochar, stdin);
                    iochar = '\\';
                }
                // Toggle backslash flag, add to w if two 
                // consecutive backslashes
                if (backslashFlag == 1) {
                     w[count++] = iochar;
                     backslashFlag = 0;
                     continue;
                } else {
                     backslashFlag = 1;
                     continue;
                }
            break;
            // Process newlines and semicolon. Similar Logic.
            case '\n': case ';':
                // If buffer has characters, okay to return. iochar must return
                // To be processed correctly, returning 0 and empty array.
                if (count > 0) {
                // If backslash or in a single quote, add semicolon to w
                    if ((backslashFlag == 1 || singleQuoteFlag == 1) &&
                        iochar == ';') {
                        w[count++] = iochar;
                        continue;
                    }
                    
                    ungetc(iochar, stdin);
                    w[count] = '\0';
                    
                    // if singleQuoteStill on error.
                    if (singleQuoteFlag == 1) {
                        fprintf(stderr, "Unmatched '.\n");
                        PARSE_ERROR = 1;
                    } 
    
                    return count;
                }
                else {
                    w[count] = '\0';
                    return 0;
                }
            break;
            // Process metacharacters with no side-effects.
            case '<': case '>': case '|': case '&':
                // In quote add to word and continue to next char
                if (singleQuoteFlag == 1) {
                    w[count++] = iochar;
                    continue;
                }
                // Chars in array put metacharacter back on input stream
                // return w array. If backslash on add to array and toggle
                if (count > 0) {
                    if (backslashFlag == 0) {
                        ungetc(iochar, stdin);
                        w[count] = '\0';
                        return count;
                    } else if (backslashFlag == 1) {
                        w[count++] = iochar;
                        backslashFlag = 0;
                        continue;
                    }
                }
                // Nothing in array, put metacharacter in array and return 
                else {
                    w[count++] = iochar;
                    if(backslashFlag == 0) {
                        w[count] = '\0';
                        return count;
                    }
                }
                backslashFlag = 0;
            break;
            // Regular character. 
            default:
                // Add char to array and toggle off unused flag
                if(backslashFlag == 1) backslashFlag = 0;
                w[count++] = iochar;
            break;
        }
        // Break out of while loop. End of word reached.
        if ((iochar == ' ' && count > 0) 
            && backslashFlag == 0 
            && singleQuoteFlag == 0) 
            break;
    }
    
    // Null termitate end of array
    w[count] = '\0';


    if (singleQuoteFlag == 1) 
        perror("Mismatched Quotes");

    // Return -1, 0, or count depending on coditions.
    if (iochar == EOF && count == 0)
        return -1;
    else if (count == 0)
        return 0; 
    else
        return count;
}

