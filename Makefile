BINDIR = bin
BINARY = relstat

VERSION = 0.1.0

CFLAGS  = -std=c23 -O3 -fPIC -Wall -Wextra -Dbin_name=$(BINARY) \
	-Drelstat_version=$(VERSION) -Dgit_sha=$(shell git rev-parse HEAD)
LDFLAGS = -lcurl -ljansson

$(BINDIR)/$(BINARY): $(BINDIR) clean
	$(CC) -o $@ main.c github.c $(CFLAGS) $(LDFLAGS)

$(BINDIR):
	mkdir -p $@

.PHONY: dash
dash: $(BINDIR)
	$(CC) -o $(BINDIR)/$@ dash.c $(CFLAGS) -lncurses

.PHONY: clean
.clean:
	rm -f $(BINDIR)/*
