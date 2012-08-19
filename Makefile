CC         = gcc
CFLAGS     = -ggdb -Wall
OUTFLAG    = -o
LDFLAGS    = 		
LIBS       = -lconfig			
OBJS       = md5c.o
TARGET     = filewatch

all:  $(TARGET)

.c.o : 
	$(CC) $(CFLAGS) -c $< $(INCLUDEDIR) 

libconfig: 
	cd libconfig-1.4.8;
	./configure;make;sudo make install

filewatch: $(OBJS) filewatch.c inifile.c md5c.c
	$(CC) $(CFLAGS) $(OUTFLAG) $@ filewatch.c $(OBJS) $(INCLUDEDIR) $(LIBDIR) $(LDFLAGS) $(LIBS) 

clean:
	rm -f *.o *~ TAGS core $(TARGET)
	cd libconfig-1.4.8;make clean

tags:
	etags *.[ch] *.[ch] 


commit:
	git add *
	git commit -m


push:
	 git push origin version2


run: filewatch
	clear;./$(TARGET) test.cfg


