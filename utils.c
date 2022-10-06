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
        "Error while opening file",
        "Internal error",
    };

    fprintf(stderr, "./%s: %s: ", PROGNAME, err_str[err_code]);

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


list_el_t *new_element(char *string_content) {
    list_el_t *new = (list_el_t *)malloc(sizeof(list_el_t));
    if(!new) {
        return NULL;
    }

    new->string = new_string(strlen(string_content) + 1);
    if(!new->string) {
        free(new);
        return NULL;
    }

    if(!set_string(new->string, string_content)) {
        free(new);
        return NULL;
    }

    new->next = NULL;

    return new;
}


void list_init(list_t *list) {
    list->header = NULL;
}


void list_dtor(list_t *list) {
    list_el_t *current = list->header;

    while(current) {
        list_el_t *tmp = current;
        current = current->next;

        string_dtor(tmp->string);
        free(tmp);
    }
}


void list_append(list_t *list, list_el_t *new_element) {
    list_el_t *current = list->header;

    if(!current) {
        list->header = new_element;
        return;
    }

    while(current->next) {
        current = current->next; 
    };

    current->next = new_element;
}


string_t *new_string(size_t len) {
    string_t *new_string = (string_t *)malloc(sizeof(string_t));
    if(!new_string) {
        return NULL;
    }

    new_string->str = new_str(len);
    if(!new_string->str) {
        free(new_string);
        return NULL;
    }

    new_string->len = len;

    return new_string;
}


string_t *ext_string(string_t *string) {
    const int coef = 2;

    char *tmp = string->str;
    string->str = new_str(string->len*coef);
    if(!string->str) {
        free(tmp);
        return NULL;
    }

    set_string(string, tmp);
    free(tmp);

    string->len *= coef;

    return string;
} 


void erase_string(string_t *string) {
    memset(string->str, 0, string->len);
}

string_t *app_char(string_t *dest, char c) {
    if(strlen(dest->str) >= dest->len - 1) {
        if(!ext_string(dest)) {
            return NULL;
        }    
    }

    dest->str[strlen(dest->str)] = c;

    return dest;
}


string_t *set_string(string_t *dest, char *src) {
    memset(dest->str, 0, dest->len);

    while(dest->len < strlen(src)) {
        if(!ext_string(dest)) {
            return NULL;
        }
    }

    strncpy(dest->str, src, dest->len);

    return dest;
}


void string_dtor(string_t *string) {
    free(string->str);
    free(string);
}


char *shift(char *str, size_t n) {
    char *shifted_str = &str[0];
    for(size_t i = 0; i < n && shifted_str; i++) {
        shifted_str = &shifted_str[1];
    }

    return shifted_str;
}