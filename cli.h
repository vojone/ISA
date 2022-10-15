/**
 * @file cli.h
 * @brief Header file of cli module
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */


#ifndef _FEEDREADER_CLI_
#define _FEEDREADER_CLI_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#define PROGNAME "feedreader"

enum err_codes {
    SUCCESS,
    USAGE_ERROR,
    FILE_ERROR,
    URL_ERROR,
    CONNECTION_ERROR,
    COMMUNICATION_ERROR,
    PATH_ERROR,
    VERIFICATION_ERROR,
    HTTP_ERROR,
    FEED_ERROR,
    INTERNAL_ERROR,
};


/**
 * @brief Structure with information about used options in program-firendly
 * format (e. g. the result of the options parsing)
 */
typedef struct settings {
    char *url, *feedfile;
    char *certfile, *certaddr;
    bool time_flag, author_flag, asoc_url_flag, help_flag; 
} settings_t;


void init_settings(settings_t *settings);

/**
 * @brief Prints formated error message to the output
 * 
 * @param err_code 
 * @param message 
 */
void printerr(int err_code, const char *message,...);

void printw(const char *message,...);

void print_usage();

void print_help();

int parse_opts(int argc, char **argv, settings_t *settings);


#endif