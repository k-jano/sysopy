GCC_FLAGS = -Wall
SOURCE=proc_interpreter2.c
LOOP=loop.c
ALLOCATION=allocation.c


main: $(SOURCE) $(LOOP) $(ALLOCATION)
	gcc $(GCC_FLAGS) $(SOURCE) -o main
	gcc $(GCC_FLAGS) $(LOOP) -o main_loop
	gcc $(GCC_FLAGS) $(ALLOCATION) -o main_allocation

