#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef TEST
#define main main_exclude
#endif

/**
 * Enum for the program mode:
 *      info 
 */
enum Mode {info, split, lookup};

route_t 

/**
 * This function parses the command line usage and stores the resulting values in global variables.
 * 
 * @param argv - Array of Strings of the commmand line arguments
 * @param argc - Number of command line args i.e length of argv
 * @return True/False whether the parse is valid or not. 
 */
static bool parseCommandlineArguments(int argc,const char *argv[]) {
    for(int i = 0; i < argc; i++) {

    }
    return true;
}



/**
 * 
 * 
 * Command Line Usage:  
 *      myapp info <address>/<prefix>
 *      myapp split <network>/<prefix> <count>
 *      myapp lookup <table-file> <address>...
 *      
 */
int main(int argc,const char *argv[])
{
    return 0;
}