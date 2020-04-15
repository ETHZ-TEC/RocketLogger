CC=gcc
RM=rm -f

# libraries and includes
LIBS=-lzmq

INCS=


all: zmq_pubber

zmq_pubber: zmq_pubber.c
	$(CC) $(LIBS) $(INCS)-o $@ $<


.PHONY: clean
clean:
	$(RM) zmq_pubber
