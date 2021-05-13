CC = clang
CFLAGS = -Wall -Wextra -fPIC
ODIR = obj
BDIR = bld
SDIR = src
SRCS = $(wildcard $(SDIR)/*.c)
OBJS = $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(SRCS))
LIBS = -lpthread

C0_ROOT = "/usr/lib/c0"

.PHONY: all
all: thr

.PHONY: thr
thr: $(BDIR)/libthr.so

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/libthr.so: $(OBJS)
	$(CC) -shared -Wl,-soname,libthr.so -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: format
format:
	clang-format -i -style=file $(SRCS)

.PHONY: clean
clean:
	rm -f obj/*
	rm -f bld/*

.PHONY: install
install: 
	install $(SDIR)/thr.h0 $(BDIR)/libthr.so $(C0_ROOT)/lib/
