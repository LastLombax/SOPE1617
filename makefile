#!/bin/bash
#Makefile for generator and sauna binaries
all: gerador sauna

gerador: generator.c
   gcc -Wall -o generator generator.c -lpthread
sauna: sauna.c
   gcc -Wall -o sauna sauna.c -lpthread

clean:
    rm -f gerador sauna
    rm -f /tmp/entrada
    rm -f /tmp/rejeitados


