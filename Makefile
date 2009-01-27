CC         = gcc
CFLAGS     = -ggdb
OUTFLAG    = -o
LDFLAGS    = 		
LIBS       = 			
OBJS       =
TARGET     = filewatch

all:  $(TARGET)

.c.o : 
	$(CC) $(CFLAGS) -c $< $(INCLUDEDIR) 

filewatch: $(OBJS) filewatch.c
	$(CC) $(CFLAGS) $(OUTFLAG) $@ filewatch.c $(OBJS) $(INCLUDEDIR) $(LIBDIR) $(LDFLAGS) $(LIBS) 

clean:
	rm -f *.o *~ TAGS  $(TARGET)

tags:
	etags *.[ch] *.[ch] 
