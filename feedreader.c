/**
 * @file feedreader.c
 * @brief Source file with main function of feedreader program
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#include "feedreader.h"

#define DEBUG

/**
 * @brief Checks if given configuration of settings is valid and eventually prints error msg
 * 
 * @param settings Configuration to be checked
 * @return 0 if settings structure is correct, otherwise error code
 */
int validate_settings(settings_t *settings) {
    if(!settings->url && !settings->feedfile) {
        printerr(USAGE_ERROR, "URL or feedfile required!");
        print_usage();
        return USAGE_ERROR;
    }
    else if(settings->url && settings->feedfile) {
        printerr(USAGE_ERROR, "Specified feedfile and URL at the same time!");
        print_usage();
        return USAGE_ERROR;
    }

    return SUCCESS;
}


int move_to_list(string_t *buffer, list_t *dst_list) {
    list_el_t *new_url = new_element(buffer->str);
    if(!new_url) {
        string_dtor(buffer);
        printerr(INTERNAL_ERROR, "");
        return INTERNAL_ERROR;
    }

    list_append(dst_list, new_url);
    erase_string(buffer);

    return SUCCESS;
}


int proc_char(char c, string_t *buff, list_t *list, int *len, bool *is_cmnt) {
    int ret;

    if(c == '\n') {
        *is_cmnt = false;
    }

    if(*len > 0 && c == '\n') {
        if((ret = move_to_list(buff, list)) != SUCCESS) {
            return INTERNAL_ERROR;
        }
        
        *len = 0;
    }
    else if((*len == 0 && c == '\n') || isspace(c) || *is_cmnt) {
        return SUCCESS;
    }
    else if(c == '#' && *len == 0) {
        *is_cmnt = true;
    }
    else {
        if(!app_char(buff, c)) {
            string_dtor(buff);
            printerr(INTERNAL_ERROR, "");
            return INTERNAL_ERROR;
        }

        (*len)++;
    }

    return SUCCESS;
}



int parse_feedfile(char *path, list_t *url_list) {
    FILE *file_ptr = fopen(path, "r");
    if(!file_ptr) {
        printerr(FILE_ERROR, "%s", strerror(errno));
        return FILE_ERROR;
    }

    string_t *buffer = new_string(INIT_STRING_SIZE);
    if(!buffer) {
        printerr(INTERNAL_ERROR, "Unable to allocate buffer for parsing feedfile!");
        fclose(file_ptr);
        return INTERNAL_ERROR;
    }

    int ret, c, len = 0;
    bool is_cmnt = false;
    while((c = fgetc(file_ptr)) != EOF) {
        if((ret = proc_char(c, buffer, url_list, &len, &is_cmnt)) != SUCCESS) {
            fclose(file_ptr);
            return ret;
        }
    }

    if(len > 0) {
        if((ret = move_to_list(buffer, url_list)) != SUCCESS) {
            fclose(file_ptr);
            return ret;
        }
    }

    #ifdef DEBUG //Prints all urls from url list
    fprintf(stderr, "Found\n");
    list_el_t *current = url_list->header;
    while(current)
    {
        fprintf(stderr, "url: %s\n", current->string->str);
        current = current->next;
    }
    #endif
    

    string_dtor(buffer);
    fclose(file_ptr);

    return SUCCESS;
}

//The process of initializing features of openssl library is based on https://developer.ibm.com/tutorials/l-openssl/
void openssl_init() {
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
}
//End of the part based on https://developer.ibm.com/tutorials/l-openssl/


int send_request(BIO *bio, h_url_t *p_url, char *url) {
    int ret, attempt_num = 0;

    const struct timespec req = { 
        .tv_nsec = REQ_TIME_INTERVAL_NS, 
        .tv_sec = 0
    };
    struct timespec rem;

    char request_b[INIT_NET_BUFF_SIZE];
    snprintf(request_b, INIT_NET_BUFF_SIZE, 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n" //< Mandatory due to RFC2616
        "Connection: close\r\n" //< Connection will be closed after completition of the response
        "User-Agent: ISAFeedReader/1.0\r\n" //< Just to better filtering from the other traffic
        "\r\n",
        p_url->h_url_parts[PATH]->str, 
        p_url->h_url_parts[HOST]->str);

    while((ret = BIO_write(bio, request_b, strlen(request_b))) <= 0) {
        if(!BIO_should_retry(bio) || attempt_num >= MAX_ATTEMPT_NUM) { //< Checking if write should be repeated (in some cases is should be repeated even without SSL due to docs)
            printerr(CONNECTION_ERROR, "Unable to send request to the '%s'!", url);
            return CONNECTION_ERROR;
        }
        else {
            if(nanosleep(&req, &rem) < 0) { //Works only with gnu* standard
                printerr(INTERNAL_ERROR, "%s", strerror(errno));
                return INTERNAL_ERROR;
            }
        }

        attempt_num++;
    }

    return SUCCESS;
}


int get_feed(list_t *url_list, settings_t *settings) {
    list_el_t *current = url_list->header;
    int ret;

    h_url_t parsed_url;
    init_h_url(&parsed_url);

    openssl_init();
    while(current) {
        char *url = current->string->str;

        if((ret = parse_h_url(url, &parsed_url, "https://")) != SUCCESS) {
            h_url_dtor(&parsed_url);
            return ret;
        }

        BIO *bio = BIO_new(BIO_s_connect());
        if(!bio) {
            printerr(INTERNAL_ERROR, "Unable to allocate BIO!");
            h_url_dtor(&parsed_url);
            return INTERNAL_ERROR;
        }

        BIO_set_conn_hostname(bio, parsed_url.h_url_parts[HOST]->str); //< Always returns 1 -> no need to check retval
        BIO_set_conn_port(bio, parsed_url.h_url_parts[PORT_PART]->str); //< -||-

        if(BIO_do_connect(bio) <= 0) {
            printerr(CONNECTION_ERROR, "Cannot connect to the '%s'!", url);
            #ifdef DEBUG
                fprintf(stderr, "Details:\n");
                ERR_print_errors_fp(stderr);
            #endif
            h_url_dtor(&parsed_url);
            BIO_free_all(bio);
            return CONNECTION_ERROR;
        }
        
        send_request(bio, &parsed_url, url);

        BIO_free_all(bio);

        current = current->next;
    }

    h_url_dtor(&parsed_url);
    return SUCCESS;
}


int main(int argc, char **argv) {
    int ret_code = SUCCESS;

    settings_t settings;
    init_settings(&settings);

    ret_code = parse_opts(argc, argv, &settings);
    if(ret_code != SUCCESS) {
        return ret_code;
    }
    if(settings.help_flag) {
        print_help();
        return SUCCESS;
    }

    ret_code = validate_settings(&settings);
    if(ret_code != SUCCESS) {
        return ret_code;
    }

    list_t url_list;
    list_init(&url_list);

    if(settings.feedfile) {
        ret_code = parse_feedfile(settings.feedfile, &url_list);
        if(ret_code != SUCCESS) {
            list_dtor(&url_list);
            return ret_code;
        }
    }
    else if(settings.url) {
        list_el_t *first = new_element(settings.url);
        if(!first) {
            printerr(INTERNAL_ERROR, "Unable to allocate new element for url list!");
            list_dtor(&url_list);
            return INTERNAL_ERROR;
        }

        list_append(&url_list, first);
    }

    ret_code = get_feed(&url_list, &settings);
    if(ret_code != SUCCESS) {
        list_dtor(&url_list);
        return ret_code;
    }

    list_dtor(&url_list);

    return ret_code;
}