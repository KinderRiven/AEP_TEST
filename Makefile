all:
	g++ -O3 -std=c++11 ntstore/ntstore.c main.cc pflush.c -Intstore -Ipmdk/include -o main

use_pmdk:
	g++ -O3 -std=c++11 main.cc pflush.c ntstore/ntstore.c -Intstore -Ipmdk/include -o main -lpmem -lpthread
