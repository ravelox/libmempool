CC      ?= gcc
OS       = $(shell uname)
CCOPTS	 = -g -fPIC
LIBS	 = 
INCS	 = 

ifeq ($(OS),Darwin)
	LIB_EXT      = dylib
	SHARED_FLAG  = -dynamiclib
	RPATH_FLAG   = -Wl,-rpath,@loader_path
	SONAME_FLAG  =
else
	LIB_EXT      = so
	SHARED_FLAG  = -shared
	RPATH_FLAG   = -Wl,-rpath,'$$ORIGIN'
	SONAME_FLAG  = -Wl,-soname,libmempool.$(LIB_EXT)
endif

BINARY	 = pool_test
LIBRARY	 = libmempool.$(LIB_EXT)

SRC	 = pool.c pool_test.c
OBJ	 = $(SRC:.c=.o)

all: $(BINARY)

$(LIBRARY): pool.o
	$(CC) $(SHARED_FLAG) $(SONAME_FLAG) -o $@ $^

$(BINARY): $(LIBRARY) pool_test.o
	$(CC) $(CCOPTS) -o $(BINARY) pool_test.o -L. -lmempool $(RPATH_FLAG) $(LIBS)

.c.o:
	$(CC) -c $(CCOPTS) $< $(INCS)

clean:
	rm -f $(BINARY) $(OBJ) $(LIBRARY)
