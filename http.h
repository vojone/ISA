/**
 * @file feedreader.h
 * @brief Header file of http module - contains functions and structures
 * related to HTTP protocol and its secure variant - HTTPs
 * @note Uses opensll library
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 15. 10. 2022
 */

#ifndef _FEEDREADER_HTTP_
#define _FEEDREADER_HTTP_

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <regex.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "common.h"
#include "cli.h"
#include "url.h"

#define HTTP_REDIRECT -1 //< Return value signalizing http redirection 
#define MAX_REDIR_NUM 5 //< Maximum amount of redirections to prevent redirection cycle
#define TIMEOUT_MS 3000 //< Maximum time in ms for waiting for the writing/reading from BIO socket

#define HTTP_VERSION "HTTP/1.0" //< HTTP version (used in request)


/**
 * @brief Indexes of HTTP response parts (for better referencing them in the code)
 * 
 */
enum re_h_resp_indexes {
    H_PART, //< Part with headers
    LINE, //< Line with hdr
    VER,
    STAT,
    PHR,
    LOC,
    CON_TYPE,
    RE_H_RESP_NUM, //< Maximum amount of tokens in URL 
};

#define CHECK_MIME_TYPE


#define ATOM_MIME "application/atom\\+xml"
#define RSS_MIME "application/rss\\+xml"
#define XML_MIME "(text/xml)|(application/xml)"


/**
 * @brief Structure holding information about parsed HTTP response
 * 
 */
typedef struct h_resp {
    string_slice_t version, status, phrase;
    string_slice_t location, content_type, content_len;
    doc_type_t doc_type;
    char *msg; //< Ptr to the start of the response message
} h_resp_t;



/**
 * @brief Wrapping structure for parsing context of HTTP response 
 * @note Just for internal usage (inside module)
 */
typedef struct resp_parse_ctx {
    regex_t regexes[RE_H_RESP_NUM];
    char *cursor;
    regmatch_t regm[RE_H_RESP_NUM];
    int res[RE_H_RESP_NUM];
} resp_parse_ctx_t;


/**
 * @brief Initialization of OpenSSL library (necessary for HTTPS) 
 */
void openssl_init();


/**
 * @brief Teardown of OpenSSL library 
 */
void openssl_cleanup();


/**
 * @brief Sends HTTP request to server with given analyzed URL
 */
int send_request(BIO *bio, url_t *p_url, char *url);


/**
 * @brief Fetching reponse from HTTP server
 */
int rec_response(BIO *bio, string_t *resp_b, char *url);


/**
 * @brief Provides sending request, verification and fetching data for HTTPS 
 */
int https_load(url_t *p_url, string_t *resp_b, char *url, settings_t *s);


/**
 * @brief Provides sending request and fetching data for HTTP
 */
int http_load(url_t *parsed_url, string_t *resp_b, char *url);


/**
 * @brief Checks if status of HTTP response has code 2xx 
 */
int check_http_status(int status_c, string_t *phrase, char *url);


/**
 * @brief Performs HTTP redirection
 */
int http_redirect(h_resp_t *p_resp, list_el_t *cur_url);


/**
 * @brief Checks the validity of HTTP response
 */
int check_http_resp(h_resp_t *p_resp, list_el_t *cur_url, char *url);


/**
 * @brief Analyses HTTP response
 */
int parse_http_resp(h_resp_t *parsed_resp, string_t *response, char *url);


/**
 * @brief Initializes structure for result of response parsing
 */
void init_h_resp(h_resp_t *h_resp);


/**
 * @brief Performs the reset of h_resp_t structure 
 */
void erase_h_resp(h_resp_t *h_resp);

#endif
