SRCS=$(wildcard *.cpp)
NAME=rice50

all:
	g++ -Wall -Wextra -Ofast $(SRCS) -o $(NAME)

debug:
	g++ -Wall -Wextra -Ofast $(SRCS) -o $(NAME) -g