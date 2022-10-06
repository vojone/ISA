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


#define PROGNAME "feedreader"

#define INIT_STRING_SIZE 32


enum err_codes {
    SUCCESS,
    USAGE_ERROR,
    FILE_ERROR,
    INTERNAL_ERROR
};


typedef struct string {
    char *str;
    size_t len;
} string_t;


typedef struct list_el {
    string_t *string;
    struct list_el *next;
} list_el_t;


typedef struct list {
    list_el_t *header;
} list_t;


/**
 * @brief Prints formated error message to the output
 * 
 * @param err_code 
 * @param message 
 */
void printerr(int err_code, const char *message,...);

list_el_t *new_element(char *string_content);

void list_init(list_t *list);

void list_dtor(list_t *list);

void list_append(list_t *list, list_el_t *new_element);

void erase_string(string_t *string);

string_t *app_char(string_t *dest, char c);

string_t *set_string(string_t *dest, char *src);

/**
 * @brief 
 * 
 * @param len 
 * @return string_t* 
 */
string_t *new_string(size_t len);

void string_dtor(string_t *string);

/**
 * @brief Allocates string buffer
 * 
 * @param len Capacity of the new buffer
 * @return Pointer to the newly allocated buffer or NULL (must be checked) 
 */
char *new_str(size_t len);

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