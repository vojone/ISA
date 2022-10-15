#ifndef _FEEDREADER_HTTP_
#define _FEEDREADER_HTTP_

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <regex.h>

#include <sys/socket.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "common.h"
#include "cli.h"

#define HTTP_REDIRECT -1 //< Return value signalizing http redirection 
#define MAX_REDIR_NUM 5 //< Maximum amount of redirections to prevent redirection cycle
#define TIMEOUT_S 5 //< Maximum time in sec for waiting for the writing/reading from BIO socket

#define HTTP_VERSION "HTTP/1.0" //< HTTP version

//Based on RFC3986
#define HEXDIG "[0-9a-f]"
#define H16 HEXDIG "{1,4}"
#define LS32 "(" H16 ":" H16 ")|" IPV4ADDRESS

#define UNRESERVED "[a-z0-9\\._~\\-]"
#define SUBDELIMS "[@!\\$&'()*+,;=]"
#define PCTENCODED "%" HEXDIG HEXDIG
#define DECOCTED "[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-5]{2}"
#define IPV4ADDRESS DECOCTED "\\." DECOCTED "\\." DECOCTED "\\." DECOCTED
#define IPV6ADDRESS "(((" H16 ":){6}" LS32 ")|"\
"(::(" H16 ":){5}" LS32 ")|"\
"((" H16 ")?::(" H16 ":){4}" LS32 ")|"\
"(((" H16 ":){,1}" H16 ")?::(" H16 ":){3}" LS32 ")|"\
"(((" H16 ":){,2}" H16 ")?::(" H16 ":){2}" LS32 ")|"\
"(((" H16 ":){,3}" H16 ")?::(" H16 ":)" LS32 ")|"\
"(((" H16 ":){,4}" H16 ")?::" LS32 ")|"\
"(((" H16 ":){,5}" H16 ")?::" H16 ")|"\
"(((" H16 ":){,6}" H16 ")?::))"
#define REGNAME "((" UNRESERVED ")|(" SUBDELIMS ")|(" PCTENCODED "))+"
//End of part base od RFC3986


enum re_h_url_indexes {
    SCHEME_PART, //< (http|https)://
    USER_INFO_PART, //< see RFC3986 - userinfo part in http(s) is deprecated due to RFC9110)
    HOST, //< IP-literal / IPv4address / reg-name
    //IPVFUTURE, //< Just to tell user, that this format is not supported
    PORT_PART, //< : *DIGIT
    PATH, //< *( "/" segment )
    QUERY, //< ? *( pchar / "/" / "?" )
    FRAG_PART, //< # *( pchar / "/" / "?" )
    RE_H_URL_NUM, //< Maximum amount of tokens in URL 
};


typedef struct h_url {
    string_t *h_url_parts[RE_H_URL_NUM];
} h_url_t;


enum re_h_resp_indexes {
    H_PART,
    LINE,
    VER,
    STAT,
    PHR,
    LOC,
    CON_TYPE,
    RE_H_RESP_NUM, //< Maximum amount of tokens in URL 
};


typedef struct h_resp {
    string_slice_t version;
    string_slice_t status;
    string_slice_t phrase;
    string_slice_t location;
    string_slice_t content_type;
    char *msg;
}   h_resp_t;


void openssl_init();

int send_request(BIO *bio, h_url_t *p_url, char *url);

int rec_response(BIO *bio, string_t *resp_b, char *url);

int https_connect(h_url_t *p_url, string_t *resp_b, char *url, settings_t *s);

int http_connect(h_url_t *parsed_url, string_t *resp_b, char *url);

int check_http_status(int status_c, string_t *phrase, char *url);

int http_redirect(h_resp_t *p_resp, list_el_t *cur_url);

int check_http_resp(h_resp_t *p_resp, list_el_t *cur_url, char *url);

int parse_http_resp(h_resp_t *parsed_resp, string_t *response, char *url);

void init_h_url(h_url_t *h_url);

void h_url_dtor(h_url_t *h_url);

void erase_h_url(h_url_t *h_url);

void init_h_resp(h_resp_t *h_resp);

int parse_h_url(char *url, h_url_t *parsed_url, char* default_scheme_str);

#endif