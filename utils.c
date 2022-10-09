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


void printw(const char *message_format,...) {
    fprintf(stderr, "./%s: Warning: ", PROGNAME);

    if(message_format) {
        va_list args;
        va_start (args, message_format);
        vfprintf(stderr, message_format, args);
    }

    fprintf(stderr, "\n");
}


int hostname_validation(char *hostname) {
    return 0;
}

void init_h_url(h_url_t *h_url) {
    for(int i = 0; i < RE_H_URL_NUM; i++) {
        h_url->h_url_parts[i] = NULL;
    }
}


void h_url_dtor(h_url_t *h_url) {
    for(int i = 0; i < RE_H_URL_NUM; i++) {
        if(h_url->h_url_parts[i]) {
            string_dtor(h_url->h_url_parts[i]);
        }
    }
}



int prepare_patterns(regex_t *regexes) {
    //Patterns are based on RFC3986
    char *patterns[] = {
        "^(http|https)://",
        "(" UNRESERVED "|" SUBDELIMS "|:|(" PCTENCODED "))+@",
        "(" IPV4ADDRESS ")|(\\[" IPV6ADDRESS "\\])|(" REGNAME ")",
        ":[0-9]*",
        "(/(" UNRESERVED "|" SUBDELIMS "|[:@]|(" PCTENCODED "))*)+",
        "\\?([" UNRESERVED "|" SUBDELIMS "|[:@/?]|(" PCTENCODED "))*",
        "#(" UNRESERVED "|" SUBDELIMS "|[:@/?]|(" PCTENCODED "))*"
    };
    //-----------------------------

    for(int i = 0; i < RE_H_URL_NUM; i++) {
        if(regcomp(&(regexes[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Invalid compilation of regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    return SUCCESS;
}


void free_all_patterns(regex_t *regexes) {
    for(int i = 0; i < RE_H_URL_NUM; i++) {
        regfree(&(regexes[i]));
    }
}


int res_url(bool is_inv, bool int_err, int *res, h_url_t *p_url, char *url) {
    if(int_err) {
        printerr(INTERNAL_ERROR, "Error while parsing URL!");
        return INTERNAL_ERROR;
    }

    if(res[HOST] == REG_NOMATCH || is_inv) { //< Host was not found in URL
        printerr(INTERNAL_ERROR, "Invalid URL '%s'!", url);
        return INVALID_URL;
    }

    if(res[SCHEME_PART] == REG_NOMATCH) {
        printw("Scheme part of URL '%s' was not found! It will be set to default!", url);
    }

    if(res[USER_INFO_PART] != REG_NOMATCH) {
        printw("Deprecated userinfo part '%s' was found in URL '%s'! It will be ignored!", p_url->h_url_parts[USER_INFO_PART]->str, url);
    }

    return SUCCESS;
}

int fix_url(h_url_t *p_url, char* def_scheme_part_str) {
    if(!p_url->h_url_parts[SCHEME_PART]) {
        size_t len = strlen(def_scheme_part_str);

        p_url->h_url_parts[SCHEME_PART] = new_string(len);
        if(p_url->h_url_parts[SCHEME_PART]) {
            return INTERNAL_ERROR;
        }

        set_stringn(p_url->h_url_parts[SCHEME_PART], def_scheme_part_str, len);
    }

    if(!p_url->h_url_parts[PATH]) {
        char *path = "/";
        size_t len = strlen(path);

        p_url->h_url_parts[PATH] = new_string(len);
        if(p_url->h_url_parts[PATH]) {
            return INTERNAL_ERROR;
        }

        set_stringn(p_url->h_url_parts[PATH], path, len);
    }

    return SUCCESS;
}


/**
 * Because is not strict parsing of url, url scheme is able to lack a scheme,
 * also user info is deprecated but (because of some possible edge cases it does not result in error)
*/
int parse_h_url(char *url, h_url_t *p_url, char* def_scheme_part_str) {
    regex_t regexes[RE_H_URL_NUM];
    regmatch_t regmatch[RE_H_URL_NUM];
    int res[RE_H_URL_NUM], ret;

    if((ret = prepare_patterns(regexes)) != SUCCESS) { //< Preparation of POSIX regex structures
        return ret;
    }

    int glob_st = 0; //< Index from which the searching begins
    bool is_invalid = false, int_err_occ = false;
    for(int i = 0; i < RE_H_URL_NUM && !is_invalid && !int_err_occ; i++) {
        res[i] = regexec(&regexes[i], &(url[glob_st]), 1, &(regmatch[i]), 0); //< Perform searching in the rest of the URL

        if(res[i] == 0) { //< Pattern was found in the rest of URL
            int st = regmatch[i].rm_so, end = regmatch[i].rm_eo;
            if(st != 0) { //< Next URL part is not immediate succesor of previous part
                is_invalid = true;
                break;
            }

            int len = end - st;
            if(!(p_url->h_url_parts[i] = new_string(len + 1))) {
                int_err_occ = true;
                break;
            }

            set_stringn(p_url->h_url_parts[i], &(url[glob_st + st]), len); //< Save matched part of the string
            glob_st += regmatch[i].rm_eo;
        }
    }

    free_all_patterns(regexes);

    #ifdef DEBUG
    fprintf(stderr, "i\tres\tstart\tend\n");
    for(int i = 0; i < RE_H_URL_NUM; i++) {
        fprintf(stderr, "%d\t%d\t", i, res[i]);
        if(res[i] == 0) fprintf(stderr, "%d\t%d\t%s", regmatch[i].rm_so, regmatch[i].rm_eo, p_url->h_url_parts[i]->str);
        fprintf(stderr, "\n");
    }
    #endif

    if((ret = res_url(is_invalid, int_err_occ, res, p_url, url)) != SUCCESS) { //< Resolving parsing results
        return ret;
    }
    if((ret = fix_url(p_url, def_scheme_part_str)) != SUCCESS) { //< Perform auto fixes of URL
        return ret;
    }

    return SUCCESS;
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


void set_stringn(string_t *dest, char *src, size_t n) {
    memset(dest->str, 0, dest->len);

    strncpy(dest->str, src, n);
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