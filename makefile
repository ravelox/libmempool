CCOPTS	=	-g
LIBS	=	
INCS	=

COMPONENTS	=	\
			pool_test \
			pool

BINARY		=	pool_test

SRC		=	$(COMPONENTS:=.c)
OBJ		=	$(SRC:.c=.o)

$(BINARY):      $(OBJ)
		gcc $(CCOPTS) -o $(BINARY) $(OBJ) $(LIBS)

.c.o:
	gcc -c $(CCOPTS) $< $(INCS)

clean:
	rm -f $(BINARY) $(OBJ)
