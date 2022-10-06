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


enum err_codes {
    SUCCESS,
    USAGE_ERROR,
    PATH_ERROR,
};


/**
 * @brief Prints formated error message to the output
 * 
 * @param err_code 
 * @param message 
 */
void printerr(int err_code, const char *message,...);

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