# ISA project - Feedreader
# Author: Vojtěch Dvořák

APP_NAME = feedreader
SRCS = $(APP_NAME).c common.c cli.c http.c feed.c url.c
HEADERS = $(APP_NAME).h common.h cli.h http.h feed.h url.h

# Compiling
CC = gcc
LDLIBS = -lssl -lcrypto
CFLAGS = -std=c11 -Wall -Wextra -pedantic

# Adding libraries and
CFLAGS := $(CFLAGS) `xml2-config --cflags`
LDLIBS := $(LDLIBS) `xml2-config --libs`

# Tests
TEST_SCRIPT_NAME = feedreadertest.sh
TEST_FOLDER_NAME = tests

# Archive
ARCHIVE_NAME = xdvora3o.tar
IN_ARCHIVE = $(SRCS) $(HEADERS) README Makefile \
$(TEST_SCRIPT_NAME) $(TEST_FOLDER_NAME)


.PHONY: all feedreader debug clean

all: $(APP_NAME)

$(APP_NAME): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDLIBS)

debug: CFLAGS := $(CFLAGS) -g 
debug: feedreader


test: $(APP_NAME)
	./$(TEST_SCRIPT_NAME)

tar:
	tar -cf $(ARCHIVE_NAME) $(IN_ARCHIVE)

clean:
	rm -f $(APP_NAME) $(ARCHIVE_NAME)
	