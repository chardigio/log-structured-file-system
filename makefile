all: Lab7.o
	g++ -std=c++11	-g Lab7.o -o Lab7

Lab7.o:	Lab7.cpp commands.cpp functions.cpp
	g++ -std=c++11 -g -c Lab7.cpp

DriveInit.o: DriveInit.cpp
	g++ -std=c++11 -g -c DriveInit.cpp

DriveLink: DriveInit.o
	g++ -std=c++11 -g DriveInit.o -o DriveInit

drive: DriveLink
	./DriveInit

clean:
	rm -f *.o *~ *.swp Lab7 DriveInit s b;
	rm -rf DRIVE/;

again:	clean drive all
