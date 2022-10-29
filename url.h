/**
 * @file url.h
 * @brief Header file of url module
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 23. 10. 2022
 */


#ifndef _FEEDREADER_URL_
#define _FEEDREADER_URL_


#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <strings.h>

#include "common.h"
#include "cli.h"


#define DEFAULT_URL_SCHEME "https://" //< Default scheme (it is added to URL if user provides URL without any scheme)

//Regex macros for parsing URL
//Based on RFC3986
#define SCHEME "[a-z][a-z0-9+\\-\\.]*://"

#define HEXDIG "[0-9a-f]"
#define H16 HEXDIG "{1,4}"
#define LS32 "(" H16 ":" H16 ")|" IPV4ADDRESS

#define UNRESERVED "[a-z0-9._~-]"
#define SUBDELIMS "[@!$&'()*+,;=]"
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

//Auxiliary regexes for percent encoding
#define PATH_ABS_STRICT "^(/(" P_CHAR ")*)+"
#define QUERY_STRICT "^\\?(" P_CHAR "|[:@/?])*"
#define FRAG_STRICT "^#(" P_CHAR "|[:@/?])*"

#define PATH_ABS "(/(" P_CHAR "|[^\\s#?/])*)+"
#define PATH_NO_SCH "(" UNRESERVED "|" SUBDELIMS "|[@]|(" PCTENCODED ")|[^\\s#?/])+\
(/(" P_CHAR "|[^\\s#?/])*)*"

#define PATH_ROOTLESS "(" P_CHAR "|[^\\s#?/])+(/(" P_CHAR "|[^\\s#?/])*)*"
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


/**
 * @brief Indexes for path regexes 
 */
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
    src_type_t type; //< Type of source (ATOM/RSS/XML)
} url_t;


/**
 * @brief Initialization of URL structure 
 */
void init_url(url_t *url);


/**
 * @brief Deallocation of URL structure 
 */
void url_dtor(url_t *url);


/**
 * @brief Replaces path in given original URL
 */
string_t *replace_path(string_t *orig_url, string_t *path);


/**
 * @brief Determines wheter given string is path (absolute or relative) 
 */
int is_path(bool *result, char *str);


/**
 * @brief Re-initialization of given URL 
 */
void erase_url(url_t *url);


/**
 * @brief Analysis of URL 
 */
int parse_url(char *url, url_t *parsed_url);


#endif
