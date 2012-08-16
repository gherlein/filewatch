CC         = gcc
CFLAGS     = -ggdb
OUTFLAG    = -o
LDFLAGS    = 		
LIBS       = 			
OBJS       = inifile.o md5c.o
TARGET     = filewatch

all:  $(TARGET)

.c.o : 
	$(CC) $(CFLAGS) -c $< $(INCLUDEDIR) 

filewatch: $(OBJS) filewatch.c inifile.c md5c.c
	$(CC) $(CFLAGS) $(OUTFLAG) $@ filewatch.c $(OBJS) $(INCLUDEDIR) $(LIBDIR) $(LDFLAGS) $(LIBS) 

clean:
	rm -f *.o *~ TAGS core $(TARGET)

tags:
	etags *.[ch] *.[ch] 


commit:
	git commit


push:
	 git push --all
