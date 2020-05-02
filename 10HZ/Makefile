INCLUDES=	-I/usr/local/opencv/include

CC=	g++

CFLAGS=	-O0	-g	-std=c++11	

CPPLIBS= -L/usr/lib -lopencv_core -lopencv_flann -lopencv_video -lpthread -lrt 
 
CPPFILES= code_10hz.cpp

CPPOBJS= ${CPPFILES:.cpp=.o}

build:	code_10hz

clean:
	-rm -f *.o *.d *.jpg *.ppm
	-rm -f code_10hz

code_10hz:	code_10hz.o	
	$(CC)	$(LDFLAGS)	$(INCLUDES)	$(CFLAGS)	-o	code_10hz	code_10hz.o	`pkg-config --libs opencv`	$(CPPLIBS)
