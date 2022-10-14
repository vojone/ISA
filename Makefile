# ISA project - Feedreader
# Author: Vojtěch Dvořák

APP_NAME = feedreader
CFLAGS = -std=c99 -Wall -Wextra -pedantic -I/usr/include/libxml2
CC = gcc
SRCS = $(APP_NAME).c common.c cli.c http.c
HEADERS = common.h cli.h http.h
LDLIBS = -lxml2 -lssl -lcrypto


.PHONY: all feedreader debug clean

all: $(APP_NAME)

$(APP_NAME): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDLIBS)

debug: CFLAGS := $(CFLAGS) -g 
debug: feedreader

clean:
	rm -f $(APP_NAME)