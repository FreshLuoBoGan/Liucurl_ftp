#CUR_DIR  = $(shell pwd)

ROUTE_FTP = ftp

OBJS = main.o $(ROUTE_FTP)/ftp.o

CLIB1 = -lpthread

edit : $(OBJS)
	g++ -o edit $(OBJS) $(CLIB1) -lcurl

main.o : main.cpp
	g++ -c main.cpp -o main.o -lcurl

$(ROUTE_FTP)/ftp.o : $(ROUTE_FTP)/ftp.cpp $(ROUTE_FTP)/ftp.h
	g++ -c $(ROUTE_FTP)/ftp.cpp -o $(ROUTE_FTP)/ftp.o -lcurl

clean : 
	rm -rf $(OBJS) edit

