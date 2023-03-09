SRCS=$(wildcard *.cpp)
NAME=rice50

all:
	g++ -Wall -Wextra -O3 $(SRCS) -o $(NAME) -std=c++17 -flto
debug:
	g++ -Wall -Wextra -O3 $(SRCS) -o $(NAME) -g3 -flto -std=c++17