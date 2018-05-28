FLAGS= -Wall
SOURCE1 = barber.c
SOURCE2 = client.c

main: $(SOURCE1) $(SOURCE2)
	make client
	make barber

barber: $(SOURCE1)
	gcc $(FLAGS) $(SOURCE1) -o barber

client: $(SOURCE2)
	gcc $(FLAGS) $(SOURCE2) -o client

