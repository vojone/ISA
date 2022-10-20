# ISA project - Feedreader
# Author: Vojtěch Dvořák

APP_NAME = feedreader
CFLAGS = -std=c99 -Wall -Wextra -pedantic -I/usr/include/libxml2
CC = gcc
SRCS = $(APP_NAME).c common.c cli.c http.c feed.c url.c
HEADERS = $(APP_NAME).h common.h cli.h http.h feed.h url.h
LDLIBS = -lxml2 -lssl -lcrypto

TEST_SCRIPT_NAME = feedreadertest.sh
TEST_FOLDER_NAME = tests

ARCHIVE_NAME = xdvora3o.tar
IN_ARCHIVE = $(SRCS) $(HEADERS) README Makefile \
$(TEST_SCRIPT_NAME) $(TEST_FOLDER_NAME)


.PHONY: all feedreader debug clean

all: $(APP_NAME)

$(APP_NAME): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDLIBS)

debug: CFLAGS := $(CFLAGS) -g 
debug: feedreader


test:
	./$(TEST_SCRIPT_NAME)

tar:
	tar -cf $(ARCHIVE_NAME) $(IN_ARCHIVE)

clean:
	rm -f $(APP_NAME) $(ARCHIVE_NAME)