.DEFAULT_GOAL = CSIServer
CC := gcc
LDFLAGS := -lpthread

CSIServer: CSIServer_ng.c
	${CC} CSIServer_ng.c ${LDFLAGS} -o CSIServer

.PHONY: clean
clean:
	rm CSIServer

