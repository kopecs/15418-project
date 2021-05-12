CC = clang
CFLAGS = -Wall -Wextra -g #-fsanitize=thread
ODIR = obj
BDIR = bld
SDIR = src
SRCS = $(wildcard $(SDIR)/*.c)
OBJS = $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(SRCS))
LIBS = -lpthread

.PHONY: all
all: thr

.PHONY: thr
thr: $(BDIR)/thr

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/thr: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: format
format:
	clang-format -i -style=file $(SRCS)

.PHONY: clean
clean:
	rm -f obj/*
	rm -f bld/*
