#!/bin/bash
#Makefile for generator and sauna binaries
all: generator sauna

gerador: generator.c
   gcc -Wall -o generator generator.c -lpthread
sauna: sauna.c
   gcc -Wall -o sauna sauna.c -lpthread

clean:
    rm -f generator sauna
    rm -f /tmp/entrada
    rm -f /tmp/rejeitados


