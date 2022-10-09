/**
 * @file utils.c
 * @brief Src file of module with definitions of functions, that are used 
 * across the project
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
        "Invalid URL",
        "Connection error",
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
    //End of part base on RFC3986

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

/**
 * Because is not strict parsing of url, url scheme is able to lack a scheme,
 * also user info is deprecated but it is only ignored (because of some 
 * possible edge cases it does not result in error)
 */

//TODO (RFC1123)
int res_url(bool is_inv, bool int_err, int *res, h_url_t *p_url, char *url) {
    if(int_err) {
        printerr(INTERNAL_ERROR, "Error while parsing URL!");
        return INTERNAL_ERROR;
    }

    if(res[HOST] == REG_NOMATCH || is_inv) { //< Host was not found in URL
        printerr(INVALID_URL, "Bad format of URL '%s'!", url);
        return INVALID_URL;
    }

    if(res[SCHEME_PART] == REG_NOMATCH) { //< Non-strict parsing
        printw("Scheme part of URL '%s' was not found! It will be set to default!", url);
    }

    if(res[USER_INFO_PART] != REG_NOMATCH) {
        printw("Deprecated userinfo part '%s' was found in URL '%s'! It will be ignored!", p_url->h_url_parts[USER_INFO_PART]->str, url);
    }

    return SUCCESS;
}

int normalize_url(h_url_t *p_url, char* def_scheme_part_str) {
    string_t **url_parts = p_url->h_url_parts;

    if(!p_url->h_url_parts[SCHEME_PART]) { //< If scheme is missing, it does not necessary mean error, it is just set to default scheme 
        size_t len = strlen(def_scheme_part_str);

        p_url->h_url_parts[SCHEME_PART] = new_string(len + 1);
        if(!p_url->h_url_parts[SCHEME_PART]) {
            return INTERNAL_ERROR;
        }

        set_stringn(url_parts[SCHEME_PART], def_scheme_part_str, len);
    }

    if(!url_parts[PORT_PART]) { //< If port number was not set, set it by scheme sign (then must be determined default port number for given service)
        size_t len = strlen(url_parts[SCHEME_PART]->str);

        url_parts[PORT_PART] = new_string(len + 1);
        if(!url_parts[PORT_PART]) {
            return INTERNAL_ERROR;
        }

        set_stringn(url_parts[PORT_PART], url_parts[SCHEME_PART]->str, len);
        trunc_string(url_parts[PORT_PART], -((int)strlen("://"))); //< Remove :// from end
    }
    else {
        trunc_string(url_parts[PORT_PART], ((int)strlen(":"))); //< Remove : from start
    }

    if(!url_parts[PATH]) { //< If path is not set (which is quite often situation), it is set to default path "/"
        size_t len = strlen("/");

        url_parts[PATH] = new_string(len + 1);
        if(!url_parts[PATH]) {
            return INTERNAL_ERROR;
        }

        set_stringn(url_parts[PATH], "/", len);
    }

    #ifdef DEBUG //Prints all parts of normalized url
        fprintf(stderr, "Normalized url parts\ni\tstr\n");
        for(int i = 0; i < RE_H_URL_NUM; i++) {
            fprintf(stderr, "%d\t%s\n", i, p_url->h_url_parts[i] ? p_url->h_url_parts[i]->str : NULL);
        }
        fprintf(stderr, "\n");
    #endif
 
    return SUCCESS;
}


void init_res_arr(int *res) {
    for(int i = 0; i < RE_H_URL_NUM; i++) {
        res[i] = REG_NOMATCH;
    }
}


int parse_h_url(char *url, h_url_t *p_url, char* def_scheme_part_str) {
    regex_t regexes[RE_H_URL_NUM];
    regmatch_t regmatch[RE_H_URL_NUM];
    int res[RE_H_URL_NUM], ret;

    if((ret = prepare_patterns(regexes)) != SUCCESS) { //< Preparation of POSIX regex structures
        return ret;
    }

    init_res_arr(res);
    size_t glob_st = 0; //< Index from which the searching begins
    bool is_invalid = false, int_err_occ = false;
    for(int i = 0; i < RE_H_URL_NUM && !is_invalid && !int_err_occ; i++) {
        res[i] = regexec(&regexes[i], &(url[glob_st]), 1, &(regmatch[i]), 0); //< Perform searching in the rest of the URL

        if(res[i] == 0) { //< Pattern was found in the rest of URL
            int st = regmatch[i].rm_so, end = regmatch[i].rm_eo;
            if(st != 0) { //< Next URL part is not immediate succesor of previous part
                res[i] = REG_NOMATCH;
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
    if(glob_st != strlen(url)) { //< There is the rest of URL, that was not parsed
        is_invalid = true;
    }

    free_all_patterns(regexes);

    #ifdef DEBUG //Prints the results of parsing
        //fprintf(stderr, "Strlen(url): %ld End of parsed part: %ld\n", strlen(url), glob_st);
        fprintf(stderr, "i\tres\tstart\tend\n");
        for(int i = 0; i < RE_H_URL_NUM; i++) {
            fprintf(stderr, "%d\t%d\t", i, res[i]);
            if(res[i] == 0) fprintf(stderr, "%d\t%d\t%s", regmatch[i].rm_so, regmatch[i].rm_eo, p_url->h_url_parts[i]->str);
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    #endif

    if((ret = res_url(is_invalid, int_err_occ, res, p_url, url)) != SUCCESS) { //< Resolving parsing results
        return ret;
    }
    if((ret = normalize_url(p_url, def_scheme_part_str)) != SUCCESS) { //< Perform auto fixes of URL
        return ret;
    }

    return SUCCESS;
}


char *new_str(size_t size) {
    char *str = (char *)malloc(sizeof(char)*size);
    
    if(str) { 
        memset(str, 0, size); //< Initialization of allocated memory 
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


string_t *ext_string(string_t *string) {
    const int coef = 2;

    char *tmp = string->str;
    string->str = new_str(string->size*coef);
    if(!string->str) {
        free(tmp);
        return NULL;
    }

    set_string(string, tmp);
    free(tmp);

    string->size *= coef;

    return string;
} 


void erase_string(string_t *string) {
    memset(string->str, 0, string->size);
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


string_t *app_char(string_t *dest, char c) {
    if(strlen(dest->str) >= dest->size - 1) {
        if(!ext_string(dest)) {
            return NULL;
        }    
    }

    dest->str[strlen(dest->str)] = c;

    return dest;
}


string_t *set_string(string_t *dest, char *src) {
    memset(dest->str, 0, dest->size);

    while(dest->size < strlen(src)) {
        if(!ext_string(dest)) {
            return NULL;
        }
    }

    strncpy(dest->str, src, dest->size);

    return dest;
}


void set_stringn(string_t *dest, char *src, size_t n) {
    memset(dest->str, 0, dest->size);

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