/**
 * @file cli.c
 * @brief Source file with functions resposible for communication with the user
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#include "cli.h"


/**
 * @brief Auxiliary structure to avoid too many parameters of option parsing 
 * functions
 */
typedef struct opt {
    char *name; //< Pointer to the name of option (usefull expecially for error messages)
    bool *flag; //< Pointer to the variable, that signalizes that option is ON
    char **arg; //< Pointer to the pointer to the argument of specific option
} opt_t;


void init_settings(settings_t *settings) {
    memset(settings, 0, sizeof(settings_t));
}


void print_usage() {
    const char *usage_msg = 
        "USAGE: ./feedreader <URL|-f <feedfile>> [options]\n";

    fprintf(stderr, "%s\n", usage_msg);
}


void print_help() {
    const char *about_msg = 
        "Reader of new in Atom/RSS from given URL\n";

    const char *option_msg = 
        "options:\n"
        "-h, --help     Prints help to stderr\n"
        "-f feedfile    Specifies path to a file with URLs to Atom/RSS sources\n"
        "-c certfile    Specifies path to a file with certificates\n"
        "-C certaddr    Specifies path to a folder with certificate files\n"
        "-T             Adds information about update time to the output\n"
        "-u             Adds associated URL to the output\n"
        "-a             Adds author name to the output\n";

    fprintf(stderr, "%s\n", about_msg);
    print_usage();
    fprintf(stderr, "%s\n", option_msg);
}


int rec_opt(char opt_char, opt_t *opt, settings_t *s) {
    opt->flag = NULL;
    opt->arg = NULL;

    // All short options of the program
    switch(opt_char) {
        case 'h':
            opt->name = "h";
            opt->flag = &s->help_flag;
            break;
        case 'T':
            opt->name = "T";
            opt->flag = &s->time_flag;
            break;
        case 'u':
            opt->name = "u";
            opt->flag = &s->asoc_url_flag;
            break;
        case 'a':
            opt->name = "a";
            opt->flag = &s->author_flag;
            break;
        case 'f':
            opt->name = "f";
            opt->arg = &s->feedfile;
            break;
        case 'c':
            opt->name = "c";
            opt->arg = &s->certfile;
            break;
        case 'C':
            opt->name = "C";
            opt->arg = &s->certaddr;
            break;
        default: //< Unknown option was used
            printerr(USAGE_ERROR, "Unknown option -%c!", opt_char);
            return USAGE_ERROR;
    }

    return SUCCESS;
}


int rec_lopt(char *cur_opt, int *char_i, opt_t *opt, settings_t *s) {
    opt->flag = NULL;
    opt->arg = NULL;

    char *opt_str = shift(cur_opt, strlen("--"));

    // All long options of the program
    if(!strcmp(opt_str, "help")) {
        opt->name = "help";
        opt->flag = &s->help_flag;
    }
    else {
        printerr(USAGE_ERROR, "Unknown option: --%s!", opt_str);
        return USAGE_ERROR;
    }

    *char_i = strlen("--") + strlen(opt_str);

    return SUCCESS;
}


char *get_arg(int argc, char **argv, int *opt_index, char *exp_start) {
    
    if(strlen(exp_start) == 0) { //< Argument of the option is (probably) next argument
        if(*opt_index + 1 == argc) { //< Not it is not (there are not any other arguments)
            return NULL;
        }
        else {
            (*opt_index)++;

            return argv[*opt_index];
        }
    }
    else { //< Argument of the option starts immediately after the option
        return exp_start;
    }
}


int set_values(int argc, char **argv, int *opt_i, char *start, opt_t *option) {
    if(option->flag) {
        *(option->flag) = true;
    }

    if(option->arg) {
        *(option->arg) = get_arg(argc, argv, opt_i, start);

        if(!*(option->arg)) {
            char *opt_name = option->name;
            printerr(USAGE_ERROR, "Option '%s' requires an argument!", opt_name);
            return USAGE_ERROR;
        }
    }

    return SUCCESS;
}


int parse_opts(int argc, char **argv, settings_t *settings) {

    for(int i = 1; i < argc; i++) { //< Skip the first argument (it is the program name)
        char *current_opt = argv[i];
        int cur_char_i = 0;
        if(current_opt[cur_char_i] != '-') { //< Argument without any option sign is url 
            settings->url = current_opt;
            continue;
        }

        cur_char_i++;

        int ret = SUCCESS;
        opt_t option = {.name = NULL, .flag = NULL, .arg = NULL};
        if(current_opt[cur_char_i] == '-') { //< Second character is also '-' => it is long option
            ret = rec_lopt(current_opt, &cur_char_i, &option, settings);
            if(ret != SUCCESS) {
                return ret;
            }

            // Set values associated with the option (flag/pointer to the argument)
            ret = set_values(argc, argv, &i, &current_opt[cur_char_i], &option);
            if(ret != SUCCESS) {
                return ret;
            }
        }
        else { //< Only first character is '-'

            while(current_opt[cur_char_i]) {
                ret = rec_opt(current_opt[cur_char_i], &option, settings);
                if(ret != SUCCESS) {
                    return ret;
                }
                cur_char_i++;

                // Set values associated with the option (flag/pointer to the argument)
                ret = set_values(argc, argv, &i, &current_opt[cur_char_i], &option);
                if(ret != SUCCESS) {
                    return ret;
                }
            }
        }
    }

    return SUCCESS;
}