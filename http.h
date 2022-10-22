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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "common.h"
#include "cli.h"
#include "url.h"

#define HTTP_REDIRECT -1 //< Return value signalizing http redirection 
#define MAX_REDIR_NUM 5 //< Maximum amount of redirections to prevent redirection cycle
#define TIMEOUT_S 5 //< Maximum time in sec for waiting for the writing/reading from BIO socket

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
    CON_LEN,
    RE_H_RESP_NUM, //< Maximum amount of tokens in URL 
};


#define CHECK_MIME_TYPE


#define ATOM_MIME "application/atom\\+xml"
#define RSS_MIME "application/rss\\+xml"
#define XML_MIME "text/xml"


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


void openssl_init();

void openssl_cleanup();

int send_request(BIO *bio, url_t *p_url, char *url);

int rec_response(BIO *bio, string_t *resp_b, char *url);

int https_connect(url_t *p_url, string_t *resp_b, char *url, settings_t *s);

int http_connect(url_t *parsed_url, string_t *resp_b, char *url);

int check_http_status(int status_c, string_t *phrase, char *url);

int http_redirect(h_resp_t *p_resp, list_el_t *cur_url);

int check_http_resp(h_resp_t *p_resp, list_el_t *cur_url, char *url);

int parse_http_resp(h_resp_t *parsed_resp, string_t *response, char *url);

void init_h_resp(h_resp_t *h_resp);

void erase_h_resp(h_resp_t *h_resp);

#endif