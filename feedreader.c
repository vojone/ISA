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
    list_el_t *new_url = new_element(buffer->str, 0);
    if(!new_url) {
        printerr(INTERNAL_ERROR, "Unable to move '%s' to the list!", buffer->str);
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
        if(!(buff = app_char(&buff, c))) {
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

void init_feed_doc(feed_doc_t *feed_doc) {
    feed_doc->src_name = NULL;
    feed_doc->feed = NULL;
}

void add_feed(feed_doc_t *feed_doc, feed_el_t *new_feed) {
    feed_el_t **feed = &(feed_doc->feed);

    while(*feed) {
        feed = &((*feed)->next);
    }

    *feed = new_feed;
}

feed_el_t *new_feed(feed_doc_t *feed_doc) {
    feed_el_t *new_feed_ = (feed_el_t *)malloc(sizeof(feed_el_t));
    if(!new_feed_) {
        return NULL;
    }

    memset(new_feed_, 0, sizeof(feed_el_t));

    if(feed_doc) {
        add_feed(feed_doc, new_feed_);
    }

    return new_feed_;
}

void feed_dtor(feed_el_t *feed) {
    if(feed) {
        if(feed->auth_name) xmlFree(feed->auth_name);
        if(feed->title) xmlFree(feed->title);
        if(feed->updated) xmlFree(feed->updated);
        if(feed->url) xmlFree(feed->url);
        free(feed);
    }
}

void feed_doc_dtor(feed_doc_t *feed_doc) {
    if(feed_doc->src_name) xmlFree(feed_doc->src_name);

    feed_el_t *feed = feed_doc->feed, *tmp;

    while(feed) {
        tmp = feed;
        feed = feed->next;

        feed_dtor(tmp);
    }
}

int parse_and_print_feed(char *msg, settings_t *settings, char *url) {
    xmlDocPtr xml = xmlReadMemory(msg, strlen(msg), url, NULL, 0);
    if(!xml) {
        printerr(INTERNAL_ERROR, "Unable to parse XML document from '%s'!", url);
        return INTERNAL_ERROR;
    }

    xmlNodePtr root = xmlDocGetRootElement(xml);
    if(!root) {
        printerr(INTERNAL_ERROR, "Unable to find root node of XML document from '%s'!", url);
        return INTERNAL_ERROR;
    }

    #ifdef DEBUG
        fprintf(stderr, "Root: %s\n", root->name);
    #endif

    feed_el_t *cur_feed;
    feed_doc_t feed_doc;
    init_feed_doc(&feed_doc);

    if(!xmlStrcasecmp(root->name, (const xmlChar *)"feed")) {
        xmlNodePtr root_child = root->children, child, sub_child;

        while(root_child) {
            if(!xmlStrcasecmp(root_child->name, (const xmlChar *)"title")) {
                if(!(feed_doc.src_name = xmlNodeGetContent(root_child))) {
                    printerr(FEED_ERROR, "");
                    feed_doc_dtor(&feed_doc);
                    return FEED_ERROR;
                }
            }
            else if(!xmlStrcasecmp(root_child->name, (const xmlChar *)"entry")) {
                if(!(cur_feed = new_feed(&feed_doc))) {
                    printerr(INTERNAL_ERROR, "Unable to allocate memory for feed structure!");
                    feed_doc_dtor(&feed_doc);
                    return INTERNAL_ERROR;
                }
                
                child = root_child->children;

                while(child) {
                    if(!xmlStrcasecmp(child->name, (const xmlChar *)"title")) {
                        if(!(cur_feed->title = xmlNodeGetContent(child))) {
                            printerr(FEED_ERROR, "");
                            feed_doc_dtor(&feed_doc);
                            return FEED_ERROR;
                        }
            
                    }
                    else if(!xmlStrcasecmp(child->name, (const xmlChar *)"updated")) {
                        if(!(cur_feed->updated = xmlNodeGetContent(child))) {
                            printerr(FEED_ERROR, "");
                            feed_doc_dtor(&feed_doc);
                            return FEED_ERROR;
                        }
                    }
                    else if(!xmlStrcasecmp(child->name, (const xmlChar *)"link")) {
                        if(!(cur_feed->url = xmlGetProp(child, (const xmlChar *)"href"))) {
                            printerr(FEED_ERROR, "");
                            feed_doc_dtor(&feed_doc);
                            return FEED_ERROR;
                        }
                    }
                    else if(!xmlStrcasecmp(child->name, (const xmlChar *)"author")) {
                        sub_child = child->children;

                        while(sub_child) {
                            if(!xmlStrcasecmp(sub_child->name, (const xmlChar *)"name")) {
                                if(!(cur_feed->auth_name = xmlNodeGetContent(sub_child))) {
                                    printerr(FEED_ERROR, "");
                                    feed_doc_dtor(&feed_doc);
                                    return FEED_ERROR;
                                }
                            }

                            sub_child = sub_child->next;
                        }
                    }

                    child = child->next;
                }
            }

            root_child = root_child->next;
        }
    }
    else if(!xmlStrcasecmp(root->name, (const xmlChar *)"rss")) {
        xmlNodePtr channel = root->children, channel_child, item_child;  

        xmlChar *v = xmlGetProp(root, (const xmlChar *)"version");
        if(!v) {
            printerr(FEED_ERROR, "Missing version attribute of 'rss' in rss tag!");
            feed_doc_dtor(&feed_doc);
            xmlFreeDoc(xml);
            return FEED_ERROR;
        }

        bool is_supported = !xmlStrcasecmp(v, (const xmlChar *)RSS_VERSION);
        xmlFree(v);

        if(!is_supported) {
            printerr(FEED_ERROR, "Unsupported version of RSS. Supported '%s' got '%s'!", RSS_VERSION, v);
            feed_doc_dtor(&feed_doc);
            xmlFreeDoc(xml);
            return FEED_ERROR;
        }

        while(channel) {
            if(!xmlStrcasecmp(channel->name, (const xmlChar *)"channel")) {
                channel_child = channel->children;

                while(channel_child) {
                    if(!xmlStrcasecmp(channel_child->name, (const xmlChar *)"title")) {
                        if(!(feed_doc.src_name = xmlNodeGetContent(channel_child))) {
                            printerr(FEED_ERROR, "");
                            feed_doc_dtor(&feed_doc);
                            return FEED_ERROR;
                        }
                    }
                    else if(!xmlStrcasecmp(channel_child->name, (const xmlChar *)"item")) {
                        if(!(cur_feed = new_feed(&feed_doc))) {
                            printerr(INTERNAL_ERROR, "Unable to allocate memory for feed structure!");
                            feed_doc_dtor(&feed_doc);
                            return INTERNAL_ERROR;
                        }

                        item_child = channel_child->children;

                        while(item_child) {
                            if(!xmlStrcasecmp(item_child->name, (const xmlChar *)"title")) {
                                if(!(cur_feed->title = xmlNodeGetContent(item_child))) {
                                    printerr(FEED_ERROR, "");
                                    feed_doc_dtor(&feed_doc);
                                    return FEED_ERROR;
                                }
                    
                            }
                            else if(!xmlStrcasecmp(item_child->name, (const xmlChar *)"link")) {
                                if(!(cur_feed->url = xmlNodeGetContent(item_child))) {
                                    printerr(FEED_ERROR, "");
                                    feed_doc_dtor(&feed_doc);
                                    return FEED_ERROR;
                                }
                            }
                            else if(!xmlStrcasecmp(item_child->name, (const xmlChar *)"pubDate")) { //?
                                if(!(cur_feed->updated = xmlNodeGetContent(item_child))) {
                                    printerr(FEED_ERROR, "");
                                    feed_doc_dtor(&feed_doc);
                                    return FEED_ERROR;
                                }
                            }
                            else if(!xmlStrcasecmp(item_child->name, (const xmlChar *)"author")) { //?
                                if(!(cur_feed->auth_name = xmlNodeGetContent(item_child))) {
                                    printerr(FEED_ERROR, "");
                                    feed_doc_dtor(&feed_doc);
                                    return FEED_ERROR;
                                }
                            }
                            

                            item_child = item_child->next;
                        }
                    }

                    channel_child = channel_child->next;
                }
            }

            channel = channel->next;
        }
    }
    else {
        printerr(FEED_ERROR, "Unexpected name of root element of XML from '%s'! Expected feed/rss", url);
        feed_doc_dtor(&feed_doc);
        xmlFreeDoc(xml);
        return FEED_ERROR;
    }


    printf("*** %s ***\n", feed_doc.src_name);

    feed_el_t *feed = feed_doc.feed;
    while(feed) {
        printf("%s\n", feed->title);

        if(feed->auth_name && settings->author_flag) {
            printf("%s\n", feed->auth_name);
        }
        if(feed->url && settings->asoc_url_flag) {
            printf("%s\n", feed->url);
        }
        if(feed->updated && settings->time_flag) {
            printf("%s\n", feed->updated);
        }

        if(settings->author_flag || 
            settings->asoc_url_flag || 
            settings->time_flag) {
            printf("\n");
        }

        feed = feed->next;
    }


    feed_doc_dtor(&feed_doc);
    xmlFreeDoc(xml);

    return SUCCESS;
}


int read_and_print_feed(list_t *url_list, settings_t *settings) {
    list_el_t *current = url_list->header;
    int ret = SUCCESS;

    h_url_t parsed_url;
    init_h_url(&parsed_url);

    h_resp_t parsed_resp;
    init_h_resp(&parsed_resp);

    string_t *resp_buff = new_string(INIT_NET_BUFF_SIZE);
    if(!resp_buff) {
        printerr(INTERNAL_ERROR, "Unable to allocate buffer for responses!");
        return INTERNAL_ERROR;
    }

    openssl_init();
    while(current) {
        char *url = current->string->str;

        erase_h_url(&parsed_url);
        if((ret = parse_h_url(url, &parsed_url, "https://")) != SUCCESS) {
            string_dtor(resp_buff);
            h_url_dtor(&parsed_url);
            return ret;
        }

        if(!strcmp("https://", parsed_url.h_url_parts[SCHEME_PART]->str)) {
            ret = https_connect(&parsed_url, resp_buff, url, settings);
        }
        else {
            ret = http_connect(&parsed_url, resp_buff, url);
        }
        if(ret != SUCCESS) {
            break;
        }

        ret = parse_http_resp(&parsed_resp, resp_buff, url);
        if(ret != SUCCESS) {
            break;
        }

        ret = check_http_resp(&parsed_resp, current, url);
        if(ret == SUCCESS) {
            ret = parse_and_print_feed(parsed_resp.msg, settings, url);
            if(ret != SUCCESS) {
                break;
            }
        }
        else if(ret == HTTP_REDIRECT) {

        }
        else {
            break;
        }

        #ifdef DEBUG
            fprintf(stderr, "V: %ld %ld\n", parsed_resp.version.st - resp_buff->str, parsed_resp.version.len);
            fprintf(stderr, "Status: %ld %ld\n", parsed_resp.status.st - resp_buff->str, parsed_resp.status.len);
            fprintf(stderr, "Phrase: %ld %ld\n", parsed_resp.phrase.st - resp_buff->str, parsed_resp.phrase.len);
            fprintf(stderr, "Loc: %ld %ld\n", parsed_resp.location.st - resp_buff->str, parsed_resp.location.len);
            fprintf(stderr, "CType: %ld %ld\n", parsed_resp.content_type.st - resp_buff->str, parsed_resp.content_type.len);
            //fprintf(stderr, "Msg: %s\n", parsed_resp.msg);
        #endif

        current = current->next;
    }

    string_dtor(resp_buff);
    h_url_dtor(&parsed_url);

    return ret;
}


int main(int argc, char **argv) {
    LIBXML_TEST_VERSION

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
        list_el_t *first = new_element(settings.url, 0);
        if(!first) {
            printerr(INTERNAL_ERROR, "Unable to allocate new element for url list!");
            list_dtor(&url_list);
            return INTERNAL_ERROR;
        }

        list_append(&url_list, first);
    }

    ret_code = read_and_print_feed(&url_list, &settings);
    if(ret_code != SUCCESS) {
        list_dtor(&url_list);
        return ret_code;
    }

    list_dtor(&url_list);
    xmlCleanupParser();

    return ret_code;
}