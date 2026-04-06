CC=arm-linux-gnueabihf-gcc
LIBS= -lpthread -lm
SRCS= color_sensor.c
BIN=app
CFLAGS+= -g -O0
OBJS=$(subst .c,.o,$(SRCS))

all : $(BIN)

$(BIN): $(OBJS)
	@echo [link] $@
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.c
	@echo [Compile] $<
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm -f $(OBJS) $(BIN)