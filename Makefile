mysql= -DUSING_MYSQL -I/usr/local/mysql/include/mysql -I/usr/include/mysql
debug= -O3

all: pow

%.o: %.c
	cc $(debug) $(mysql) -Wall $*.c -c -o $@

%.o: %.cpp
	g++ $(debug) $(mysql) -Wall $*.cpp -c -o $@

mongoose.o: 
	cc $(debug) -std=c99 -D_POSIX_SOURCE -D_BSD_SOURCE -c mongoose.c -o mongoose.o

models: make_models.o
	g++ $(debug) -Wall make_models.o -o make_models $(mysql) -lmysqlclient -L/usr/local/mysql/lib/mysql

OBJECTS= pow.o \
	models/locations.o \
	mongoose.o

pow: $(OBJECTS)
	g++ $(debug) -Wall $(OBJECTS) -o pow $(mysql) -lmysqlclient -L/usr/local/mysql/lib/mysql
