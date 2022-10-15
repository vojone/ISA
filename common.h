/**
 * @file utils.h
 * @brief Header file of utils module
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#ifndef _FEEDREADER_COMMON_
#define _FEEDREADER_COMMON_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/types.h>

#include "cli.h"

#define INIT_STRING_SIZE 32 //< Default initial size of strings (that are used as buffer)
#define INIT_NET_BUFF_SIZE 16384 //< Must be big enough for request (at least strlen(req_pattern) + strlen(host) + strlen(path) + strlen("\0"))

#define DEBUG

#define HTTP_REDIRECT -1
#define MAX_REDIR_NUM 5
#define TIMEOUT_S 5

#define ABS(x) (unsigned int)((x > 0) ? x : -x)


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


bool is_empty(string_t *string);

char *skip_w_spaces(char *str);

string_slice_t new_str_slice(char *ptr, size_t len);

bool is_line_empty(char *line_start_ptr);

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