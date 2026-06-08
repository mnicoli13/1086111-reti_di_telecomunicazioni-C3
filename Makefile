CC        = g++
CFLAGS    = -g -Wno-write-strings
LIBFLAG   = -lm

HDRS      = global.h \
            mqueue.h \
            rand.h \
            simulator.h \
            calendar.h \
            packet.h \
            event.h \
            easyio.h \
            stat.h \
            buffer.h

OBJS      = main.o \
            mqueue.o \
            rand.o \
            simulator.o \
            calendar.o \
            packet.o \
            event.o \
            easyio.o \
            stat.o \
            buffer.o

PROGRAM   = c3sim

SRCS      = main.c \
            mqueue.c \
            rand.c \
            simulator.c \
            calendar.c \
            packet.c \
            event.c \
            easyio.c \
            stat.c \
            buffer.c

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	@echo "Loading $(PROGRAM) ... "
	@$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBFLAG)
	@echo "done"

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS) $(PROGRAM)

###
main.o:      global.h mqueue.h simulator.h
mqueue.o:    global.h mqueue.h simulator.h event.h calendar.h buffer.h rand.h easyio.h
rand.o:      global.h rand.h
simulator.o: simulator.h
calendar.o:  calendar.h global.h event.h
packet.o:    packet.h global.h
buffer.o:    buffer.h packet.h
event.o:     event.h buffer.h calendar.h rand.h global.h
stat.o:      stat.h
easyio.o:    easyio.h
