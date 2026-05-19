CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 $(shell pkg-config --cflags glib-2.0 gio-2.0)
LIBS    = $(shell pkg-config --libs glib-2.0 gio-2.0) -lm

TARGETS = telemetry_provider_daemon
# TARGETS = telemetry_provider_daemon diagnostic_dashboard_client

.PHONY: all clean

all: $(TARGETS)

telemetry_provider_daemon: telemetry_provider_daemon.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# diagnostic_dashboard_client: diagnostic_dashboard_client.c
# 	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGETS)


debug: CFLAGS += -DDEBUG_ENABLED
debug: $(TARGETS)	