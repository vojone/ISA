/**
 * @file feedreader.h
 * @brief Header file of main file of feedreader program
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 5. 11. 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


#include "common.h"
#include "cli.h"
#include "http.h"
#include "feed.h"
#include "url.h"


/**
 * @brief Wrapping structure for data that are necessary for analysis of the document 
 * with feed
 */
typedef struct data_ctx {
    char* doc_start; //< Ptr to start of the document with feed
    int exp_type; //< Expected type of document
    url_t *parsed_url; //< Analysed URL
    char *url; //< Original URL
} data_ctx_t;


