#
# Makefile of v4l2-sample
#

TARGET = v4l2-sample

CC ?= ${CROSS_COMPILE}gcc
CFLAGS = -Wall -Wextra -O3
LDLIBS = -ljpeg

# Compilation
all: $(TARGET)

$(TARGET): camera2jpeg.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Nettoyage
clean:
	rm -f *~ *.o $(TARGET)

# Installation
install: $(TARGET)
	install -d ${DESTDIR}${BINDIR}
	install -m 0755 $(TARGET) ${DESTDIR}${BINDIR}