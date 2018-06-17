GCC_FLAGS = -Wall
SOURCE=reader.c
SOURCE2=writer.c

all: $(SOURCE) $(SOURCE2)
	make client
	make server

server: $(SOURCE)
	gcc $(GCC_FLAGS) $(SOURCE) -o server

client: $(SOURCE2)
	gcc $(GCC_FLAGS) $(SOURCE2) -o client
