/**
 * @file feed.c
 * @brief Header file of feed module (mod. for parsing and printing XML 
 * with RSS2/Atom feeds)
 * @note Uses libxml2 library to parse XML docs
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 15. 10. 2022 
 */

#ifndef _FEEDREADER_FEED_
#define _FEEDREADER_FEED_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "common.h"
#include "cli.h"

#define RSS_VERSION "2.0"

typedef struct feed_el {
    xmlChar *title, *auth_name, *updated, *url;
    struct feed_el *next;
} feed_el_t;

typedef struct feed_doc {
    xmlChar *src_name;
    feed_el_t *feed;
} feed_doc_t;

void xml_parser_init();

void xml_parser_cleanup();

void init_feed_doc(feed_doc_t *feed_doc);

void add_feed(feed_doc_t *feed_doc, feed_el_t *new_feed);

feed_el_t *new_feed(feed_doc_t *feed_doc);

void feed_dtor(feed_el_t *feed);

void feed_doc_dtor(feed_doc_t *feed_doc);

int parse_feed_doc(feed_doc_t *feed_doc, char *feed, char *url);

void print_feed_doc(feed_doc_t *feed_doc, settings_t *settings);

#endif