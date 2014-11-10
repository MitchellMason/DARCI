NAME=DARCI

all: 
	g++ -g -o $(NAME) -F /Library/Frameworks/ -framework SDL2 main.cpp
net: 
	g++ -g -o net.o network.cpp
clean:
	rm -rf $(NAME)
