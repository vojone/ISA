/**
 * @file utils.c
 * @brief Src file of module with definitions of functions, that are used 
 * across the project
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#include "common.h"

bool is_empty(string_t *string) {
    return (!string) || string->str == NULL || string->str[0] == '\0';
}


char *skip_w_spaces(char *str) {
    while(isspace(str[0])) {
        str = &(str[1]);
    }

    return str;
}


string_slice_t new_str_slice(char *ptr, size_t len) {
    string_slice_t slice = {
        .st = ptr,
        .len = len,
    };

    return slice;
}


bool is_line_empty(char *line_start_ptr) {
    return !strncmp(line_start_ptr, "\r\n", strlen("\r\n"));
}   


char *new_str(size_t size) {
    char *str = (char *)malloc(sizeof(char)*size);
    
    if(str) { 
        memset(str, 0, size); //< Initialization of allocated memory 
    }

    return str;
}


list_el_t *new_element(char *str_content, size_t indir_level) {
    list_el_t *new = (list_el_t *)malloc(sizeof(list_el_t));
    if(!new) {
        return NULL;
    }

    new->string = new_string(strlen(str_content) + 1);
    if(!new->string) {
        free(new);
        return NULL;
    }

    if(!(new->string = set_string(&(new->string), str_content))) {
        free(new);
        return NULL;
    }

    new->indirect_lvl = indir_level;
    new->next = NULL;

    return new;
}


list_el_t *slice2element(string_slice_t *slice, size_t indir_level) {
    list_el_t *new = (list_el_t *)malloc(sizeof(list_el_t));
    if(!new) {
        return NULL;
    }

    new->string = new_string(slice->len + 1);
    if(!new->string) {
        free(new);
        return NULL;
    }

    new->string = set_stringn(&(new->string), slice->st, slice->len);
    if(!new->string) {
        free(new);
        return NULL;
    }

    new->indirect_lvl = indir_level;
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


string_t *new_string(size_t size) {
    string_t *new_string_ = (string_t *)malloc(sizeof(string_t));
    if(!new_string_) {
        return NULL;
    }

    new_string_->str = new_str(size);
    if(!new_string_->str) {
        free(new_string_);
        return NULL;
    }

    new_string_->size = size;

    return new_string_;
}


string_t *slice_to_string(string_slice_t *slice) {
    string_t *string = new_string(slice->len + 1);
    if(!string) {
        return NULL;
    }

    string = set_stringn(&string, slice->st, slice->len);
    return string;
}


string_t *ext_string(string_t *string) {
    const int coef = 2;

    size_t old_size = string->size;

    string->str = realloc(string->str, string->size*coef);
    if(!string->str) {
        return NULL;
    }

    string->size *= coef;

    memset(&(string->str[old_size]), 0, string->size - old_size);

    return string;
} 


void erase_string(string_t *string) {
    if(string) {
        if(string->str) {
            memset(string->str, 0, string->size);
        }
    }
}


void trunc_string(string_t *string, int n) {
    char *str = string->str;

    int trunc_n = ABS(n) > string->size ? string->size : ABS(n);

    if(n > 0) {
        for(int i = 0; (unsigned)(trunc_n + i) < string->size; i++) {
            str[i] = str[trunc_n + i]; //< Move characters to beginning to remove characters at the begining 
        }
    }
    else {
        memset(&(str[string->size - trunc_n - 1]), 0, trunc_n); //< Remove characters from the end (replace them by '\0')
    }
}


string_t *app_char(string_t **dest, char c) {
    if(strlen((*dest)->str) >= (*dest)->size - 1) {
        if(!(*dest = ext_string(*dest))) {
            return NULL;
        }    
    }

    (*dest)->str[strlen((*dest)->str)] = c;

    return *dest;
}


string_t *set_string(string_t **dest, char *src) {
    if(*dest == NULL) {
        *dest = new_string(strlen(src) + 1);
        if(!(*dest)) {
            return NULL;
        }
    }

    memset((*dest)->str, 0, (*dest)->size);

    while((*dest)->size <= strlen(src)) {
        if(!(*dest = ext_string(*dest))) {
            return NULL;
        }
    }

    strncpy((*dest)->str, src, (*dest)->size);

    return *dest;
}


string_t *set_stringn(string_t **dest, char *src, size_t n) {
    if(*dest == NULL) {
        *dest = new_string(n + 1);
        if(!(*dest)) {
            return NULL;
        }
    }

    memset((*dest)->str, 0, (*dest)->size);

    while((*dest)->size <= strlen(src)) {
        if(!(*dest = ext_string(*dest))) {
            return NULL;
        }
    }

    strncpy((*dest)->str, src, n);

    return *dest;
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