/* 

Program goals: 
Write a program that creates 4 threads to process input from standard input as follows

    Thread 1, called the Input Thread, reads in lines of characters from the standard input.
    Thread 2, called the Line Separator Thread, replaces every line separator in the input by a space.
    Thread 3, called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", by a "^".
    Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters.

Furthermore, in your program these 4 threads must communicate with each other using the Producer-Consumer approach. 

After the program receives the stop-processing line and before it terminates, the program must produce all 80 character 
lines it can still produce based on the input lines which were received before the stop-processing line and which have 
not yet been processed to produce output.

Your program must output only lines with 80 characters (with a line separator after each line).

For the second replacement requirement, pairs of plus signs must be replaced as they are seen.

    Examples:
        The string “abc+++def” contains only one pair of plus signs and must be converted to the string "abc^+def".
        The string “abc++++def” contains two pairs of plus signs and must be converted to the string "abc^^def".
    Thus, whenever the Plus Sign thread replaces a pair of plus signs by a caret, the number of characters produced by the Plus Sign thread decreases by one compared to the number characters consumed by this thread.


Build Using:
gcc --std=gnu99 -o line_processor program5.c -lpthread

*/


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>




/* Constant definitions */
#define SEPARATOR '\n'
#define STOP_PROC "STOP\n"
#define BUFFER_SIZE 50      // Since the input will never have more than 49 before the stop processing line - unbounded buffer
#define MAX_LINE 1000       // This includes line separator ^ - this is maximum number of characters on a line
#define MAX_OUTPUT 80

// #define DEBUG

/* Buffers - Number of inputs * max number of characters for each input */
char buffer1[BUFFER_SIZE][MAX_LINE];        // Between input and separator
char buffer2[BUFFER_SIZE][MAX_LINE];        // Between separator and replace
char buffer3[BUFFER_SIZE][MAX_LINE];        // Between replace and output

/* Buffer counters and indexes*/
int count1 = 0, count2 = 0, count3 = 0;
int in1 = 0 , out1 = 0, in2 = 0, out2 = 0, in3 = 0, out3 = 0;

int STOPFLAG = 0;

// Initialize the mutex

pthread_mutex_t input_separator_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t separator_replace_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t replace_output_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize the condition variables

pthread_cond_t input_separator_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t separator_replace_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t replace_output_cond = PTHREAD_COND_INITIALIZER;



// TEMPLATE FUNC DESC

/* ----------------------------------------
    Function: 
===========================================
    Desc: 

    Params:
---------------------------------------- */


void* inputThread(void * arg);     // stdin -> inputThread (producer) -> separatorThread (consumer)
void* separatorThread(void * arg); // separatorThread (producer) -> replaceThread (consumer)
void* replaceThread(void * arg);   // replaceThread (producer) -> outputThread (consumer)
void* outputThread(void * arg);    // outputThread -> stdout

char* replaceLine(char *line);

int main(){

    pthread_t input, separator, replace, output;
    // Create the threads
    pthread_create(&input,      NULL, inputThread,      NULL);
    pthread_create(&separator,  NULL, separatorThread,  NULL);
    pthread_create(&replace,    NULL, replaceThread,    NULL);
    pthread_create(&output,     NULL, outputThread,     NULL);

    // Wait for threads to finish
    pthread_join(input,     NULL);
    #ifdef DEBUG
    printf("main: input thread joined\n");
    #endif
    pthread_join(separator, NULL);
    #ifdef DEBUG
    printf("main: separator thread joined\n");
    #endif
    pthread_join(replace,   NULL);
    #ifdef DEBUG
    printf("main: replace thread joined\n");
    #endif
    pthread_join(output,    NULL);
    #ifdef DEBUG
    printf("main: output thread joined\n");
    #endif

    return 0;
}

void* inputThread(void * arg){
    // Input as big as it can get
    char inputLine[MAX_LINE];

    // Continuously gets input unless fgets fails
    while(fgets(inputLine, MAX_LINE, stdin)) {
        // If input is stop processing command, stop
        if(strcmp(inputLine, STOP_PROC) == 0){
            STOPFLAG = -1;
            break;
        }
        
        pthread_mutex_lock(&input_separator_mutex);             // Lock mutex between input and separator
        strcpy(buffer1[in1], inputLine);                        // Copy inputted line over to buffer
        count1++;                                               // Increases count of lines in buffer 1
        in1 = (in1 + 1) % BUFFER_SIZE;                          // Increase index (rolling over if necessary) 
        pthread_cond_signal(&input_separator_cond);             // Let separator know there's something for it to do (if necessary)
        pthread_mutex_unlock(&input_separator_mutex);           // Unlock mutex
    }
    #ifdef DEBUG
    printf("input thread return statement reached\n");
    #endif
    // Function returns NULL
    return NULL;

}

/* ----------------------------------------
    Function: separatorThread
===========================================
    Desc: Retrieves lines from Buffer 1 and
    replaces newline characters with spaces.
    Stores the modified lines in Buffer 2

    Params:
---------------------------------------- */
void* separatorThread(void * arg){
    char line[MAX_LINE];

    while(1){
        // If stop flag is up and nothing left in buffer, break
        if(STOPFLAG != 0 && count1 == 0){
            break;
        }

// Consumer
        pthread_mutex_lock(&input_separator_mutex);         // Locking mutex between input and separator

        // Buffer is empty, wait for input to signal that buffer has data
        while (count1 == 0){
            pthread_cond_wait(&input_separator_cond , &input_separator_mutex);
        }
        strcpy(line, buffer1[out1]);                        // Copy over whatever is in the buffer to our variable
        count1--;                                           // Decreases count of lines in buffer 1        
        out1 = (out1 + 1) % BUFFER_SIZE;                    // Increase out1 index

        pthread_mutex_unlock(&input_separator_mutex);       // Unlock input and separator mutex

// Actual work
        // Go through line until we hit the end
        for(int i = 0; line[i] != '\0'; i++){
            // If we find SEPARATOR ('\n'), replace with space
            if(line[i] == SEPARATOR)
                line[i] = ' ';
        }

// Producer 
        
        pthread_mutex_lock(&separator_replace_mutex);       // Lock separator and replace mutex
        strcpy(buffer2[in2], line);                         // Copy over our modified string to buffer 2
        count2++;                                           // Increase count of lines in buffer 2
        in2 = (in2 + 1) % BUFFER_SIZE;                      // Increase in2 index
        pthread_cond_signal(&separator_replace_cond);       // Let replace thread know it has work to do (if necessary)
        pthread_mutex_unlock(&separator_replace_mutex);     // Unlock mutex
    }
    #ifdef DEBUG
    printf("separator thread return statement reached\n");
    #endif
    return NULL;

}

/* ----------------------------------------
    Function: replaceThread
===========================================
    Desc: Retrieves lines from Buffer 2 and
    replaces "++" with "^". Stores modified
    lines in Buffer 3.

    Params:
---------------------------------------- */
void* replaceThread(void * arg){
    char line[MAX_LINE];

    while(1){
        // If stop flag is up and nothing left in buffer, break
        if(STOPFLAG != 0 && count2 == 0){
            break;
        }

// Consumer 
        pthread_mutex_lock(&separator_replace_mutex);       // Lock mutex

        // Buffer is empty, wait for separator to signal that buffer has data
        while(count2 == 0){
            pthread_cond_wait(&separator_replace_cond, &separator_replace_mutex);
        }

        strcpy(line, buffer2[out2]);                        // Copy over buffer
        count2--;                                           // Decrease number of lines in buffer
        out2 = (out2 + 1) % BUFFER_SIZE;                    // Update index
        pthread_mutex_unlock(&separator_replace_mutex);     // Unlock thread
        

// Replace ++
        replaceLine(line);

// Producer

        pthread_mutex_lock(&replace_output_mutex);          // Lock mutex
        strcpy(buffer3[in3], line);                         // Copy over our modified string to buffer 3
        count3++;                                           // Increase count of lines in buffer 3
        in3 = (in3 + 1) % BUFFER_SIZE;                      // Increase index
        pthread_cond_signal(&replace_output_cond);          // Let output thread know it has work to do (if necessary)
        pthread_mutex_unlock(&replace_output_mutex);        // Unlock mutex
    }
    #ifdef DEBUG
    printf("replace thread return statement reached\n");
    #endif
    return NULL;
}
/* ----------------------------------------
    Function: replaceLine
===========================================
    Desc: Takes pointer, token, and 
    replaces the location specified by the 
    substring pointer with the pid. It 
    checks for multiple occurances.

    Params:
    token: char * , string to change.
    substring: char * , part of string to 
    change
---------------------------------------- */
char* replaceLine(char *line){
    char * pos; 
    while((pos = strstr(line , "++")) != NULL){
        memmove(pos + 1, pos + 2 , strlen(pos + 2)+ 1);
        *pos = '^';
    }
}

/* ----------------------------------------
    Function: outputThread
===========================================
    Desc: Retrieves lines from Buffer 3 and
    writes lines of exactly 80 characters
    to standard output.

    Params:
---------------------------------------- */
void* outputThread(void * arg){

    // Variables 
    char line[MAX_LINE];
    int len = 0;
    char output[MAX_OUTPUT + 1];                            // +1 for the terminating char '\0'

    while(1){
        // If stop flag is up and nothing left in buffer, break
        if(STOPFLAG != 0 && count3 == 0){
            break;
        }
// Consumer
        pthread_mutex_lock(&replace_output_mutex);          // Lock mutex
        // If nothing in buffer, wait till we get a signal that specifies otherwise
        while(count3 == 0){
            pthread_cond_wait(&replace_output_cond , &replace_output_mutex);
        }
        strcpy(line, buffer3[out3]);                        // Copy over whatever is in buffer to our line variable 
        count3--;                                           // Decrease the lines in buffer
        out3 = (out3 + 1) % BUFFER_SIZE;                    // Increase index for output
        pthread_mutex_unlock(&replace_output_mutex);        // Unlock mutex

// Actual output
        for( int i = 0; line[i] != '\0'; i++){
            // Write character to index of output char - incrementing after use
            output[len++] = line[i];
            if(len == MAX_OUTPUT){
                output[MAX_OUTPUT] = '\0';
                printf("%s\n",output );
                fflush(stdout);
                len = 0;
            }
        }
    }
    #ifdef DEBUG
    printf("output thread return statement reached\n\n");
    #endif
    return NULL;
}