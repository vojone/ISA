/**
 * @file url.c
 * @brief Source file of url module
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 23. 10. 2022
 */


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


/**
 * @brief Initialization of array with result values for parsing URL
 */
void init_url_res_arr(int *res) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        res[i] = REG_NOMATCH;
    }
}


/**
 * @brief Deallocation of regex structures
 */
void free_all_url_patterns(regex_t *regexes) {
    for(int i = 0; i < RE_URL_NUM; i++) {
        regfree(&(regexes[i]));
    }
}


/**
 * @brief Checks whether string contains only numbers
 */
bool is_numeric_str(char *str) {
    size_t i = 0;
    for(; str[i] && (str[i] >= '0' && str[i] <= '9'); i++);

    if(i == strlen(str) - 1) {
        return true;
    }
    else {
        return false;
    }
}


/**
 * @brief Removes the last segment of path (file name) from path 
 */
void rem_file_from_path(char *path) {
    int i = strlen(path) - 1, cnt = 0;
    while(path[i] != '/') {
        i--;
        cnt++;
    }

    cnt++;

    trunc_str(path, -cnt);
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
        if(!is_empty(url.url_parts[i])) {
            if(i == PORT_PART && !is_numeric_str(url.url_parts[i]->str)) {
                continue;
            }
            else if(i == PORT_PART) {
                if(!app_string(&new_url, ":")) {
                    url_dtor(&url);
                    return NULL;
                }
            }

            if(!app_string(&new_url, url.url_parts[i]->str)) {
                url_dtor(&url);
                return NULL;
            }
        }
    }

    if(path->str[0] != '/') { //< Path is relative
        if(!is_empty(url.url_parts[PATH])) {
            if(!app_string(&new_url, url.url_parts[PATH]->str)) {
                url_dtor(&url);
                return NULL;
            }
        }
        rem_file_from_path(new_url->str);
    }

    if(!app_string(&new_url, path->str)) { //< Append new path
        url_dtor(&url);
        return NULL;
    }

    url_dtor(&url);

    return new_url;
}


int is_path(bool *result, char *str) {
    regex_t path_re[PATH_RE_NUM + 1];
    regmatch_t matches[PATH_RE_NUM + 1];

    char *patterns[PATH_RE_NUM + 1] = {
        "^" SCHEME,
        "^" PATH_ABS "$",
        "^" PATH_NO_SCH "$",
        "^" PATH_ROOTLESS "$",
    };

    for(int i = 0; i < PATH_RE_NUM + 1; i++) {
        if(regcomp(&(path_re[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Chyba pro kompilaci regularniho vyrazu! %d", i);
            return INTERNAL_ERROR;
        }
    }

    int res;
    *result = false;
    for(int i = 0; i < PATH_RE_NUM + 1; i++) {
        res = regexec(&path_re[i], str, 1, &(matches[i]), 0);
        
        if(i == 0 && res == 0) {
            break;
        }
        else if(res == 0) {
            *result = true;
            break;
        }
    }

    for(int i = 0; i < PATH_RE_NUM + 1; i++) {
        regfree(&(path_re[i]));
    }

    return SUCCESS;
}


//Converts scheme to enum values (it is more program-friendly format)
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
        "^" SCHEME,
        "^((" UNRESERVED "|" SUBDELIMS "|:|(" PCTENCODED "))+@)",
        "^((" IPV4ADDRESS ")|(\\[" IPV6ADDRESS "\\])|(" REGNAME "))",
        "^(:[0-9]*)",
        "^" PATH_ABS, //< Case in which is path empty is handled in normalize URL function
        "^\\?(" P_CHAR "|[:@/?]|[^\\s#?/])*",
        "^#(" P_CHAR "|[:@/?]|[^\\s#?/])*"
    };
    //End of part base on RFC3986

    for(int i = 0; i < RE_URL_NUM; i++) {
        if(regcomp(&(regexes[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Chyba pro kompilaci regularniho vyrazu pro URL! %d", i);
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

int res_url(bool is_inv, bool int_err, int *res, url_t *p_url, char *url) {
    if(int_err) {
        printerr(INTERNAL_ERROR, "Chyba pri analyze URL!");
        return INTERNAL_ERROR;
    }

    if(res[SCHEME_PART] == REG_NOMATCH) { //< Non-strict parsing
        printw("Nebylo mozne najit platne schema URL v '%s'! URL bude automaticky doplnena o vychozi schema ('%s')!", url, DEFAULT_URL_SCHEME);
        p_url->type = get_src_type(NULL);
    }
    else if(!is_empty(p_url->url_parts[SCHEME_PART])) { //< Check whether is scheme supported
        p_url->type = get_src_type(p_url->url_parts[SCHEME_PART]->str);
        if(p_url->type == UNKNOWN) {
            printerr(URL_ERROR, "Nepodporovane schema '%s' adresy '%s'!", p_url->url_parts[SCHEME_PART]->str, url);
            return URL_ERROR;
        }
    }

    if(p_url->type == FILE_SRC) {
        if(res[PATH] == REG_NOMATCH || is_inv) { //< Host was not found in URL
            printerr(URL_ERROR, "Spatny format URL adresy '%s'!", url);
            return URL_ERROR;
        }
    }
    else {
        if(res[HOST] == REG_NOMATCH || is_inv) { //< Host was not found in URL
            printerr(URL_ERROR, "Spatny format URL adresy '%s'!", url);
            return URL_ERROR;
        }

        if(res[USER_INFO_PART] != REG_NOMATCH) {
            printw("Zastarala autentizacni cast '%s' byla nalezena v '%s'! Bude ignorovana!", p_url->url_parts[USER_INFO_PART]->str, url);
        }
    }

    return SUCCESS;
}


int ins_perc_encoded(string_t **dst, size_t index, char c) {
    char tmp[4];

    snprintf(tmp, 4, "%hhX", c);

    rm_char(*dst, index);
    
    if(!ins_char(dst, index, '%') || 
        !ins_char(dst, index + 1, tmp[0]) || 
        !ins_char(dst, index + 2, tmp[1])) {
        printerr(INTERNAL_ERROR, "Nepodarilo se provest zakodovani znaku!");
        return INTERNAL_ERROR;
    }

    return SUCCESS;
}


/**
 * @brief Performs percent encoding of input (preserves characters/sequences allowed by pattern) 
 * @note Tested mainly with UTF-8 (all inputs should be firstly converted to UTF-8 due to RFC3986)
 */
int perc_enc(string_t **src, const char *pattern) {
    regex_t reg;
    regmatch_t match;
    int ret;

    if(regcomp(&reg, pattern, REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
        printerr(INTERNAL_ERROR, "Neplatna kompilace regularniho vyrazu! %d");
        return INTERNAL_ERROR;
    }

    size_t match_end = 0;
    while(match_end < strlen((*src)->str)) { //< If there are any unallowed characters (regex deos not match the entire string)
        int res = regexec(&reg, (*src)->str, 1, &match, 0);
        if(res == REG_NOMATCH) {
            return SUCCESS;
        }
        else {
            match_end = match.rm_eo;
            if(match_end < strlen((*src)->str)) {
                char cur_char = (*src)->str[match_end];
                if((ret = ins_perc_encoded(src, match_end, cur_char)) != SUCCESS) {
                    return ret;
                }
                
                if((cur_char & 0xC0) == 0xC0) { //< UTF-8 multibyte
                    for(int i = 3; ((cur_char = (*src)->str[match_end + i]) & 0xC0) == 0x80; i++) {
                        if((ret = ins_perc_encoded(src, match_end + i, cur_char)) != SUCCESS) {
                            return ret;
                        }
                        i+=2;
                    }    
                }
            }
        }
    }

    regfree(&reg);

    return SUCCESS;
}


/**
 * @brief Performs normalization of URL (addition of missing scheme, path, percent encoding...) 
 */
int normalize_url(url_t *p_url, char* def_scheme_part_str) {
    string_t **url_parts = p_url->url_parts, **dst;
    int ret;

    if(p_url->type == FILE_SRC) { //< If it is file there is no need to normalization
        return SUCCESS;
    }

    if(is_empty(url_parts[SCHEME_PART])) { //< If scheme is missing, it does not necessary mean error, it is just set to default scheme 
        dst = &(p_url->url_parts[SCHEME_PART]);

        url_parts[SCHEME_PART] = set_string(dst, def_scheme_part_str);
        if(!p_url->url_parts[SCHEME_PART]) {
            printerr(INTERNAL_ERROR, "Nepodrailo se alokovat pamet pro schema URL!");
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
            printerr(INTERNAL_ERROR, "Nepodarilo se alokovat pamet pro portove cislo!");
            return INTERNAL_ERROR;
        }

        trunc_string(url_parts[PORT_PART], -((int)strlen("://"))); //< Remove :// from the end
    }

    string_to_lower(url_parts[PORT_PART]);

    if(is_empty(url_parts[PATH])) { //< If path is not set (which is quite often situation), it is set to default path "/"
        dst = &(url_parts[PATH]);

        url_parts[PATH] = set_string(dst, "/");
        if(!url_parts[PATH]) {
            printerr(INTERNAL_ERROR, "Nepodarilo se alokovat pamet pro cestu!");
            return INTERNAL_ERROR;
        }
    }
    else {
        if((ret = perc_enc(&(url_parts[PATH]), PATH_ABS_STRICT)) != SUCCESS) { //< Due to RFCs - all characters that are not explicitly allowed should be percent encoded
            return ret;
        }
    }

    if(!is_empty(url_parts[QUERY])) {
        if((ret = perc_enc(&(url_parts[QUERY]), QUERY_STRICT)) != SUCCESS) {
            return ret;
        }
    }

    if(!is_empty(url_parts[FRAG_PART])) {
        if((ret = perc_enc(&(url_parts[FRAG_PART]), FRAG_STRICT)) != SUCCESS) {
            return ret;
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
            if(res[i] == 0) fprintf(stderr, "%ld\t%ld\t%s", (size_t)regmatch[i].rm_so, (size_t)regmatch[i].rm_eo, p_url->url_parts[i]->str);
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
