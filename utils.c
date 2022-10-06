/**
 * @file utils.c
 * @brief Src file of module with functions, that are used across the project
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#include "utils.h"


void printerr(int err_code, const char *message_format,...) {
    char *err_str[] = {
        "Success",
        "Usage error",
    };

    fprintf(stderr, "./%s: %s: ", PROGNAME, err_str[err_code]); //< TODO: Better message

    if(message_format) {
        va_list args;
        va_start (args, message_format);
        vfprintf(stderr, message_format, args);
    }

    fprintf(stderr, "\n");
}


char *new_str(size_t len) {
    char *str = (char *)malloc(sizeof(char)*len);
    
    if(str) {
        memset(str, 0, len);
    }

    return str;
}


char *shift(char *str, size_t n) {
    char *shifted_str = &str[0];
    for(size_t i = 0; i < n && shifted_str; i++) {
        shifted_str = &shifted_str[1];
    }

    return shifted_str;
}