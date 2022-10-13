/**
 * @file utils.h
 * @brief Header file of utils module
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#ifndef _FEEDREADER_UTILS_
#define _FEEDREADER_UTILS_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <regex.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/types.h>


#define PROGNAME "feedreader"

#define INIT_STRING_SIZE 32 //< Default initial size of strings (that are used as buffer)
#define INIT_NET_BUFF_SIZE 16384 //< Must be big enough for request (at least strlen(req_pattern) + strlen(host) + strlen(path) + strlen("\0"))

#define DEBUG


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
    INTERNAL_ERROR,
};


#define HTTP_REDIRECT -1
#define MAX_REDIR_NUM 5
#define TIMEOUT_S 5


typedef struct string {
    char *str;
    size_t size;
} string_t;


typedef struct string_slice {
    char *st;
    size_t len;
} string_slice_t;


typedef struct list_el {
    string_t *string;
    int indirect_lvl;
    struct list_el *next;
} list_el_t;


typedef struct list {
    list_el_t *header;
} list_t;

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
    RE_H_RESP_NUM, //< Maximum amount of tokens in URL 
};


typedef struct h_resp {
    string_slice_t version;
    string_slice_t status;
    string_slice_t phrase;
    string_slice_t location;
    char *msg;
}   h_resp_t;


//http-URI = "http" "://" authority path-abempty [ "?" query ] ("#" [fragment]) (see RFC9110)

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


#define ABS(x) (unsigned int)((x > 0) ? x : -x)

/**
 * @brief Prints formated error message to the output
 * 
 * @param err_code 
 * @param message 
 */
void printerr(int err_code, const char *message,...);

void printw(const char *message,...);

void init_h_url(h_url_t *h_url);

void h_url_dtor(h_url_t *h_url);

void erase_h_url(h_url_t *h_url);

void init_h_resp(h_resp_t *h_resp);

int parse_h_url(char *url, h_url_t *parsed_url, char* default_scheme_str);

int parse_http_resp(h_resp_t *parsed_resp, string_t *response, char *url);

list_el_t *new_element(char *str_content, size_t indir_level);

list_el_t *slice2element(string_slice_t *slice, size_t indir_level);

void list_init(list_t *list);

void list_dtor(list_t *list);

void list_append(list_t *list, list_el_t *new_element);

void erase_string(string_t *string);

bool is_empty(string_t *string);

void trunc_string(string_t *string, int n);

string_t *app_char(string_t **dest, char c);

string_t *set_string(string_t **dest, char *src);

string_t *ext_string(string_t *string);

string_t *set_stringn(string_t **dest, char *src, size_t n);

/**
 * @brief 
 * 
 * @param size 
 * @return string_t* 
 */
string_t *new_string(size_t size);

string_t *slice_to_string(string_slice_t *slice);

void string_dtor(string_t *string);

/**
 * @brief Allocates string buffer
 * 
 * @param size Capacity of the new buffer
 * @return Pointer to the newly allocated buffer or NULL (must be checked) 
 */
char *new_str(size_t size);

/**
 * @brief Moves the given pointer n places from the start of the string
 * 
 * @param str String, that should be shifted
 * @param n Number of places that, should be shifted
 * @return Pointer to the shifted string OR pointer to the terminating char \0
 * @warning If n parameter is greater than strlen of given string, pointer to 
 * the terminating character (\0) is returned 
 */
char *shift(char *str, size_t n);


#endif