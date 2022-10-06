/**
 * @file feedreader.c
 * @brief Source file with main function of feedreader program
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
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
        printerr(INTERNAL_ERROR, "");
        return INTERNAL_ERROR;
    }

    int c, cur_len = 0;
    bool is_cmnt = false;
    while((c = fgetc(file_ptr)) != EOF) {
        if(c == '\n') {
            is_cmnt = false;
        }

        if(cur_len > 0 && c == '\n') {
            list_el_t *new_url = new_element(buffer->str);
            if(!new_url) {
                string_dtor(buffer);
                printerr(INTERNAL_ERROR, "");
                return INTERNAL_ERROR;
            }

            list_append(url_list, new_url);
            cur_len = 0;
            erase_string(buffer);
        }
        else if((cur_len == 0 && c == '\n') || isspace(c) || is_cmnt) {
            continue;
        }
        else if(c == '#' && cur_len == 0) {
            is_cmnt = true;
        }
        else {
            if(!app_char(buffer, c)) {
                return INTERNAL_ERROR;
            }

            cur_len++;
        }
    }

    #ifdef DEBUG //Prints all urls from url list
    list_el_t *current = url_list->header;
    while(current)
    {
        fprintf(stderr, "%s\n", current->string->str);
        current = current->next;
    }
    #endif
    

    string_dtor(buffer);
    fclose(file_ptr);

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
            printerr(INTERNAL_ERROR, "");
            list_dtor(&url_list);
            return INTERNAL_ERROR;
        }

        list_append(&url_list, first);
    }


    list_dtor(&url_list);

    return ret_code;
}