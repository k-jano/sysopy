//array
gcc -Wall -O0 -c array.c

//main
gcc -Wall -O0 -c main.c

//Static
ar rcs lib_array.a array.o
gcc -Wall -O0 -static main.c -L. -l_array -o main_static

//Shared
gcc -Wall -O0 -fPIC -c array.c
gcc -Wall -O0 -fPIC -shared -o lib_array.so array.o
gcc -Wall -O0 -Wl,-rpath=./ main.o -L. -l_array -o main_shared

//Dynamic
gcc -Wall -O0 -fPIC -c array.c
gcc -Wall -O0 -fPIC -shared -o lib_array.so array.o
gcc -Wall -O0 -Wl,-rpath=./,--no-as-needed main.c -L. -ldl -D _DYNAMIC -o main_dynamic

//Clear
rm -f *.o *.a *.so *.exe


