CFLAGS = -O -Wall -fPIC -ansi
ROOT = $(shell $(ROOTSYS)/bin/root-config --glibs) $(shell $(ROOTSYS)/bin/root-config --cflags)

all: add_tree.c add_tree.h
	g++ add_tree.c $(CFLAGS) $(ROOT) -o ../bin/add_tree
