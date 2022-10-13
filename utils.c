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
        "Communication error",
        "Path error",
        "Verification error",
        "HTTP error",
        "Internal error",
    };

    fprintf(stderr, "%s: %s: ", PROGNAME, err_str[err_code]);

    if(message_format) {
        va_list args;
        va_start (args, message_format);
        vfprintf(stderr, message_format, args);
    }

    fprintf(stderr, "\n");
}


void printw(const char *message_format,...) {
    fprintf(stderr, "%s: Warning: ", PROGNAME);

    if(message_format) {
        va_list args;
        va_start(args, message_format);
        vfprintf(stderr, message_format, args);
    }

    fprintf(stderr, "\n");
}


void erase_h_url(h_url_t *h_url) {
    for(int i = 0; i < RE_H_URL_NUM; i++) {
        erase_string(h_url->h_url_parts[i]);
    }
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

void init_h_resp(h_resp_t *h_resp) {
    memset(h_resp, 0, sizeof(h_resp_t));
}


int prepare_url_patterns(regex_t *regexes) {
    //Patterns are based on RFC3986
    char *patterns[] = {
        "^(http|https)://",
        "(" UNRESERVED "|" SUBDELIMS "|:|(" PCTENCODED "))+@",
        "(" IPV4ADDRESS ")|(\\[" IPV6ADDRESS "\\])|(" REGNAME ")",
        ":[0-9]*",
        "(/(" UNRESERVED "|" SUBDELIMS "|[:@]|(" PCTENCODED "))*)+",
        "\\?(" UNRESERVED "|" SUBDELIMS "|[:@/?]|(" PCTENCODED "))*",
        "#(" UNRESERVED "|" SUBDELIMS "|[:@/?]|(" PCTENCODED "))*"
    };
    //End of part base on RFC3986

    for(int i = 0; i < RE_H_URL_NUM; i++) {
        if(regcomp(&(regexes[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Invalid compilation of URL regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    return SUCCESS;
}


void free_all_patterns(regex_t *regexes, int r_num) {
    for(int i = 0; i < r_num; i++) {
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
        printerr(URL_ERROR, "Bad format of URL '%s'!", url);
        return URL_ERROR;
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
    string_t **url_parts = p_url->h_url_parts, **dst;

    if(is_empty(url_parts[SCHEME_PART])) { //< If scheme is missing, it does not necessary mean error, it is just set to default scheme 
        dst = &(p_url->h_url_parts[SCHEME_PART]);

        url_parts[SCHEME_PART] = set_string(dst, def_scheme_part_str);
        if(!p_url->h_url_parts[SCHEME_PART]) {
            printerr(INTERNAL_ERROR, "Unable to allocate buffer for scheme part of URL!");
            return INTERNAL_ERROR;
        }
    }

    if(is_empty(url_parts[PORT_PART])) { //< If port number was not set, set it by scheme sign (then must be determined default port number for given service)
        dst = &(url_parts[PORT_PART]);

        url_parts[PORT_PART] = set_string(dst, url_parts[SCHEME_PART]->str);
        if(!url_parts[PORT_PART]) {
            printerr(INTERNAL_ERROR, "Unable to allocate buffer for port part of URL!");
            return INTERNAL_ERROR;
        }

        trunc_string(url_parts[PORT_PART], -((int)strlen("://"))); //< Remove :// from end
    }
    else {
        trunc_string(url_parts[PORT_PART], ((int)strlen(":"))); //< Remove : from start
    }

    if(is_empty(url_parts[PATH])) { //< If path is not set (which is quite often situation), it is set to default path "/"
        dst = &(url_parts[PATH]);

        url_parts[PATH] = set_string(dst, "/");
        if(!url_parts[PATH]) {
            printerr(INTERNAL_ERROR, "Unabel to allocate buffer for path of URL!");
            return INTERNAL_ERROR;
        }
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


bool is_empty(string_t *string) {
    return (!string) || string->str == NULL || string->str[0] == '\0';
}


void init_res_arr(int *res, int res_num) {
    for(int i = 0; i < res_num; i++) {
        res[i] = REG_NOMATCH;
    }
}


int parse_h_url(char *url, h_url_t *p_url, char* def_scheme_part_str) {
    regex_t regexes[RE_H_URL_NUM];
    regmatch_t regmatch[RE_H_URL_NUM];
    int res[RE_H_URL_NUM], ret;

    if((ret = prepare_url_patterns(regexes)) != SUCCESS) { //< Preparation of POSIX regex structures
        return ret;
    }

    init_res_arr(res, RE_H_URL_NUM);
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

            string_t **dst = &(p_url->h_url_parts[i]);
            size_t len = end - st;
            char *st_ptr = &(url[glob_st + st]);
            if(!(p_url->h_url_parts[i] = set_stringn(dst, st_ptr, len))) { //< Save matched part of the string
                int_err_occ = true;
                break;
            }

            glob_st += regmatch[i].rm_eo;
        }
    }
    if(glob_st != strlen(url)) { //< There is the rest of URL, that was not parsed
        is_invalid = true;
    }

    free_all_patterns(regexes, RE_H_URL_NUM);

    #ifdef DEBUG //Prints the results of parsing
        fprintf(stderr, "Parsed URL parts\n");
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


int prepare_resp_patterns(regex_t *regexes) {
    char *patterns[] = { //< There is always ^ because we always match pattern from start of the string 
        "^(.|\r\n)*\r\n\r\n",
        "^.*\r\n",
        "^[^ \t]*",
        "^[0-9]{3}",
        "^[^\r\n]*",
        "^Location:",
    };

    for(int i = 0; i < RE_H_RESP_NUM; i++) {
        if(regcomp(&(regexes[i]), patterns[i], REG_EXTENDED | REG_NEWLINE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Invalid compilation of response regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    return SUCCESS;
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


int parse_first_line(char *cursor, regex_t *r, char *url, h_resp_t *p_resp) {
    int res[RE_H_RESP_NUM];
    regmatch_t regmatch[RE_H_RESP_NUM];
    init_res_arr(res, RE_H_RESP_NUM);

    res[VER] = regexec(&(r[VER]), cursor, 1, &(regmatch[VER]), 0);
    if(res[VER] != REG_NOMATCH) {
        p_resp->version = new_str_slice(cursor, regmatch[VER].rm_eo - regmatch[VER].rm_so);
        cursor = &(cursor[regmatch[VER].rm_eo]);
    }

    cursor = skip_w_spaces(cursor);
    res[STAT] = regexec(&(r[STAT]), cursor, 1, &(regmatch[STAT]), 0);
    if(res[STAT] == REG_NOMATCH) {
        printerr(HTTP_ERROR, "Unable to find status code in reponse from '%s'!", cursor);
        return HTTP_ERROR;
    }

    p_resp->status = new_str_slice(cursor, regmatch[STAT].rm_eo - regmatch[STAT].rm_so);
    cursor = &(cursor[regmatch[STAT].rm_eo]);

    cursor = skip_w_spaces(cursor);
    res[PHR] = regexec(&(r[PHR]), cursor, 1, &(regmatch[PHR]), 0);
    if(res[PHR] == REG_NOMATCH) {
        printerr(HTTP_ERROR, "Unable to find status phrase in reponse from '%s'!", url);
        return HTTP_ERROR;
    }

    p_resp->phrase = new_str_slice(cursor, regmatch[PHR].rm_eo - regmatch[PHR].rm_so);
    cursor = &(cursor[regmatch[PHR].rm_eo]);

    return SUCCESS;
}


bool is_line_empty(char *line_start_ptr) {
    return !strncmp(line_start_ptr, "\r\n", strlen("\r\n"));
}   


int parse_resp_headers(char *h_st, regex_t *r, char *url, h_resp_t *p_resp) {
    size_t line_no = 0;

    int res[RE_H_RESP_NUM], ret = SUCCESS;
    regmatch_t regm[RE_H_RESP_NUM];
    init_res_arr(res, RE_H_RESP_NUM);

    char *cursor = h_st, *line_st;
    res[LINE] = regexec(&(r[LINE]), cursor, 1, &(regm[LINE]), 0);
    cursor = line_st = &(cursor[regm[LINE].rm_so]);

    for(; (res[LINE] != REG_NOMATCH) && !is_line_empty(cursor); line_no++) {
        if(line_no == 0) {
            if((ret = parse_first_line(cursor, r, url, p_resp)) != SUCCESS) {
                break;
            }
        }
        else {
            res[LOC] = regexec(&(r[LOC]), cursor, 1, &(regm[LOC]), 0);

            if(res[LOC] != REG_NOMATCH) {
                cursor =  skip_w_spaces(&(cursor[regm[LOC].rm_eo]));
                size_t len = (size_t)(&line_st[regm[LINE].rm_eo] - cursor);
                p_resp->location = new_str_slice(cursor, len - strlen("\r\n"));
            }   
        }

        cursor = &(line_st[regm[LINE].rm_eo]);
        res[LINE] = regexec(&(r[LINE]), cursor, 1, &(regm[LINE]), 0);
        cursor = line_st = &(cursor[regm[LINE].rm_so]);
    }

    if(line_no == 0 && ret == SUCCESS) {
        printerr(HTTP_ERROR, "Invalid headers of HTTP response from '%s' (missing initial header)!", url);
        ret = HTTP_ERROR;
    }

    return ret;
}


int parse_http_resp(h_resp_t *parsed_resp, string_t *response, char *url) {
    regex_t regexes[RE_H_RESP_NUM];
    regmatch_t regmatch[RE_H_RESP_NUM];
    int res[RE_H_RESP_NUM], ret = SUCCESS;
    char *resp = response->str;

    if((ret = prepare_resp_patterns(regexes)) != SUCCESS) { //< Preparation of POSIX regex structures
        return ret;
    }

    init_res_arr(res, RE_H_RESP_NUM);

    res[H_PART] = regexec(&regexes[H_PART], resp, 1, &(regmatch[H_PART]), 0);

    if(res[H_PART] == REG_NOMATCH) {
        printerr(HTTP_ERROR, "Headers of HTTP response from '%s' was not found!", url);
        ret = HTTP_ERROR;
    }
    else {
        parsed_resp->msg = &(resp[regmatch[H_PART].rm_eo]);
        ret = parse_resp_headers(resp, regexes, url, parsed_resp);
    }

    free_all_patterns(regexes, RE_H_RESP_NUM);

    return ret;
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