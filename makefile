SRCS=$(wildcard *.cpp)
NAME=rice50

all:
	g++ -Wall -Wextra -O3 $(SRCS) -o $(NAME)

debug:
	g++ -Wall -Wextra -O3 $(SRCS) -o $(NAME) -g