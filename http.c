/**
 * @file feedreader.c
 * @brief Source file with functions related to the HTTP(s) connection of
 * feedreader program
 * @note Uses opensll library
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 15. 10. 2022
 */


#include "http.h"


//The process of initializing features of openssl library is based on https://developer.ibm.com/tutorials/l-openssl/
void openssl_init() {
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
}
//End of the part based on https://developer.ibm.com/tutorials/l-openssl/


int send_request(BIO *bio, url_t *p_url, char *url) {
    int ret, attempt_num = 0;

    struct pollfd pfd;
    pfd.fd = BIO_get_fd(bio, NULL);
    pfd.events = POLLIN;

    char request_b[INIT_NET_BUFF_SIZE];
    snprintf(request_b, INIT_NET_BUFF_SIZE, 
        "GET %s " HTTP_VERSION "\r\n"
        "Host: %s\r\n" //< Mandatory due to RFC2616
        "Connection: close\r\n" //< Connection will be closed after completition of the response
        "User-Agent: ISAFeedReader/1.0\r\n" //< Just to better filtering from the other traffic
        "\r\n",
        p_url->url_parts[PATH]->str, 
        p_url->url_parts[HOST]->str);

    #ifdef DEBUG
        fprintf(stderr, "Request:\n");
        fprintf(stderr, "%s\n", request_b);
    #endif

    while((ret = BIO_write(bio, request_b, strlen(request_b))) <= 0) {
        if(!BIO_should_retry(bio)) { //< Checking if write should be repeated (in some cases is should be repeated even without SSL due to docs)
            printerr(COMMUNICATION_ERROR, "Unable to send request to the '%s'!", url);
            return COMMUNICATION_ERROR;
        }
        else {
            ret = poll(&pfd, 1, TIMEOUT_S);
            if(ret && !(pfd.revents & POLLIN)) {
                printerr(COMMUNICATION_ERROR, "Unable to send request to the '%s'!", url);
                return COMMUNICATION_ERROR;
            }
        }

        attempt_num++;
    }

    return SUCCESS;
}


int rec_response(BIO *bio, string_t *resp_b, char *url) {
    int ret = 0;

    size_t emp_size = 0, total_b = 0;

    struct pollfd pfd;
    pfd.fd = BIO_get_fd(bio, NULL);
    pfd.events = POLLIN;

    while(ret > 0 || total_b == 0) { //< Extend buffer until there is something on the input
        emp_size = resp_b->size - total_b;
        while((ret = BIO_read(bio, &(resp_b->str[total_b]), emp_size)) <= 0) {
            if(!BIO_should_retry(bio)) { //< Checking if read should be repeated, but is BIO_should_read returns false if there is nothing to read anymore
                if(ret == 0) { //< Connection was closed
                    break;
                }
                
                printerr(COMMUNICATION_ERROR, "Unable to get response from the '%s'!", url);
                return COMMUNICATION_ERROR;
            }
            else { //< Read can be retried -> try it again (using poll)
                ret = poll(&pfd, 1, TIMEOUT_S);
                if(ret && !(pfd.revents & POLLIN)) {
                    printerr(COMMUNICATION_ERROR, "Unable to get response from the '%s'!", url);
                    return COMMUNICATION_ERROR;
                }
            }
        }

        total_b += ret;
        if(resp_b->size == total_b) { //< Response is in whole buffer => extend it and try to read again
            if(!(resp_b = ext_string(resp_b))) {
                printerr(INTERNAL_ERROR, "Error while expanding buffer for response!");
                return INTERNAL_ERROR;
            }
        }
    }

    return SUCCESS;
}


void free_https_connection(BIO *bio, SSL_CTX *ctx) {
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
}


int https_connect(url_t *p_url, string_t *resp_b, char *url, settings_t *s) {
    int ret = SUCCESS;

    //Based on IBM tutorial https://developer.ibm.com/tutorials/l-openssl/
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl;

    if(!s->certfile && !s->certaddr) {
        ret = SSL_CTX_set_default_verify_paths(ctx);
    }
    else {
        ret = SSL_CTX_load_verify_locations(ctx, s->certfile, s->certaddr); //< First is performed searching in file then in dir 
    }
   
    if(ret == 0) { //< Setting of paths was not succesful
        const char *appendix = s->certfile || s->certaddr ? "Please check given paths!" : "";
        printerr(PATH_ERROR, "Unable to set paths to certificate files! %s", appendix);
        SSL_CTX_free(ctx);
        return PATH_ERROR;
    }

    BIO *bio = BIO_new_ssl_connect(ctx);
    BIO_get_ssl(bio, &ssl);
    if(!ssl) {
        printerr(INTERNAL_ERROR, "Unable to allocate SSL!");
        SSL_CTX_free(ctx);
        return INTERNAL_ERROR;
    }

    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY); //< Set ssl to auto retry to prevent errors caused by non-application data
    //End of code based on https://developer.ibm.com/tutorials/l-openssl/

    if(!SSL_set_tlsext_host_name(ssl, p_url->url_parts[HOST]->str)) { //< Set Server Name Indication (if it is missing, self signed certificate error can occur)
        printerr(INTERNAL_ERROR, "Unable to set SNI!");
        free_https_connection(bio, ctx);
        return INTERNAL_ERROR;
    }

    BIO_set_conn_hostname(bio, p_url->url_parts[HOST]->str); //< Always returns 1 -> no need to check retval
    BIO_set_conn_port(bio, p_url->url_parts[PORT_PART]->str); //< -||-

    if(BIO_do_connect(bio) <= 0) { //< Perform handshake
        printerr(CONNECTION_ERROR, "Cannot connect to the '%s'!", url);
        free_https_connection(bio, ctx);
        return CONNECTION_ERROR;
    }

    if(SSL_get_verify_result(ssl) != X509_V_OK) { //< Check verify result
        printerr(VERIFICATION_ERROR, "Unable to verify certificate of '%s'! (%s)", url, X509_verify_cert_error_string(ret));
        free_https_connection(bio, ctx);
        return VERIFICATION_ERROR;
    }
    
    if((ret = send_request(bio, p_url, url)) != SUCCESS) { //< Send request to the server
        free_https_connection(bio, ctx);
        return ret;
    }

    if((ret = rec_response(bio, resp_b, url)) != SUCCESS) { //< Receive response
        free_https_connection(bio, ctx);
        return ret;
    }
    
    #ifdef DEBUG
        fprintf(stderr, "Response (%ld):\n%s\n", strlen(resp_b->str), resp_b->str);
    #endif

    free_https_connection(bio, ctx);
    return SUCCESS;
}


int http_connect(url_t *parsed_url, string_t *resp_b, char *url) {
    int ret;
    BIO *bio = BIO_new(BIO_s_connect());
    if(!bio) {
        printerr(INTERNAL_ERROR, "Unable to allocate BIO!");
        return INTERNAL_ERROR;
    }

    BIO_set_conn_hostname(bio, parsed_url->url_parts[HOST]->str); //< Always returns 1 -> no need to check retval
    BIO_set_conn_port(bio, parsed_url->url_parts[PORT_PART]->str); //< -||-

    if(BIO_do_connect(bio) <= 0) {
        printerr(CONNECTION_ERROR, "Cannot connect to the '%s'!", url);
        BIO_free_all(bio);
        return CONNECTION_ERROR;
    }
    
    if((ret = send_request(bio, parsed_url, url)) != SUCCESS) {
        BIO_free_all(bio);
        return ret;
    }

    if((ret = rec_response(bio, resp_b, url)) != SUCCESS) {
        BIO_free_all(bio);
        return ret;
    }

    #ifdef DEBUG
        fprintf(stderr, "Response (%ld):\n%s\n", strlen(resp_b->str), resp_b->str);
    #endif

    BIO_free_all(bio);

    return SUCCESS;
}


int check_http_status(int status_c, string_t *phrase, char *url) {
    switch(status_c/100) {
        case 2:
            return SUCCESS;
        case 3:
            printw("Got %s (code %d) from '%s'! Redirecting...", phrase->str, status_c, url);
            return HTTP_REDIRECT;
        default:
            printerr(HTTP_ERROR, "Got %s (code %d) from '%s'! expected OK (200)", phrase->str, status_c, url);
            return HTTP_ERROR;
    }
}


int http_redirect(h_resp_t *p_resp, list_el_t *cur_url) {
    if(cur_url->indirect_lvl >= MAX_REDIR_NUM) {
        printerr(HTTP_ERROR, "Maximum number of redirections (%d) was exceeded!", MAX_REDIR_NUM);
        return HTTP_ERROR;
    }
    else if(p_resp->location.st != NULL) {
        size_t new_lvl = cur_url->indirect_lvl + 1;
        list_el_t *new_url = slice2element(&(p_resp->location), new_lvl);
        if(!new_url) {
            printerr(INTERNAL_ERROR, "Unable to allocate memory for new URL!");
            return INTERNAL_ERROR;
        }

        new_url->next = cur_url->next;
        cur_url->next = new_url;
    }
    else {
        printerr(HTTP_ERROR, "Unable to redirect, because Location header was not found!");
        return HTTP_ERROR;
    }

    return SUCCESS;
}


void init_h_resp(h_resp_t *h_resp) {
    memset(h_resp, 0, sizeof(h_resp_t));
}


void erase_h_resp(h_resp_t *h_resp) {
    init_h_resp(h_resp);
}


void init_resp_res_arr(int *res) {
    for(int i = 0; i < RE_H_RESP_NUM; i++) {
        res[i] = REG_NOMATCH;
    }
}


void free_all_patterns(regex_t *regexes, int r_num) {
    for(int i = 0; i < r_num; i++) {
        regfree(&(regexes[i]));
    }
}


int prepare_mime_patterns(regex_t *regexes) {
    char *patterns[MIME_NUM] = {
        "^" RSS_MIME, 
        "^" ATOM_MIME, 
        "^" XML_MIME,
    };

    for(int i = 0; i < MIME_NUM; i++) {
        if(regcomp(&(regexes[i]), patterns[i], REG_EXTENDED | REG_ICASE)) { //< We will need extended posix notation and case insensitive matching 
            printerr(INTERNAL_ERROR, "Invalid compilation of MIME regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    return SUCCESS;
}


int find_mime(h_resp_t *p_resp, char *url) {
    regex_t re[MIME_NUM];
    regmatch_t match[MIME_NUM];

    int ret;

    if((ret = prepare_mime_patterns(re)) != SUCCESS) {
        return ret;
    }

    string_t *content_type = slice_to_string(&(p_resp->content_type));
    if(!content_type) {
        printerr(INTERNAL_ERROR, "Unable to allocate temporary buffer for MIME type!");
        return INTERNAL_ERROR;
    }

    ret = HTTP_ERROR;
    char *con_type_str = content_type->str;
    for(int i = 0; i < MIME_NUM; i++) {
        int res = regexec(&(re[i]), con_type_str, 1, &(match[i]), 0);
        if(res == 0) {
            p_resp->doc_type = i; //< Set mime type
            ret = SUCCESS;
            break;
        }
    }

    if(ret != SUCCESS) {
        printerr(HTTP_ERROR, "MIME type '%s' of document from '%s' is not supported!", content_type->str, url);
    }

    free_all_patterns(re, MIME_NUM);
    string_dtor(content_type);

    return ret;
}



int check_http_resp(h_resp_t *p_resp, list_el_t *cur_url, char *url) {
    string_t *status = slice_to_string(&(p_resp->status));
    if(!status) {
        printerr(INTERNAL_ERROR, "Unable to allocate buffer for HTTP status code!");
        return INTERNAL_ERROR;
    }

    string_t *phrase = slice_to_string(&(p_resp->phrase));
    if(!status) {
        printerr(INTERNAL_ERROR, "Unable to allocate buffer for HTTP phrase!");
        return INTERNAL_ERROR;
    }

    char *rest = NULL;
    int status_c = strtoul(status->str, &rest, 10);
    int ret = check_http_status(status_c, phrase, url);

    string_dtor(phrase);
    string_dtor(status);

    if(ret == HTTP_REDIRECT) {
        ret = http_redirect(p_resp, cur_url);
        if(ret == SUCCESS) {
            return HTTP_REDIRECT;
        }
    }
    else if(ret == SUCCESS) {

        #ifdef CHECK_MIME_TYPE
            if(p_resp->content_type.st) {
                ret = find_mime(p_resp, url);
            }
        #endif
    }

    return ret;
}


int prepare_resp_patterns(regex_t *regexes) {
    char *patterns[RE_H_RESP_NUM] = { //< There is always ^ because we always match pattern from start of the string 
        "^(.|\r\n)*\r\n\r\n",
        "^.*\r\n",
        "^[^ \t]*",
        "^[0-9]{3}",
        "^[^\r\n]*",
        "^Location:",
        "^Content-Type:",
        "^Content-Length:"
    };

    for(int i = 0; i < RE_H_RESP_NUM; i++) {
        int comp_flags = REG_EXTENDED | REG_NEWLINE | REG_ICASE; //< We will need extended posix notation and case insensitive matching (for better robustness) 
        if(regcomp(&(regexes[i]), patterns[i], comp_flags)) {
            printerr(INTERNAL_ERROR, "Invalid compilation of response regexes! %d", i);
            return INTERNAL_ERROR;
        }
    }

    return SUCCESS;
}

typedef struct resp_parse_ctx {
    regex_t regexes[RE_H_RESP_NUM];
    char *cursor;
    regmatch_t regm[RE_H_RESP_NUM];
    int res[RE_H_RESP_NUM];
} resp_parse_ctx_t;


void parse_hdrs(char *line_end, resp_parse_ctx_t *ctx, h_resp_t *p_resp) {
    regex_t *regexes = ctx->regexes;
    regmatch_t *matches = ctx->regm;
    int *res = ctx->res;
    char **cursor = &(ctx->cursor);

    res[LOC] = regexec(&(regexes[LOC]), *cursor, 1, &(matches[LOC]), 0);
    if(res[LOC] != REG_NOMATCH) {
        *cursor =  skip_w_spaces(&((*cursor)[matches[LOC].rm_eo]));
        size_t len = (size_t)(line_end - *cursor);
        p_resp->location = new_str_slice(*cursor, len - strlen("\r\n"));
    }

    regex_t *con_type_r = &(regexes[CON_TYPE]);
    res[CON_TYPE] = regexec(con_type_r, *cursor, 1, &(matches[CON_TYPE]), 0);
    if(res[CON_TYPE] != REG_NOMATCH) {
        *cursor =  skip_w_spaces(&((*cursor)[matches[CON_TYPE].rm_eo]));
        size_t len = (size_t)(line_end - *cursor);
        p_resp->content_type = new_str_slice(*cursor, len - strlen("\r\n"));
    }

    regex_t *con_len_r = &(regexes[CON_LEN]);
    res[CON_LEN] = regexec(con_len_r, *cursor, 1, &(matches[CON_LEN]), 0);
    if(res[CON_LEN] != REG_NOMATCH) {
        *cursor =  skip_w_spaces(&((*cursor)[matches[CON_LEN].rm_eo]));
        size_t len = (size_t)(line_end - *cursor);
        p_resp->content_len = new_str_slice(*cursor, len - strlen("\r\n"));
    }
}


int parse_first_line(resp_parse_ctx_t *ctx, h_resp_t *p_resp, char *url) {
    regex_t *regexes = ctx->regexes;
    regmatch_t *matches = ctx->regm;
    int *res = ctx->res;
    char **cursor = &(ctx->cursor);

    res[VER] = regexec(&(regexes[VER]), *cursor, 1, &(matches[VER]), 0);
    if(res[VER] != REG_NOMATCH) {
        size_t version_len = matches[VER].rm_eo - matches[VER].rm_so;
        p_resp->version = new_str_slice(*cursor, version_len);
        *cursor = &((*cursor)[matches[VER].rm_eo]);
    }

    *cursor = skip_w_spaces(*cursor);
    res[STAT] = regexec(&(regexes[STAT]), *cursor, 1, &(matches[STAT]), 0);
    if(res[STAT] == REG_NOMATCH) {
        printerr(HTTP_ERROR, "Unable to find status code in reponse from '%s'!", *cursor);
        return HTTP_ERROR;
    }

    size_t status_len = matches[STAT].rm_eo - matches[STAT].rm_so;
    p_resp->status = new_str_slice(*cursor, status_len);
    *cursor = &((*cursor)[matches[STAT].rm_eo]);

    *cursor = skip_w_spaces(*cursor);
    res[PHR] = regexec(&(regexes[PHR]), *cursor, 1, &(matches[PHR]), 0);
    if(res[PHR] == REG_NOMATCH) {
        printerr(HTTP_ERROR, "Unable to find status phrase in reponse from '%s'!", url);
        return HTTP_ERROR;
    }

    size_t phrase_len = matches[PHR].rm_eo - matches[PHR].rm_so;
    p_resp->phrase = new_str_slice(*cursor, phrase_len);
    *cursor = &((*cursor)[matches[PHR].rm_eo]);

    return SUCCESS;
}


int parse_resp_headers(resp_parse_ctx_t *ctx, h_resp_t *p_resp, char *hdrs_start, char *url) {
    size_t line_no = 0;

    int ret = SUCCESS;
    regex_t *regexes = ctx->regexes;
    regmatch_t *matches = ctx->regm;
    int *res = ctx->res;
    char **cursor = &(ctx->cursor);

    char *line_st;
    *cursor = hdrs_start;
    res[LINE] = regexec(&(regexes[LINE]), *cursor, 1, &(matches[LINE]), 0);
    *cursor = line_st = &((*cursor)[matches[LINE].rm_so]);

    for(; (res[LINE] != REG_NOMATCH) && !is_line_empty(*cursor); line_no++) {
        if(line_no == 0) {
            if((ret = parse_first_line(ctx, p_resp, url)) != SUCCESS) {
                break;
            }
        }
        else {
            parse_hdrs(&line_st[matches[LINE].rm_eo], ctx, p_resp);
        }

        *cursor = &(line_st[matches[LINE].rm_eo]);
        res[LINE] = regexec(&(regexes[LINE]), *cursor, 1, &(matches[LINE]), 0);
        *cursor = line_st = &((*cursor)[matches[LINE].rm_so]);
    }

    if(line_no == 0 && ret == SUCCESS) {
        printerr(HTTP_ERROR, "Invalid headers of HTTP response from '%s' (missing initial header)!", url);
        ret = HTTP_ERROR;
    }

    return ret;
}


int parse_http_resp(h_resp_t *parsed_resp, string_t *response, char *url) {
    resp_parse_ctx_t ctx;
    int ret = SUCCESS;
    char *resp = response->str;

    regex_t *regexes = ctx.regexes;
    regmatch_t *matches = ctx.regm;
    int *res = ctx.res;

    if((ret = prepare_resp_patterns(regexes)) != SUCCESS) { //< Preparation of POSIX regex structures
        return ret;
    }

    init_resp_res_arr(ctx.res);

    res[H_PART] = regexec(&regexes[H_PART], resp, 1, &(matches[H_PART]), 0);

    if(res[H_PART] == REG_NOMATCH) {
        printerr(HTTP_ERROR, "Headers of HTTP response from '%s' was not found!", url);
        ret = HTTP_ERROR;
    }
    else {
        parsed_resp->msg = &(resp[matches[H_PART].rm_eo]);
        ret = parse_resp_headers(&ctx, parsed_resp, response->str, url);
    }

    free_all_patterns(regexes, RE_H_RESP_NUM);

    return ret;
}
