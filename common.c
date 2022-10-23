/**
 * @file common.c
 * @brief Src file of module with definitions of functions, that are used 
 * across the project (mostly suited for ADT declared in .h)
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 23. 10. 2022
 */

#include "common.h"



char *new_str(size_t size) {
    char *str = (char *)malloc(sizeof(char)*size);
    
    if(str) { 
        memset(str, 0, size); //< Initialization of allocated memory 
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


list_el_t *new_element(char *str_content) {
    list_el_t *new = (list_el_t *)malloc(sizeof(list_el_t));
    if(!new) { //< Allocation error
        return NULL;
    }

    new->string = new_string(strlen(str_content) + 1); //< Content + '\0'
    if(!new->string) {
        free(new);
        return NULL;
    }

    if(!(new->string = set_string(&(new->string), str_content))) {
        free(new);
        return NULL;
    }

    //Initialization
    new->indirect_lvl = 0;
    new->result = SUCCESS;
    new->next = NULL;

    return new;
}


list_el_t *new_element_non_dup(string_t *content, size_t indirection_lvl) {
    list_el_t *new = (list_el_t *)malloc(sizeof(list_el_t));
    if(!new) { //< Allocation error
        return NULL;
    }

    new->string = content;

    //Initialization
    new->indirect_lvl = indirection_lvl;
    new->result = SUCCESS;
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

    while(current) { //< Free all elements in the list
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

    while(current->next) { //< Got to he end of the list
        current = current->next; 
    };

    current->next = new_element;
}


int move_to_list(string_t *buffer, list_t *dst_list) {
    list_el_t *new_url = new_element(buffer->str);
    if(!new_url) {
        printerr(INTERNAL_ERROR, "Unable to move '%s' to the list!", buffer->str);
        return INTERNAL_ERROR;
    }

    new_url->indirect_lvl = 0; //< It is original URL

    list_append(dst_list, new_url);

    return SUCCESS;
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

    int trunc_n = ABS(n) > string->size ? string->size : ABS(n); //< Limit the truncation to the size of the string

    if(n > 0) { //< + means from start
        for(int i = 0; (unsigned)(trunc_n + i) < string->size; i++) {
            str[i] = str[trunc_n + i]; //< Move characters to beginning to remove characters at the begining 
        }
    }
    else { //< - means from the end
        memset(&(str[strlen(string->str) - trunc_n]), 0, trunc_n); //< Remove characters from the end (replace them by '\0')
    }
}


void trunc_str(char *str, int n) {
    size_t str_len = strlen(str);
    int trunc_n = ABS(n) > str_len ? str_len : ABS(n); //< Limit the truncation to the size of the string

    if(n > 0) { //< + means from start
        for(int i = 0; (unsigned)(trunc_n + i) < str_len; i++) {
            str[i] = str[trunc_n + i]; //< Move characters to beginning to remove characters at the begining 
        }
    }
    else { //< - means from the end
        char *str_end;
        for(int i = 1; (signed)(str_len - i) > 0; i++) {
            if(*(str_end = &(str[str_len - i])) == '\0') {
                break;
            }
        }
    
        memset(&(str_end[str_len - trunc_n]), 0, trunc_n); //< Remove characters from the end (replace them by '\0')
    }
}


void string_to_lower(string_t *string) {
    for(size_t i = 0; i < string->size; i++) {
        string->str[i] = tolower(string->str[i]);
    }
}


string_t *app_char(string_t **dest, char c) {
    if(strlen((*dest)->str) >= (*dest)->size - 1) {
        if(!(*dest = ext_string(*dest))) { //< Extend string if necessary
            return NULL;
        }    
    }

    (*dest)->str[strlen((*dest)->str)] = c;

    return *dest; //< Return pointer to the (reallocated) string
}


void rm_char(string_t *dest, size_t index) {
    if(index >= strlen(dest->str)) {
        return;
    }
    else {
        size_t rest_len = dest->size - index - 1;
        memmove(&(dest->str[index]), &(dest->str[index + 1]), rest_len);
    }
}


string_t *ins_char(string_t **dest, size_t index, char c) {
    if(index > strlen((*dest)->str)) {
        return *dest;
    }
    else {
        if(strlen((*dest)->str) >= (*dest)->size - 1) {
            if(!(*dest = ext_string(*dest))) { //< Extend string if necessary
                return NULL;
            }    
        }

        size_t rest_len = (*dest)->size - index - 1;
        memmove(&((*dest)->str[index + 1]), &((*dest)->str[index]), rest_len);
        (*dest)->str[index] = c;
    }

    return *dest;
}


string_t *app_string(string_t **dest, char *src) {
    for(int i = 0; src[i]; i++) {
        *dest = app_char(dest, src[i]);
        if(!(*dest)) {
            return NULL;
        }
    }

    return *dest;
}


string_t *set_string(string_t **dest, char *src) {
    if(*dest == NULL) { //< Allocate the new string if it is necessary
        *dest = new_string(strlen(src) + 1);
        if(!(*dest)) {
            return NULL;
        }
    }

    memset((*dest)->str, 0, (*dest)->size);

    while((*dest)->size <= strlen(src)) {
        if(!(*dest = ext_string(*dest))) { //< Extend to the necessary size
            return NULL;
        }
    }

    strncpy((*dest)->str, src, (*dest)->size); //< Copy src to string structure

    return *dest;
}


string_t *ext_string(string_t *string) {
    const int coef = 2; //< Multiplication coeficient of string resizing

    size_t old_size = string->size;

    string->str = realloc(string->str, string->size*coef);
    if(!string->str) {
        return NULL;
    }

    string->size *= coef;

    memset(&(string->str[old_size]), 0, string->size - old_size);

    return string;
}


string_t *set_stringn(string_t **dest, char *src, size_t n) {
    if(*dest == NULL) {
        *dest = new_string(n + 1);
        if(!(*dest)) {
            return NULL;
        }
    }

    memset((*dest)->str, 0, (*dest)->size);

    while((*dest)->size <= n) {
        if(!(*dest = ext_string(*dest))) {
            return NULL;
        }
    }

    strncpy((*dest)->str, src, n);

    return *dest;
}


string_t *new_string(size_t size) {
    string_t *new_string_ = (string_t *)malloc(sizeof(string_t));
    if(!new_string_) {
        return NULL;
    }

    //Init
    new_string_->str = new_str(size);
    if(!new_string_->str) {
        free(new_string_);
        return NULL;
    }

    new_string_->size = size;

    return new_string_;
}


string_t *slice2string(string_slice_t *slice) {
    string_t *string = new_string(slice->len + 1);
    if(!string) {
        return NULL;
    }

    string = set_stringn(&string, slice->st, slice->len);
    return string;
}


void string_dtor(string_t *string) {
    free(string->str);
    free(string);
}

