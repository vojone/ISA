
#include "url.h"


void init_url(url_t *url) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        url->url_parts[i] = NULL;
    }

    url->type = UNKNOWN;
}


void erase_url(url_t *url) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        erase_string(url->url_parts[i]);
    }
}


void url_dtor(url_t *url) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        if(url->url_parts[i]) {
            string_dtor(url->url_parts[i]);
        }
    }
}


void init_url_res_arr(int *res) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        res[i] = REG_NOMATCH;
    }
}


void free_all_url_patterns(regex_t *regexes) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        regfree(&(regexes[i]));
    }
}


string_t *replace_path(string_t *orig_url, string_t *path) {
    url_t url;
    init_url(&url);

    string_t *new_url = new_string(INIT_STRING_SIZE);
    if(!new_url) {
        return NULL;
    }

    int ret = parse_url(orig_url->str, &url);
    if(ret != SUCCESS) { //< We ignore return codes because orig url should be already checked
        return NULL;
    }

    for(int i = 0; i < PATH; i++) {
        if(!app_string(&new_url, url.url_parts[i]->str)) {
            url_dtor(&url);
            return NULL;
        }
    }

    if(path->str[0] != '/') { //< Path is relative
        if(!app_string(&new_url, url.url_parts[PATH]->str)) {
            url_dtor(&url);
            return NULL;
        }
    }

    if(!app_string(&new_url, path->str)) { //< Append new path
        url_dtor(&url);
        return NULL;
    }

    url_dtor(&url);

    return new_url;
}


int is_path(bool *result, char *str) {
    regex_t path_re[PATH_RE_NUM];
    regmatch_t matches[PATH_RE_NUM];

    char *patterns[PATH_RE_NUM] = {
        "^" PATH_ABS,
        "^" PATH_NO_SCH,
        "^" PATH_ROOTLESS,
    };

    for(int i = 0; i < PATH_RE_NUM; i++) {
        if(regcomp(&(path_re[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Invalid compilation of path regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    int res;
    for(int i = 0; i < PATH_RE_NUM; i++) {
        res = regexec(&path_re[i], str, 1, &(matches[i]), 0);
        if(res == 0) {
            *result = true;
            break;
        }
    }

    *result = false;

    for(int i = 0; i < PATH_RE_NUM; i++) {
        regfree(&(path_re[i]));
    }

    return SUCCESS;
}


int get_src_type(char *scheme) {
    if(scheme == NULL) {
        scheme = DEFAULT_URL_SCHEME;
    }

    if(!strcasecmp(scheme, "file://")) {
        return FILE_SRC;
    }
    else if(!strcasecmp(scheme, "https://")) {
        return HTTPS_SRC;
    }
    else if(!strcasecmp(scheme, "http://")) {
        return HTTP_SRC;
    }
    else {
        return UNKNOWN;
    }
}


int prepare_url_patterns(regex_t *regexes) {
    //Patterns are based on RFC3986
    //http-URI = "http" "://" authority path-abempty [ "?" query ] ("#" [fragment]) (see RFC9110)
    char *patterns[RE_URL_NUM] = {
        "^[a-z][a-z0-9+\\-\\.]*://",
        "^((" UNRESERVED "|" SUBDELIMS "|:|(" PCTENCODED "))+@)",
        "^((" IPV4ADDRESS ")|(\\[" IPV6ADDRESS "\\])|(" REGNAME "))",
        "^(:[0-9]*)",
        "^" PATH_ABS, //< Case in which is path empty is handled in normalize URL function
        "^\\?(" UNRESERVED "|" SUBDELIMS "|[:@/?]|(" PCTENCODED "))*",
        "^#(" UNRESERVED "|" SUBDELIMS "|[:@/?]|(" PCTENCODED "))*"
    };
    //End of part base on RFC3986

    for(int i = 0; i < RE_URL_NUM; i++) {
        if(regcomp(&(regexes[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Invalid compilation of URL regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    return SUCCESS;
}


/**
 * Because is not strict parsing of url, url scheme is able to lack a scheme,
 * also user info is deprecated but it is only ignored (because of some 
 * possible edge cases it does not result in error)
 */

//TODO (RFC1123)
int res_url(bool is_inv, bool int_err, int *res, url_t *p_url, char *url) {
    if(int_err) {
        printerr(INTERNAL_ERROR, "Error while parsing URL!");
        return INTERNAL_ERROR;
    }

    if(res[SCHEME_PART] == REG_NOMATCH) { //< Non-strict parsing
        printw("Valid scheme part of URL '%s' was not found! It will be set to default ('%s')!", url, DEFAULT_URL_SCHEME);
        p_url->type = get_src_type(NULL);
    }
    else if(!is_empty(p_url->url_parts[SCHEME_PART])) { //< Check whether is scheme supported
        p_url->type = get_src_type(p_url->url_parts[SCHEME_PART]->str);
        if(p_url->type == UNKNOWN) {
            printerr(URL_ERROR, "Unsupported scheme '%s' of URL '%s'!", p_url->url_parts[SCHEME_PART]->str, url);
            return URL_ERROR;
        }
    }

    if(p_url->type == FILE_SRC) {
        if(res[PATH] == REG_NOMATCH || is_inv) { //< Host was not found in URL
            printerr(URL_ERROR, "Bad format of URL '%s'!", url);
            return URL_ERROR;
        }
    }
    else {
        if(res[HOST] == REG_NOMATCH || is_inv) { //< Host was not found in URL
            printerr(URL_ERROR, "Bad format of URL '%s'!", url);
            return URL_ERROR;
        }

        if(res[USER_INFO_PART] != REG_NOMATCH) {
            printw("Deprecated userinfo part '%s' was found in URL '%s'! It will be ignored!", p_url->url_parts[USER_INFO_PART]->str, url);
        }
    }

    return SUCCESS;
}


int normalize_url(url_t *p_url, char* def_scheme_part_str) {
    string_t **url_parts = p_url->url_parts, **dst;

    if(p_url->type == FILE_SRC) { //< If it is file there is no need to normalization
        return SUCCESS;
    }

    if(is_empty(url_parts[SCHEME_PART])) { //< If scheme is missing, it does not necessary mean error, it is just set to default scheme 
        dst = &(p_url->url_parts[SCHEME_PART]);

        url_parts[SCHEME_PART] = set_string(dst, def_scheme_part_str);
        if(!p_url->url_parts[SCHEME_PART]) {
            printerr(INTERNAL_ERROR, "Unable to allocate buffer for scheme part of URL!");
            return INTERNAL_ERROR;
        }
    }

    if(!is_empty(url_parts[PORT_PART])) {
        trunc_string(url_parts[PORT_PART], ((int)strlen(":"))); //< Remove : from start
    }

    if(is_empty(url_parts[PORT_PART]) || !strlen(url_parts[PORT_PART]->str)) { //< If port number was not set, set it by scheme sign (then must be determined default port number for given service)
        dst = &(url_parts[PORT_PART]);

        url_parts[PORT_PART] = set_string(dst, url_parts[SCHEME_PART]->str);
        if(!url_parts[PORT_PART]) {
            printerr(INTERNAL_ERROR, "Unable to allocate buffer for port part of URL!");
            return INTERNAL_ERROR;
        }

        trunc_string(url_parts[PORT_PART], -((int)strlen("://"))); //< Remove :// from the end
    }

    string_to_lower(url_parts[PORT_PART]);

    if(is_empty(url_parts[PATH])) { //< If path is not set (which is quite often situation), it is set to default path "/"
        dst = &(url_parts[PATH]);

        url_parts[PATH] = set_string(dst, "/");
        if(!url_parts[PATH]) {
            printerr(INTERNAL_ERROR, "Unable to allocate buffer for path of URL!");
            return INTERNAL_ERROR;
        }
    }

    #ifdef DEBUG //Prints all parts of normalized url
        fprintf(stderr, "Normalized url parts\ni\tstr\n");
        for(int i = 0; i < RE_URL_NUM; i++) {
            fprintf(stderr, "%d\t%s\n", i, p_url->url_parts[i] ? p_url->url_parts[i]->str : NULL);
        }
        fprintf(stderr, "\n");
    #endif
 
    return SUCCESS;
}



int parse_url(char *url, url_t *p_url) {
    regex_t regexes[RE_URL_NUM];
    regmatch_t regmatch[RE_URL_NUM];
    int res[RE_URL_NUM], ret;

    if((ret = prepare_url_patterns(regexes)) != SUCCESS) { //< Preparation of POSIX regex structures
        return ret;
    }

    init_url_res_arr(res);
    size_t glob_st = 0; //< Index from which the searching begins
    bool is_invalid = false, int_err_occ = false;
    for(int i = 0; i < RE_URL_NUM && !is_invalid && !int_err_occ; i++) {
        res[i] = regexec(&regexes[i], &(url[glob_st]), 1, &(regmatch[i]), 0); //< Perform searching in the rest of the URL
        if(res[i] == 0) { //< Pattern was found in the rest of URL
            int st = regmatch[i].rm_so, end = regmatch[i].rm_eo;

            string_t **dst = &(p_url->url_parts[i]);
            size_t len = end - st;
            char *st_ptr = &(url[glob_st + st]);
            if(!(p_url->url_parts[i] = set_stringn(dst, st_ptr, len))) { //< Save matched part of the string
                int_err_occ = true;
                break;
            }
        
            glob_st += regmatch[i].rm_eo;
        }
    }
    if(glob_st != strlen(url)) { //< There is the rest of URL, that was not parsed
        is_invalid = true;
    }

    free_all_url_patterns(regexes);

    #ifdef DEBUG //Prints the results of parsing
        fprintf(stderr, "Parsed URL parts\n");
        //fprintf(stderr, "Strlen(url): %ld End of parsed part: %ld\n", strlen(url), glob_st);
        fprintf(stderr, "i\tres\tstart\tend\n");
        for(int i = 0; i < RE_URL_NUM; i++) {
            fprintf(stderr, "%d\t%d\t", i, res[i]);
            if(res[i] == 0) fprintf(stderr, "%d\t%d\t%s", regmatch[i].rm_so, regmatch[i].rm_eo, p_url->url_parts[i]->str);
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    #endif

    if((ret = res_url(is_invalid, int_err_occ, res, p_url, url)) != SUCCESS) { //< Resolving parsing results
        return ret;
    }
    if((ret = normalize_url(p_url, DEFAULT_URL_SCHEME)) != SUCCESS) { //< Perform auto fixes of URL
        return ret;
    }

    return SUCCESS;
}
