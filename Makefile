BINDIR = bin
BINARY = relstat

CFLAGS  = -std=c17 -O3 -fPIC -Wall -Wextra
LDFLAGS = -lcurl -ljansson

$(BINDIR)/$(BINARY): clean
	$(CC) -o $@ main.c github.c $(CFLAGS) $(LDFLAGS)

$(BINDIR):
	mkdir -p $@

.PHONY: clean
.clean:
	rm -f $(BINDIR)/$(BINARY)
