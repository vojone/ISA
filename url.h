#ifndef _FEEDREADER_URL_
#define _FEEDREADER_URL_


#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <strings.h>

#include "common.h"
#include "cli.h"


#define DEFAULT_URL_SCHEME "https://"

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
"(((" H16 ":){0,1}" H16 ")?::(" H16 ":){3}" LS32 ")|"\
"(((" H16 ":){0,2}" H16 ")?::(" H16 ":){2}" LS32 ")|"\
"(((" H16 ":){0,3}" H16 ")?::(" H16 ":)" LS32 ")|"\
"(((" H16 ":){0,4}" H16 ")?::" LS32 ")|"\
"(((" H16 ":){0,5}" H16 ")?::" H16 ")|"\
"(((" H16 ":){0,6}" H16 ")?::))"

#define REGNAME "((" UNRESERVED ")|(" SUBDELIMS ")|(" PCTENCODED "))+"

#define P_CHAR "(" UNRESERVED "|" SUBDELIMS "|[:@]|(" PCTENCODED "))"
#define PATH_ABS "(/(" P_CHAR ")*)+"
#define PATH_NO_SCH "(" UNRESERVED "|" SUBDELIMS "|[@]|(" PCTENCODED "))+\
(/(" P_CHAR ")*)*"

#define PATH_ROOTLESS "(" P_CHAR ")+(/(" P_CHAR ")*)*"
//End of part based on RFC3986


enum re_url_indexes {
    SCHEME_PART, //< (http|https)://
    USER_INFO_PART, //< see RFC3986 - userinfo part in http(s) is deprecated due to RFC9110)
    HOST, //< IP-literal / IPv4address / reg-name
    //IPVFUTURE, //< Just to tell user, that this format is not supported
    PORT_PART, //< : *DIGIT
    PATH, //< *( "/" segment )
    QUERY, //< ? *( pchar / "/" / "?" )
    FRAG_PART, //< # *( pchar / "/" / "?" )
    RE_URL_NUM, //< Maximum amount of tokens in URL 
};


enum re_path_indexes {
    ABS,
    NO_SCHEME,
    ROOTLESS,
    PATH_RE_NUM,
};


/**
 * @brief Structure with parts of HTTP url
 * 
 */
typedef struct url {
    string_t *url_parts[RE_URL_NUM];
    src_type_t type;
} url_t;


void init_url(url_t *url);

void url_dtor(url_t *url);

string_t *replace_path(string_t *orig_url, string_t *path);

int is_path(bool *result, char *str);

void erase_url(url_t *url);

int parse_url(char *url, url_t *parsed_url);


#endif
