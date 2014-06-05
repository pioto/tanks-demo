CFLAGS = $(shell pkg-config --cflags glu) -Wall
CXXFLAGS = $(shell pkg-config --cflags glu) -Wall
LDFLAGS = $(shell pkg-config --libs glu) -lglut

PROGS = tanks

all : $(PROGS)

clean :
	$(RM) -f $(PROGS)

