mysql= -DUSING_MYSQL -I/usr/local/mysql/include/mysql -I/usr/include/mysql
debug= -O3

all: pow

OBJECTS= main.o \
	mongoose.o

%.o: %.c
	cc $(debug) $(mysql) -Wall $*.c -c -o $@

%.o: %.cpp
	g++ $(debug) $(mysql) -Wall $*.cpp -c -o $@

mongoose.o: 
	cc $(debug) -std=c99 -D_POSIX_SOURCE -D_BSD_SOURCE -c mongoose.c -o mongoose.o

models:
	g++ models/*.m

pow: $(OBJECTS)
	g++ $(debug) -Wall $(OBJECTS) -o pow $(mysql) -lmysqlclient -L/usr/local/mysql/lib/mysql
