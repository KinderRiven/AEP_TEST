all:
	g++ -O0 -std=c++11 main.cc pflush.c -Ipmdk/include -o main

use_pmdk:
	g++ -O0 -std=c++11 main.cc pflush.c -Ipmdk/include -o main -lpmem -lpthread
