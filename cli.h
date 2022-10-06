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

#include "utils.h"


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


void print_usage();


void print_help();


int parse_opts(int argc, char **argv, settings_t *settings);


#endif