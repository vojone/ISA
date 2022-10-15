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

#define RSS_VERSION "2.0" //< Supported version of RSS format

/**
 * @brief Structure holding all important information about specific feed entry 
 * 
 */
typedef struct feed_el {
    xmlChar *title, *auth_name, *updated, *url;
    struct feed_el *next;
} feed_el_t;


/**
 * @brief Linked list with feed entries found in feed doc
 * 
 */
typedef struct feed_doc {
    xmlChar *src_name; //< Name of the feed doc
    feed_el_t *feed; //< Ptr to the first feed entry
} feed_doc_t;


/**
 * @brief Initializes libxml2 parser library, should be called before
 * any feed is parsed
 * 
 */
void xml_parser_init();


/**
 * @brief Cleanup of libxml2 parser library, should be called after parsing
 * is done
 * 
 */
void xml_parser_cleanup();


/**
 * @brief Initialzes linked list with feed document entries (feed_doc_t) to
 * the initial value
 * 
 * @param feed_doc STructure to be initialized
 */
void init_feed_doc(feed_doc_t *feed_doc);


/**
 * @brief Append feed entry to the end of linked list
 * 
 * @param feed_doc Feed document (linked list)
 * @param new_feed Feed to be added
 */
void add_feed(feed_doc_t *feed_doc, feed_el_t *new_feed);


/**
 * @brief Allocates element for the new feed entry and adds it
 * to the given feed doc
 * 
 * @param feed_doc Target feed doc (or NULL)
 * @return feed_el_t* Ptr the newly allocated element or NULL (should be checked)
 */
feed_el_t *new_feed(feed_doc_t *feed_doc);


/**
 * @brief Feed entry deallocator (should be called instead of simple free func)
 * 
 * @param feed Feed entry to be deallocated
 */
void feed_dtor(feed_el_t *feed);


/**
 * @brief Deallocates feed document structure
 * 
 * @param feed_doc Feed document to be deallocated
 */
void feed_doc_dtor(feed_doc_t *feed_doc);


/**
 * @brief Parses XML document with feed, the format is determined by the root tag
 * 
 * @param feed_doc Feed document structure to be filled by the data from parsed document
 * @param feed Pointer to buffer with document that should be parsed
 * @param url Source URL of XML document
 * @return int SUCCESS if parsing went OK
 */
int parse_feed_doc(feed_doc_t *feed_doc, char *feed, char *url);


/**
 * @brief Prints formatted feed to stdout
 * 
 * @param feed_doc Structure with information from feed document that should be printed
 * @param settings Settings structure to determine which information should be printed
 */
void print_feed_doc(feed_doc_t *feed_doc, settings_t *settings);

#endif