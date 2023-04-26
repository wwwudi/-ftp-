TARGET:=client
CC:=gcc

CSRCS+=$(wildcard *.c)
CSRCS+=$(wildcard mp3/*.c)

OBJS+=$(patsubst %.c,%.o,$(CSRCS))

INC+= -I ../include
LIBS:= -lpthread

$(TARGET):$(OBJS)
	$(CC) $^ $(LIBS) -o $@

%.o:%.c
	$(CC) -c $< $(INC)  -o $@

clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
