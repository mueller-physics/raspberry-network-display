#
# TODO: This Makefile is very basic, it
# definitely misses HEADER FILE DEPENDENCIES
#
# Thus, if you change a header file, depending
# source code will NOT be recompiled automatically.
#   
# gcc's dependency file generator should be able
# to solve that.

%.o : %.c
	gcc -c -O3 -Wall -o $@ $<


OBJ1 := handleimages.o ../external/lodepng.o network-udp.o network-tcp.o misc.o main.o
OBJ2 := network-udp.o udp-cli-sender.o
OBJ3 := network-udp.o send-udp-on-ttl.o

default : fbrunner udp-cli-sender 

fbrunner : $(OBJ1)
	gcc $(OBJ1) -O3 -lpthread -o $@

udp-cli-sender : $(OBJ2)
	gcc $(OBJ2) -O3 -o $@

send-udp-on-ttl: $(OBJ3)
	gcc $(OBJ3) -O3 -lwiringPi -lpthread -o $@



# On standard clean, do not remove lodepng.o... it takes forever to
# build on the Raspberry, and should not change anyway
clean:
	rm -f *.o fbrunner udp-cli-sender *.tar

clean-all:
	rm -f *.o fbrunner udp-cli-sender *.tar
	rm -f ../external/*.o    

# TODO: re-enable the "tar up the project" feature?
#tar:
#	tar -cvf RaspberrySimControl_$(shell date +%Y%m%d-%H%M).tar *.c *.h Makefile 


