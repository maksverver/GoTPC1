CFLAGS=-Wall -ansi -g -O3
CFLAGS+=-DREVISION=`svn info | grep Revision | cut -d\  -f 2`
LDLIBS=-lX11

CFLAGS+=-pg
LDFLAGS=-pg

CHECKER_OBJS=Checker.o Base.o Gui.o
PLAYER_OBJS=Player.o Base.o Gui.o
MANUAL_OBJS=Manual.o Base.o Gui.o

all: checker manual player

checker: $(CHECKER_OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o checker $(CHECKER_OBJS)

manual: $(MANUAL_OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o manual $(MANUAL_OBJS)

player: $(PLAYER_OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o player $(PLAYER_OBJS)

clean:
	-rm *.o checker manual player

