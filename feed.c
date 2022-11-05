/**
 * @file feed.c
 * @brief Implementation of feed module (mod. for parsing and printing XML with
 * RSS2/Atom feeds)
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 5. 11. 2022 
 */

#include "feed.h"


void xml_parser_init() {
    LIBXML_TEST_VERSION
}


void xml_parser_cleanup() {
    xmlCleanupParser();
}


void init_feed_doc(feed_doc_t *feed_doc) {
    feed_doc->src_name = NULL;
    feed_doc->feed = NULL;
}


void add_feed(feed_doc_t *feed_doc, feed_el_t *new_feed) {
    feed_el_t **feed = &(feed_doc->feed);

    while(*feed) { //< Go to the end of the linked list
        feed = &((*feed)->next);
    }

    *feed = new_feed;
}


feed_el_t *new_feed(feed_doc_t *feed_doc) {
    feed_el_t *new_feed_ = (feed_el_t *)malloc(sizeof(feed_el_t));
    if(!new_feed_) {
        return NULL;
    }

    memset(new_feed_, 0, sizeof(feed_el_t)); //< Initialization of structure

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

    while(feed) { //< Deallocate whole linked list
        tmp = feed;
        feed = feed->next;

        feed_dtor(tmp);
    }
}


/**
 * @brief Determines whether XML node has given name or not
 * 
 * @param node Node to be checked
 * @param name String against the check is done 
 * @return true If tag name of node is same as given string
 * @return false Otherwise
 */
bool hasName(xmlNodePtr node, const char *name) {
    return !xmlStrcasecmp(node->name, (const xmlChar *)name); //< Tag names are case insensitive
}


/**
 * @brief Safely sets field of feed structure, if there is any value
 * already it is freed
 * 
 * @param field Ptr to the field to be set
 * @param new_content Ptr to the new content of the field
 * @param tag Tagname of original tag, from which it content comes from 
 * @return int SUCCESS or FEED_ERROR if new_content is NULL
 */
int set_feed_field(xmlChar **field, xmlChar *new_content, const char *tag) {
    if(!new_content) {
        printerr(FEED_ERROR, "Nepovedlo se ziskat obsah znacky '%s'!", tag);
        return FEED_ERROR;
    }

    if(*field) { //< Free current content to avoid memory leaks
        xmlFree(*field);
        *field = NULL;
    }

    *field = new_content;

    return SUCCESS;
}


/**
 * @brief Parses Atom HTML author structure and extracts name from it
 * 
 * @param author The xml node that represents author structure
 * @param cur_feed The ptr to the structure, that should be filled with author name
 * @return SUCCESS if everything went OK (even if the name of the author was not found)  
 */
int parse_atom_author(xmlNodePtr author, feed_el_t *cur_feed) {
    int ret = SUCCESS;
    
    xmlNodePtr sub_child = author->children;

    while(sub_child) {
        xmlChar *sub_content = xmlNodeGetContent(sub_child);

        if(hasName(sub_child, "name")) {
            ret = set_feed_field(&(cur_feed->auth_name), sub_content, "name");
        }
        else {
            if(sub_content) xmlFree(sub_content);
        }
        if(ret != SUCCESS) {
            break;
        }

        sub_child = sub_child->next; //< Go to the next sibling
    }

    return ret;
}


/**
 * @brief Parses atom entry XML structure
 * 
 * @param cur_feed Current feed structure to be filled with data 
 * @param entry 'Root' node of entry
 * @return int SUCCESS or FEED_ERROR
 */
int parse_atom_entry(feed_el_t *cur_feed, xmlNodePtr entry) {
    int ret = SUCCESS;
    xmlNodePtr child;

    child = entry->children;

    while(child) { //< Try to find given tags
        xmlChar *content = xmlNodeGetContent(child); //< Get content of child (it can be NULL)

        if(hasName(child, "title")) {
            ret = set_feed_field(&(cur_feed->title), content, "title");
        }
        else if(hasName(child, "updated")) {
            ret = set_feed_field(&(cur_feed->updated), content, "updated");
        }
        else if(hasName(child, "link")) {
            xmlChar *link = xmlGetProp(child, (xmlChar *)"href");
            xmlChar *rel = xmlGetProp(child, (xmlChar *)"rel");

            bool is_alt = !rel || !xmlStrcasecmp(rel, (xmlChar *)"alternate");
            if(is_alt || !(cur_feed->url)) { //< Set the link URL only if link was not defined yet or rel has default value ("alternate") or via
                if(link) {
                    ret = set_feed_field(&(cur_feed->url), link, "link");
                }
            }
            else if(link) {
                xmlFree(link);
            }
            
            if(rel) {
                xmlFree(rel);
            }
            
            if(content) xmlFree(content);
        }
        else if(hasName(child, "author")) { //< Go inside author tag (there can be name and email)
            ret = parse_atom_author(child, cur_feed);
            if(content) xmlFree(content);
        }
        else {
            if(content) xmlFree(content);
        }
        if(ret != SUCCESS) {
            break;
        }

        child = child->next; //< Go to the next sibling
    }

    return ret;
}


/**
 * @brief Parses feed in atom format (parsing is done due to spec from RFC4287)
 * 
 * @param root Root node of XML document
 * @param feed_doc Feed document structure to be filled with data (output param)
 * @return int SUCCESS if everything went OK
 */
int parse_atom(xmlNodePtr root, feed_doc_t *feed_doc) {
    int ret = SUCCESS;
    feed_el_t *cur_feed;
    xmlNodePtr root_child = root->children;

    while(root_child) { //< Perform search in root child
        xmlChar *content = xmlNodeGetContent(root_child); //< Get content of child (it can be NULL)

        if(hasName(root_child, "title")) {
            ret = set_feed_field(&(feed_doc->src_name), content, "title");
        }
        else if(hasName(root_child, "entry")) { //< Entry was found
            if(!(cur_feed = new_feed(feed_doc))) {
                printerr(INTERNAL_ERROR, "Nepodarilo se alokovat strukturu pro novinku!");
                return INTERNAL_ERROR;
            }
            
            ret = parse_atom_entry(cur_feed, root_child); //< Parse it
            if(content) xmlFree(content);
        }
        else {
            if(content) xmlFree(content);
        }
        if(ret != SUCCESS) {
            break;
        }       

        root_child = root_child->next; //< Go to the next child of the root node
    }

    return ret;
}


/**
 * @brief Parses item structure of document in RSS format
 * 
 * @param item Pointer to the item node
 * @param cur_feed Feed sctructure to be filled with data
 * @return int SUCCESS if everything went OK
 */
int parse_rss_item(xmlNodePtr item, feed_el_t *cur_feed) {
    int ret = SUCCESS;
    xmlNodePtr item_child = item->children;

    while(item_child) {
        xmlChar *content = xmlNodeGetContent(item_child);

        if(hasName(item_child, "title")) {
            ret = set_feed_field(&(cur_feed->title), content, "title");
        }
        else if(hasName(item_child, "link")) {
            ret = set_feed_field(&(cur_feed->url), content, "link");
        }
        else if(hasName(item_child, "pubDate")) { //Equivalent of <published> (due to forum)
            ret = set_feed_field(&(cur_feed->updated), content, "pubDate");
        }
        else if(hasName(item_child, "author")) { //Equivalent of Atom <author> structure (due to forum)
            ret = set_feed_field(&(cur_feed->auth_name), content, "author");
        }
        else {
            if(content) xmlFree(content);
        }
        if(ret != SUCCESS) {
            break;
        }
        
        item_child = item_child->next; //< Go to the next sibling
    }

    return ret;
}


/**
 * @brief Parses XML document in RSS2.0 format (specificated e. g. here: https://validator.w3.org/feed/docs/rss2.html) 
 * 
 * @param root The root node of the document
 * @param feed_doc Feed document to be filled with data
 * @return int SUCCESS if everything went OK
 */
int parse_rss(xmlNodePtr root, feed_doc_t *feed_doc) {
    int ret = SUCCESS;
    feed_el_t *cur_feed;
    xmlNodePtr channel = root->children, channel_child;  

    xmlChar *v = xmlGetProp(root, (const xmlChar *)"version"); //< Get version attribute
    if(!v) {
        printerr(FEED_ERROR, "Chybejici atribut znacky 'rss' udavajici verzi RSS protokolu!");
        return FEED_ERROR;
    }

    bool is_supported = !xmlStrcasecmp(v, (const xmlChar *)RSS_VERSION); //< Check RSS version (mandatory tag)

    if(!is_supported) {
        printerr(FEED_ERROR, "Nepodoporovana verze RSS. Podporovana '%s', ziskana '%s'!", RSS_VERSION, v);
        xmlFree(v);
        return FEED_ERROR;
    }

    xmlFree(v);

    while(channel) { //< There should be maximum one and only one channel tag (but ,for better robustness, mutliple of them are accepted )
        if(hasName(channel, "channel")) {
            channel_child = channel->children;

            while(channel_child) { //< Search all nodes inside channel tag
                if(hasName(channel_child, "title")) {
                    xmlChar *cntnt = xmlNodeGetContent(channel_child);
                    ret = set_feed_field(&(feed_doc->src_name), cntnt, "title");
                }
                else if(hasName(channel_child, "item")) {
                    if(!(cur_feed = new_feed(feed_doc))) {
                        printerr(INTERNAL_ERROR, "Nepodarilo se alokovat pamet pro novinku!");
                        return INTERNAL_ERROR;
                    }
                    ret = parse_rss_item(channel_child, cur_feed);
                }
                if(ret != SUCCESS) {
                    return ret;
                }

                channel_child = channel_child->next; //< Go to the next child of channel tag
            }
        }

        channel = channel->next; //< Shouldn't be used (but for better robustness it is here)
    }

    return SUCCESS;
}


/**
 * @brief Selects the parsing function due to root element 
 */
int sel_parser(xmlNodePtr root, int exp_type, char *url, parse_f_ptr_t *func) {
    int real_mime;

    if(hasName(root, "feed")) {
        real_mime = ATOM;
        *func = parse_atom;
    }
    else if(hasName(root, "rss")) {
        real_mime = RSS;
        *func = parse_rss;
    }
    else {
        #ifdef FORMAT_STRICT
            printerr(FEED_ERROR, "Neocekavany nazev korenove znacky XML z '%s'! Ocekavano 'feed'/'rss'", url);
            return FEED_ERROR;
        #endif
    }

    if(real_mime != exp_type && exp_type != XML) { //< The it seems that mime type of document is wrong
        printw("Skutecny format dokumentu z '%s' se neshoduje s MIME typem HTTP odpovedi!", url);
    }

    return SUCCESS;
}


int parse_feed_doc(feed_doc_t *feed_doc, int exp_type, char *feed, char *url) {
    int ret;

    int xml_p_flags = XML_PARSE_HUGE | XML_PARSE_RECOVER | XML_PARSE_RECOVER;

    #ifndef DEBUG
        xml_p_flags |= XML_PARSE_NOERROR | XML_PARSE_NOWARNING;
    #endif

    xmlDocPtr xml = xmlReadMemory(feed, strlen(feed), url, NULL, xml_p_flags); //< Parse document by libxml2
    if(!xml) {
        printerr(FEED_ERROR, "Nepodarilo se provest analyzu dokumentu z '%s'!", url);
        return FEED_ERROR;
    }

    xmlNodePtr root = xmlDocGetRootElement(xml); //< Get The root element of XML doc
    if(!root) {
        printerr(FEED_ERROR, "Nepodarilo se najit korenovy prvek XML dokumentu z adresy '%s'!", url);
        xmlFreeDoc(xml);
        return FEED_ERROR;
    }

    //Determine the format of by the first tag the feed document
    parse_f_ptr_t parsing_function = NULL;
    ret = sel_parser(root, exp_type, url, &parsing_function);
    if(ret == SUCCESS) {
        ret = parsing_function(root, feed_doc);
    }

    xmlFreeDoc(xml);

    return ret;
}


bool is_known(xmlChar *field) {
    return field && strlen((char *)field);
}


void print_feed_doc(feed_doc_t *feed_doc, settings_t *settings) {
    
    printf("*** %s ***\n", is_known(feed_doc->src_name) ? (char *)feed_doc->src_name : "<neznamy zdroj>");

    feed_el_t *feed = feed_doc->feed;
    while(feed) {
        printf("%s\n", is_known(feed->title) ? (char*)feed->title : "<nepojmenovany prispevek>");

        if(is_known(feed->auth_name) && settings->author_flag) {
            printf("Autor: %s\n", feed->auth_name);
        }
        if(is_known(feed->url) && settings->asoc_url_flag) {
            printf("URL: %s\n", feed->url);
        }
        if(is_known(feed->updated) && settings->time_flag) {
            printf("Aktualizace: %s\n", feed->updated);
        }

        if(settings->author_flag ||  //< There is newline only if there are any additional information flag
            settings->asoc_url_flag || 
            settings->time_flag) {
            printf("\n"); 
        }

        feed = feed->next;
    }
}