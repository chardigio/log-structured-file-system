all:	Lab7.o
	g++ -std=c++11	-g Lab7.o -o Lab7

Lab7.o:	Lab7.cpp Lab7.h
	g++ -std=c++11 -g -c Lab7.cpp

Lab7: Lab7.o Lab7.h
	g++ -std=c++11 -g -c Lab7.o -o Lab7

DriveInit.o: DriveInit.cpp
	g++ -std=c++11 -g -c DriveInit.cpp

DriveLink: Lab7.h DriveInit.o
	g++ -std=c++11 -g DriveInit.o -o DriveInit

drive: DriveLink
	rm -rf DRIVE/;
	./DriveInit

clean:
	rm -f *.o *~ *.swp Lab7 DriveInit

again:	clean all
