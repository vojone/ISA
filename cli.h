/**
 * @file cli.h
 * @brief Header file of cli module - responsible for communication with user
 * @note There are only declarations of exported functions and structures (
 * there are also internal functions/structures in cli.c file)
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 31. 10. 2022
 */


#ifndef _FEEDREADER_CLI_
#define _FEEDREADER_CLI_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>


#define PROGNAME "feedreader" //< The name of the program for better filtering of error/warning messages

#define CLI_WARNINGS //< Comment to turn off warnings

//#define DEBUG //< Uncomment to allow debugging prints to stderr

/**
 * @brief Error codes, that can be returned by program
 * 
 * @see manual.pdf for description of all these return codes
 */
enum err_codes {
    SUCCESS, //< Everything went OK
    USAGE_ERROR,
    FILE_ERROR,
    URL_ERROR,
    CONNECTION_ERROR,
    COMMUNICATION_ERROR,
    PATH_ERROR,
    VERIFICATION_ERROR,
    HTTP_ERROR,
    FEED_ERROR,
    INTERNAL_ERROR, //< Reserved for allocation errors and other similar errors
};


/**
 * @brief Structure with information about arguments of the program 
 * in program-firendly format (e. g. the result of the options parsing)
 * 
 */
typedef struct settings {
    char *url, *feedfile; //< Options with argument (or it is single argument of program - such as url)
    char *certfile, *certaddr;
    bool time_flag, author_flag, asoc_url_flag, help_flag; //< Options without arguments
} settings_t;


/**
 * @brief Auxiliary structure to avoid too many parameters of option parsing 
 * functions
 * @note Just for internal usage
 */
typedef struct opt {
    char *name; //< Pointer to the name of option (usefull expecially for error messages)
    bool *flag; //< Pointer to the variable, that signalizes that option is ON
    char **arg; //< Pointer to the pointer to the argument of specific option
} opt_t;


void init_settings(settings_t *settings);


/**
 * @brief Prints formated error message to the stderr
 * 
 * @param err_code Error code of the error (value from err_codes enum)
 * @param message Auxiliary format of message of the error
 * @param ... 
 */
void printerr(int err_code, const char *message,...);


/**
 * @brief Prints warning message to stderr (macro CLI_WARNINGS must be defined)
 * 
 * @param message Format of message of the warning
 * @param ... 
 */
void printw(const char *message,...);


/**
 * @brief Prints usage information to stdout
 * 
 */
void print_usage();


/**
 * @brief Prints help message to stdout
 * 
 */
void print_help();


/**
 * @brief Parses options of the program (short options and long options
 * must be firstly defined by implementation in internal functions rec_lopt and
 * rec_opt) and sets settings structure 
 * 
 * @param argc Argument count from main function of the program
 * @param argv Argument vector from the main function of the program
 * @param settings Initialized settings structure for result
 * @return Returns SUCCESS if everything went OK (all options were recognized and
 * their arguments were found), otherwise USAGE error
 */
int parse_opts(int argc, char **argv, settings_t *settings);


#endif
