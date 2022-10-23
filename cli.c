/**
 * @file cli.c
 * @brief Source file with functions resposible for communication with the user
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 23. 10. 2022
 */

#include "cli.h"


void init_settings(settings_t *settings) {
    memset(settings, 0, sizeof(settings_t));
}


void printerr(int err_code, const char *message_format,...) {
    char *err_str[] = { //< Headers of error message (for general message classification)
        "Success",
        "Usage error",
        "Error while opening file",
        "Invalid URL",
        "Connection error",
        "Communication error",
        "Path error",
        "Verification error",
        "HTTP error",
        "Feed source error",
        "Internal error",
    };

    fprintf(stderr, "%s: %s: ", PROGNAME, err_str[err_code]); //< Print headers

    if(message_format) { //< Print the message
        va_list args;
        va_start (args, message_format);
        vfprintf(stderr, message_format, args);
    }

    fprintf(stderr, "\n");
}


void printw(const char *message_format,...) {
    #ifdef CLI_WARNINGS
    
        fprintf(stderr, "%s: Warning: ", PROGNAME);

        if(message_format) {
            va_list args;
            va_start(args, message_format);
            vfprintf(stderr, message_format, args);
        }

        fprintf(stderr, "\n");

    #endif
}


void print_usage() {
    const char *usage_msg = 
        "USAGE: ./feedreader <URL|-f <feedfile>> [options]\n";

    fprintf(stdout, "%s\n", usage_msg);
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

    fprintf(stdout, "%s\n", about_msg);
    print_usage();
    fprintf(stdout, "%s\n", option_msg);
}



/**
 * @brief Recognizes short option (short option means one char flag) with '-'
 * as a prefix
 * 
 * @param opt_char Option char 
 * @param opt Out param, option structure for the result 
 * @param s Settings structure of program
 * @return SUCCESS if everything went OK, otherwise USAGE error
 */
int rec_opt(char opt_char, opt_t *opt, settings_t *s) {
    opt->flag = NULL;
    opt->arg = NULL;

    // All short options of the program
    switch(opt_char) {
        case 'h':
            opt->name = "h"; //< Set option name
            opt->flag = &s->help_flag; //< Set ptr to related variable in settings struct
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


/**
 * @brief Equivalent of rec_opt for long options (prefix '--' is expected)
 * 
 * @param cur_opt Pointer to current processed option in argv array
 * @param char_i Index of ending character of recognized long option
 * @param opt Output parameter, returns option name and ptr to related variable
 * in this opt_t structure 
 * @param s Settings of the program
 * @return SUCCESS if everything went ok, otherwise USAGE_ERROR
 */
int rec_lopt(char *cur_opt, int *char_i, opt_t *opt, settings_t *s) {
    opt->flag = NULL;
    opt->arg = NULL;

    char *opt_str = &(cur_opt[strlen("--")]);

    // All long options of the program
    if(!strcmp(opt_str, "help")) {
        opt->name = "help";
        opt->flag = &s->help_flag;
    }
    else { //< Long option was not recognized
        printerr(USAGE_ERROR, "Unknown option: --%s!", opt_str);
        return USAGE_ERROR;
    }

    *char_i = strlen("--") + strlen(opt_str);

    return SUCCESS;
}


/**
 * @brief Gets the argument of the option (it can be in next option string or
 * it can be immediately after the (short) option flag)  
 * 
 * @param argc 
 * @param argv 
 * @param opt_index Ptr to index of option in argv array
 * @param exp_start Ptr to expected start of argument (after the option flag)
 * @return Ptr to option argument or NULL (if it was not found)
 */
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


/**
 * @brief Set the values due to given option structure
 * 
 * @param argc 
 * @param argv 
 * @param opt_i Ptr to option index 
 * @param start Start of argument of the option
 * @param option Option structure holding option name and ptr to related var
 * @return SUCCESS if setting values was successful 
 */
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

        cur_char_i++; //< Skip to next character in current option

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