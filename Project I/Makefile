DEBUG= -O2 
CC= gcc 
INCLUDE= -I/usr/local/include 
CFLAGS= $(DEBUG) -Wall $(INCLUDE) -Winline 
LDFLAGS= -L/usr/local/lib 
LIBS= -lpthread -lm 

SRC = myprogram.c

OBJ = $(SRC:.c=.o)
BIN = $(SRC:.c=)

$(BIN): $(OBJ)
	@echo [link] $@
	$(CC) -o $@ $< $(LDFLAGS) $(LIBS)

.c.o:
	@echo [Compile] $<
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm -f $(OBJ) $(BIN)
