#ifndef _FEEDREADER_HTTP_
#define _FEEDREADER_HTTP_


#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include <sys/socket.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "common.h"
#include "cli.h"

#define HTTP_VERSION "HTTP/1.1"

void openssl_init();

int send_request(BIO *bio, h_url_t *p_url, char *url);

int rec_response(BIO *bio, string_t *resp_b, char *url);

int https_connect(h_url_t *p_url, string_t *resp_b, char *url, settings_t *s);

int http_connect(h_url_t *parsed_url, string_t *resp_b, char *url);

int check_http_status(int status_c, string_t *phrase, char *url);

int http_redirect(h_resp_t *p_resp, list_el_t *cur_url);

int check_http_resp(h_resp_t *p_resp, list_el_t *cur_url, char *url);

#endif