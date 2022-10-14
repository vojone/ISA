/**
 * @file feedreader.h
 * @brief Header file of main file of feedreader program
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <libxml/parser.h>
#include <libxml/tree.h>


#include "common.h"
#include "cli.h"
#include "http.h"

#define RSS_VERSION "2.0"

typedef struct feed_el {
    xmlChar *title;
    xmlChar *auth_name;
    xmlChar *updated;
    xmlChar *url;
    struct feed_el *next;
} feed_el_t;

typedef struct feed_doc {
    xmlChar *src_name;
    feed_el_t *feed;
} feed_doc_t;


