# A few common Makefile items

CC := gcc
CXX := g++

UNAME := $(shell uname)

LIB_NAME := pmcheck
LIB_SO := lib$(LIB_NAME).so

CFLAGS := -Wall -O0 -g
CPPFLAGS += -Wall -O0 -g

# Mac OSX options
ifeq ($(UNAME), Darwin)
CPPFLAGS += -D_XOPEN_SOURCE -DMAC
endif
