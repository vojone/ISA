/**
 * @file feedreader.h
 * @brief Header file of main file of feedreader program
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 6. 10. 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#include "utils.h"
#include "cli.h"

#define REQ_TIME_INTERVAL_NS 5e8
#define MAX_ATTEMPT_NUM 5

