/**
 * @file feed.c
 * @brief Implementation of feed module (mod. for parsing and printing XML with
 * RSS2/Atom feeds)
 * 
 * @author Vojtěch Dvořák (xdvora3o)
 * @date 15. 10. 2022 
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


bool hasName(xmlNodePtr node, const char *name) {
    return !xmlStrcasecmp(node->name, (const xmlChar *)name); //< Tag names are case insensitive
}


int parse_atom_entry(feed_el_t *cur_feed, xmlNodePtr entry) {
    xmlNodePtr child, sub_child;

    child = entry->children;

    while(child) {
        if(hasName(child, "title")) {
            if(!(cur_feed->title = xmlNodeGetContent(child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(child, "updated")) {
            if(!(cur_feed->updated = xmlNodeGetContent(child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(child, "link")) {
            if(!(cur_feed->url = xmlGetProp(child, (const xmlChar *)"href"))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(child, "author")) {
            sub_child = child->children;

            while(sub_child) {
                if(hasName(sub_child, "name")) {
                    if(!(cur_feed->auth_name = xmlNodeGetContent(sub_child))) {
                        printerr(FEED_ERROR, "");
                        return FEED_ERROR;
                    }
                }

                sub_child = sub_child->next;
            }
        }

        child = child->next;
    }

    return SUCCESS;
}


int parse_atom(xmlNodePtr root, feed_doc_t *feed_doc) {
    int ret;
    feed_el_t *cur_feed;
    xmlNodePtr root_child = root->children;

    while(root_child) {
        if(hasName(root_child, "title")) {
            if(!(feed_doc->src_name = xmlNodeGetContent(root_child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(root_child, "entry")) {
            if(!(cur_feed = new_feed(feed_doc))) {
                printerr(INTERNAL_ERROR, "Unable to allocate memory for feed structure!");
                return INTERNAL_ERROR;
            }
            
            ret = parse_atom_entry(cur_feed, root_child);
            if(ret != SUCCESS) {
                return ret;
            }
        }

        root_child = root_child->next;
    }

    return SUCCESS;
}


int parse_rss_item(xmlNodePtr item, feed_el_t *cur_feed) {
    xmlNodePtr item_child = item->children;

    while(item_child) {
        if(hasName(item_child, "title")) {
            if(!(cur_feed->title = xmlNodeGetContent(item_child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(item_child, "link")) {
            if(!(cur_feed->url = xmlNodeGetContent(item_child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(item_child, "pubDate")) { //?
            if(!(cur_feed->updated = xmlNodeGetContent(item_child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        else if(hasName(item_child, "author")) { //?
            if(!(cur_feed->auth_name = xmlNodeGetContent(item_child))) {
                printerr(FEED_ERROR, "");
                return FEED_ERROR;
            }
        }
        
        item_child = item_child->next;
    }

    return SUCCESS;
}


int parse_rss(xmlNodePtr root, feed_doc_t *feed_doc) {
    int ret;
    feed_el_t *cur_feed;
    xmlNodePtr channel = root->children, channel_child;  

    xmlChar *v = xmlGetProp(root, (const xmlChar *)"version");
    if(!v) {
        printerr(FEED_ERROR, "Missing mandatory version attribute of 'rss' in rss tag!");
        return FEED_ERROR;
    }

    bool is_supported = !xmlStrcasecmp(v, (const xmlChar *)RSS_VERSION);
    xmlFree(v);

    if(!is_supported) {
        printerr(FEED_ERROR, "Unsupported version of RSS. Supported '%s' got '%s'!", RSS_VERSION, v);
        return FEED_ERROR;
    }

    while(channel) {
        if(hasName(channel, "channel")) {
            channel_child = channel->children;

            while(channel_child) {
                if(hasName(channel_child, "title")) {
                    feed_doc->src_name = xmlNodeGetContent(channel_child);
                    if(!feed_doc->src_name) {
                        printerr(FEED_ERROR, "");
                        return FEED_ERROR;
                    }
                }
                else if(hasName(channel_child, "item")) {
                    if(!(cur_feed = new_feed(feed_doc))) {
                        printerr(INTERNAL_ERROR, "Unable to allocate memory for feed structure!");
                        return INTERNAL_ERROR;
                    }

                    ret = parse_rss_item(channel_child, cur_feed);
                    if(ret != SUCCESS) {
                        return ret;
                    }
                }

                channel_child = channel_child->next;
            }
        }

        channel = channel->next;
    }

    return SUCCESS;
}


int parse_feed_doc(feed_doc_t *feed_doc, char *feed, char *url) {
    int ret;

    xmlDocPtr xml = xmlReadMemory(feed, strlen(feed), url, NULL, 0);
    if(!xml) {
        printerr(INTERNAL_ERROR, "Unable to parse XML document from '%s'!", url);
        return INTERNAL_ERROR;
    }

    xmlNodePtr root = xmlDocGetRootElement(xml);
    if(!root) {
        printerr(INTERNAL_ERROR, "Unable to find root node of XML document from '%s'!", url);
        return INTERNAL_ERROR;
    }

    if(hasName(root, "feed")) {
        ret = parse_atom(root, feed_doc);
    }
    else if(hasName(root, "rss")) {
        ret = parse_rss(root, feed_doc);
    }
    else {
        printerr(FEED_ERROR, "Unexpected name of root element of XML from '%s'! Expected feed/rss", url);
        ret = FEED_ERROR;
    }

    xmlFreeDoc(xml);

    return ret;
}


void print_feed_doc(feed_doc_t *feed_doc, settings_t *settings) {
    printf("*** %s ***\n", feed_doc->src_name);

    feed_el_t *feed = feed_doc->feed;
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
}