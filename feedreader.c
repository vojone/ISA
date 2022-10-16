/**
 * @file feedreader.c
 * @brief Source file with main function of feedreader program
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 15. 10. 2022
 */

#include "feedreader.h"


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


/**
 * @brief Moves string buffer (with URL) to linked list as new element
 * 
 * @param buffer Buffer its value should be copied to the element of list
 * @param dst_list Target list
 * @return int SUCCESS if moving went OK
 * @note buffer is erased
 */
int move_to_list(string_t *buffer, list_t *dst_list) {
    list_el_t *new_url = new_element(buffer->str);
    if(!new_url) {
        printerr(INTERNAL_ERROR, "Unable to move '%s' to the list!", buffer->str);
        return INTERNAL_ERROR;
    }

    new_url->indirect_lvl = 0; //< It is original URL

    list_append(dst_list, new_url);
    erase_string(buffer);

    return SUCCESS;
}


/**
 * @brief Processes character from the feedfile to create the list with URLs
 * 
 * @param c Character to be processed
 * @param buff Buffer with URL
 * @param list URL list
 * @param len Ptr to the length of string in the buffer
 * @param is_cmnt Comment flag (line is ignored if there is '#' as first non-whitepsace character)
 * @return int SUCESS if processing went OK
 */
int proc_char(char c, string_t *buff, list_t *list, int *len, bool *is_cmnt) {
    int ret;

    if(c == '\n') {
        *is_cmnt = false;
    }

    if(*len > 0 && c == '\n') { //< If there is newline and buffer is not empty -> move URL to the list
        if((ret = move_to_list(buff, list)) != SUCCESS) {
            printerr(INTERNAL_ERROR, "Unable to move URL to the list!");
            return INTERNAL_ERROR;
        }
        
        *len = 0;
    }
    else if((*len == 0 && c == '\n') || isspace(c) || *is_cmnt) { //< Characters to be ignored
        return SUCCESS;
    }
    else if(c == '#' && *len == 0) { //< Comment was found
        *is_cmnt = true;
    }
    else { //< Regular character
        if(!(buff = app_char(&buff, c))) {
            printerr(INTERNAL_ERROR, "Unable to append char to the string while parsing feedfile!");
            return INTERNAL_ERROR;
        }

        (*len)++;
    }

    return SUCCESS;
}


/**
 * @brief Parses feedfile (text file with URL) and creates URL list from it
 * 
 * @param path Path to the feedfile
 * @param url_list Output structure with URLs from feedfile 
 * @return int SUCCESS if everything was OK
 */
int parse_feedfile(char *path, list_t *url_list) {
    FILE *file_ptr = fopen(path, "r");
    if(!file_ptr) {
        printerr(FILE_ERROR, "%s", strerror(errno));
        return FILE_ERROR;
    }

    string_t *buffer = new_string(INIT_STRING_SIZE); //< Buffer for the file content
    if(!buffer) {
        printerr(INTERNAL_ERROR, "Unable to allocate buffer for parsing feedfile!");
        fclose(file_ptr);
        return INTERNAL_ERROR;
    }

    int ret, c, len = 0;
    bool is_cmnt = false;
    while((c = fgetc(file_ptr)) != EOF) { //< PArse file char by char
        if((ret = proc_char(c, buffer, url_list, &len, &is_cmnt)) != SUCCESS) {
            fclose(file_ptr);
            return ret;
        }
    }

    if(len > 0) { //< There is EOF without LF before (it shouldn't cause it is abnormal in UNIX text files)
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


/**
 * @brief Parses and prints feed from specific URL
 * 
 * @param feed Pointer to feed to be parsed
 * @param exp_type Expected MIME type of the document
 * @param settings 
 * @param url Source URL
 * @return int SUCCESS if everything went OK
 */
int parse_and_print(char *feed, int exp_type, settings_t *settings, char *url) {
    int ret;
    feed_doc_t feed_doc;
    init_feed_doc(&feed_doc);

    ret = parse_feed_doc(&feed_doc, exp_type, feed, url);
    if(ret != SUCCESS) {
        feed_doc_dtor(&feed_doc);
        return ret;
    }

    print_feed_doc(&feed_doc, settings);
    
    if(settings->feedfile) {
        printf("\n");
    }

    feed_doc_dtor(&feed_doc);

    return SUCCESS;
}


int load_data(h_url_t *p_url, string_t *data_buff, char *url, settings_t *s) {
    if(!strcmp("https://", p_url->h_url_parts[SCHEME_PART]->str)) {
        return https_connect(p_url, data_buff, url, s);
    }
    else {
        return http_connect(p_url, data_buff, url);
    }
}


int parse_data(int *exp_type, char** doc_start, h_url_t *parsed_url, 
                list_el_t *current, string_t *data_buff,  char *url) {
    int ret = SUCCESS;

    char *scheme = parsed_url->h_url_parts[SCHEME_PART]->str;
    if(!strcmp(scheme, "https://") || !strcmp(scheme, "http://")) {
        h_resp_t parsed_resp;
        init_h_resp(&parsed_resp);

        erase_h_resp(&parsed_resp);
        ret = parse_http_resp(&parsed_resp, data_buff, url);
        if(ret != SUCCESS) {
            return ret;
        }

        ret = check_http_resp(&parsed_resp, current, url);
        if(ret != SUCCESS) {
            return ret;
        }

        *exp_type = parsed_resp.doc_type;
        *doc_start = parsed_resp.msg;

        #ifdef DEBUG
            fprintf(stderr, "HTTP hdr position:\n");
            fprintf(stderr, "Version: %ld %ld\n", parsed_resp.version.st - data_buff->str, parsed_resp.version.len);
            fprintf(stderr, "Status: %ld %ld\n", parsed_resp.status.st - data_buff->str, parsed_resp.status.len);
            fprintf(stderr, "Phrase: %ld %ld\n", parsed_resp.phrase.st - data_buff->str, parsed_resp.phrase.len);
            fprintf(stderr, "Location: %ld %ld\n", parsed_resp.location.st - data_buff->str, parsed_resp.location.len);
            fprintf(stderr, "Content-Type: %ld %ld\n", parsed_resp.content_type.st - data_buff->str, parsed_resp.content_type.len);
            //fprintf(stderr, "Msg: %s\n", parsed_resp.msg);
        #endif
    }
    else {
        printerr(URL_ERROR, "Unknown scheme!");
        return URL_ERROR;
    }

    return ret;
}


/**
 * @brief Performs the general functionality of the program - parsing and 
 * printing formatted feed from all specified source
 * 
 * @param url_list List with URLs to feed documents 
 * @param settings Settings of the program
 * @return int SUCCESS if everything went OK, otherwise INTERNAL ERROR
 * @note If problem occurs while parsing URL or XML from source, result field
 * in related list_el_t is modified, but processing of other URLs continues
 */
int do_feedread(list_t *url_list, settings_t *settings) {
    list_el_t *current = url_list->header;

    h_url_t parsed_url;
    init_h_url(&parsed_url);

    string_t *data_buff = new_string(INIT_NET_BUFF_SIZE);
    if(!data_buff) {
        printerr(INTERNAL_ERROR, "Unable to allocate buffer for responses!");
        return INTERNAL_ERROR;
    }

    openssl_init();

    for(; current; current = current->next) { //< Parse URL, load document and parse it for every URL in linked list
        int *ret = &(current->result);
        char *url = current->string->str;

        erase_h_url(&parsed_url);
        if((*ret = parse_h_url(url, &parsed_url, "https://")) != SUCCESS) { //< Parsing of URL (with default scheme 'https://')
            continue;
        }

        *ret = load_data(&parsed_url, data_buff, url, settings); //< Loading data (XML doc)
        if(*ret != SUCCESS) {
            continue;
        }

        char *doc_start;
        int exp_type;

        *ret = parse_data(&exp_type, &doc_start, &parsed_url, current, data_buff, url);
        if(*ret != SUCCESS) {
            continue;
        }

        *ret = parse_and_print(doc_start, exp_type, settings, url);
    }

    string_dtor(data_buff);
    h_url_dtor(&parsed_url);

    return SUCCESS;
}


/**
 * @brief Creates linked list with URLs (it can be one element list or
 * llinked list with multiple element in case of using feedfile -
 * feedfile has higher precedence than single URL)
 * 
 * @param url_list Output structure with URLs
 * @param settings Settings of the program
 * @return int SUCCESS if everything went ok
 */
int create_url_list(list_t *url_list, settings_t *settings) {
    int ret_code = SUCCESS;

    if(settings->feedfile) { //< Feed file was specified so sandalone URL in argument is ignored
        ret_code = parse_feedfile(settings->feedfile, url_list);
    }
    else if(settings->url) { //< Put URL from argument to the liast as one element
        list_el_t *first = new_element(settings->url);
        if(!first) {
            printerr(INTERNAL_ERROR, "Unable to allocate new element for url list!");
            ret_code = INTERNAL_ERROR;
        }
        else {
            first->indirect_lvl = 0; //< It is original URL
            list_append(url_list, first);
        }
    }

    return ret_code;
}


int get_return_code(list_t *url_list) {
    int ret_code = SUCCESS;
    list_el_t *url = url_list->header;

    while(url) {
        if(url->result != SUCCESS) {
            url->result = ret_code;
            break;
        }

        url = url->next;
    }

    return ret_code;
}


/**
 * @brief Main function of the program feedreader 
 */
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

    if((ret_code = validate_settings(&settings)) != SUCCESS) {
        return ret_code;
    }

    list_t url_list;
    list_init(&url_list);

    if((ret_code = create_url_list(&url_list, &settings) != SUCCESS)) {
        list_dtor(&url_list);
        return ret_code;
    }

    xml_parser_init();
    ret_code = do_feedread(&url_list, &settings);
    xml_parser_cleanup();

    if(ret_code == SUCCESS) {
        ret_code = get_return_code(&url_list);
    }

    list_dtor(&url_list);

    return ret_code;
}