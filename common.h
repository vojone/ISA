/**
 * @file common.h
 * @brief Header file of common module containing declaration of
 * exported functions and structures (ADT)
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 15. 10. 2022
 */

#ifndef _FEEDREADER_COMMON_
#define _FEEDREADER_COMMON_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/types.h>

#include "cli.h"

#define INIT_STRING_SIZE 32 //< Default initial size of strings (that are used as buffer)
#define INIT_NET_BUFF_SIZE 16384 //< Must be big enough for request (at least strlen(req_pattern) + strlen(host) + strlen(path) + strlen("\0"))

#define ABS(x) (unsigned int)((x > 0) ? x : -x)

/**
 * @brief Enum of supported MIME types
 *  
 */
typedef enum doc_type {
    RSS,
    ATOM,
    XML,
    MIME_NUM,
} doc_type_t;

/**
 * @brief String structure (emulates string data type from high lvl languages)
 * 
 */
typedef struct string {
    char *str; //< Pointer to string itself
    size_t size; //< Size that was allocated for string (IT IS NOT LENGTH)
} string_t;


/**
 * @brief Structure that marks specific part of (char) buffer as a specific string
 * @note It can be used instead of string_t in some cases to save memory
 * 
 */
typedef struct string_slice {
    char *st; //< Ptr to slice start
    size_t len; //< Length of slice
} string_slice_t;


/**
 * @brief Element of linked list (in this project is used for storing URLs)
 * 
 */
typedef struct list_el {
    string_t *string; //< String structure with content (URL)
    int indirect_lvl; //< Indirection level (for recognizing redirection URL)
    int result;
    struct list_el *next; //< Pointer to next element in the linked list
} list_el_t;


/**
 * @brief Linked list structure (for storing URLs)
 * 
 */
typedef struct list {
    list_el_t *header; //< Pointer to first element of linked list (or NULL if it is empty)
} list_t;


/**
 * @brief Allocates string buffer
 * 
 * @param size Capacity of the new buffer
 * @return Pointer to the newly allocated buffer or NULL (must be checked) 
 */
char *new_str(size_t size);


/**
 * @brief Moves the given pointer n places from the start of the string
 * 
 * @param str String, that should be shifted
 * @param n Number of places that, should be shifted
 * @return Pointer to the shifted string OR pointer to the terminating char \0
 * @warning If n parameter is greater than strlen of given string, pointer to 
 * the terminating character ('\0') is returned 
 */
char *shift(char *str, size_t n);


/**
 * @brief Determines string_t structure is empty or not
 * 
 * @param string String to be measured
 * @return true if length of string represented by given buffer is not zero
 * @return false if given ptr is NULL or length of str (in string) is zero or str is NULL
 */
bool is_empty(string_t *string);


/**
 * @brief Skips all whitespaces from the start of the given character buffer
 * 
 * @param str Ptr to the beginning of the string (with whitespaces)
 * @return char* Ptr to the beginning of the string withotu whitespaces at the start
 * @warning Consider saving ptr to the original string (else it can cause e. g. memory leaks)
 */
char *skip_w_spaces(char *str);


/**
 * @brief Initializes string_slice structure with given parameters
 * 
 * @param ptr Ptr to slice in buffer
 * @param len Length of slice in given buffer
 * @return string_slice_t Initialized structure of string_slice
 */
string_slice_t new_str_slice(char *ptr, size_t len);


/**
 * @brief Checks if given string contains only "\r\n"
 * 
 * @param line_start_ptr Strign to be checked
 * @return true If tehre is only "\r\n"
 * @return false 
 */
bool is_line_empty(char *line_start_ptr);


/**
 * @brief Allocates a new element of the linked list (with indirection level 0)
 * 
 * @param str_content Content of the new element
 * @return list_el_t* Ptr to newly allocated and initialized element or NULL
 */
list_el_t *new_element(char *str_content);


/**
 * @brief Creates new linked list element from string described in pointed slice
 * 
 * @param slice Slice describing the content of the new element
 * @param indir_level Indirection level of the new element
 * @return list_el_t* Pointer to newly created element or NULL (should be checked)
 */
list_el_t *slice2element(string_slice_t *slice, size_t indir_level);


/**
 * @brief Initializes list structure
 * 
 * @param list Struct to be initialized
 */
void list_init(list_t *list);


/**
 * @brief Frees all resources of given linked list struct
 * 
 * @param list List struct to be freed
 */
void list_dtor(list_t *list);


/**
 * @brief Adds given element to the end of given linked list
 * 
 * @param list List to be extended from the and by given element
 * @param new_element Element to be added to the end of the list
 */
void list_append(list_t *list, list_el_t *new_element);


/**
 * @brief Sets all allocated bytes of string to the 0 ('\0')
 * 
 * @param string String to be erased
 */
void erase_string(string_t *string);


/**
 * @brief Truncates string from start of from the end (it does not change 
 * the allocated size, it just rewrites characters to the '\0' or shift them)
 * 
 * @param string String to be truncated
 * @param n Number of chars that should be removed from string (if it is negative
 * it means remove them from the end)
 */
void trunc_string(string_t *string, int n);


/**
 * @brief Adds character to the end of given string, if it is necessary size
 * (capacity) of the string is extended
 * 
 * @param dest Target string
 * @param c Character to be added
 * @return string_t* Pointer to the string with new character at the end or NULL
 * @warning Always get pointer of the returned string (there may be reallocation)
 */
string_t *app_char(string_t **dest, char c);


/**
 * @brief Set the content of the string to the null terminated character
 * string (pointed by second argument), string is extended if necessary
 * 
 * @param dest Target string
 * @param src Source buffer with content, that should be copiend to the structure
 * @return string_t* (Extended) target string with copied value of src argument or NULL
 * @warning Always get pointer of the returned string (there may be reallocation)
 */
string_t *set_string(string_t **dest, char *src);


/**
 * @brief Extends string to the new bigger size
 * 
 * @param string String to be extended
 * @return string_t* Extende string or NULL
 */
string_t *ext_string(string_t *string);


/**
 * @brief Set value of string in string structure to the part of string pointed
 * by second argument (null termination is ignored in this case)
 * 
 * @param dest String to be set
 * @param src Source buffer 
 * @param n Amount of characters that should be copied to the array
 * @return string_t* string_t* (Extended) target string with copied value of src argument or NULL
  * @warning Always get pointer of the returned string (there may be reallocation)
 */
string_t *set_stringn(string_t **dest, char *src, size_t n);


/**
 * @brief Allocates new string structure
 * 
 * @param size Initial size of string
 * @return string_t* Newly allocated string
 */
string_t *new_string(size_t size);


/**
 * @brief Converts string_slice structure (that basically just points to the
 * part of some buffer) to the newly allocated string
 * 
 * @param slice Slice from which the new string hsould be created
 * @return string_t* Newly created string described by slice in argument or NULL
 */
string_t *slice_to_string(string_slice_t *slice);


/**
 * @brief Deallocates string structure
 * 
 * @param string String to be deallocated
 */
void string_dtor(string_t *string);


#endif