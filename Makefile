all: memsim

memsim: memsim.c
	gcc -Wall -Wextra -std=c17 -o memsim memsim.c

clean:
	del memsim.exe 2>nul || rm -f memsim
