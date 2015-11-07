CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LIBS = -lusb-1.0 -lpthread -lrt
CFLAGS = -Wall -Iinclude/
CPPFLAGS = -Wall
LDFLAGS = 
LIST_DEVS_OBJS = list-devs.o
CDC_ACM_OBJS = cdc-acm.o

all: list-devs cdc-acm

list-devs: ${LIST_DEVS_OBJS}
	$(CC) -o $@ ${LIST_DEVS_OBJS} ${LIBS} ${LDFLAGS}
cdc-acm: ${CDC_ACM_OBJS}
	$(CC) -o $@ ${CDC_ACM_OBJS} ${LIBS} ${LDFLAGS}

%.o: %.c
	$(CC) -c $< ${CFLAGS}
%.o: %.cpp
	$(CPP) -c $< ${CPPFLAGS}
clean:
	rm -f list-devs cdc-acm ${LIST_DEVS_OBJS} ${CDC_ACM_OBJS}

