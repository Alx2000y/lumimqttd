#CC=clang -flto
CC?=gcc
ifndef CFLAGS
#CFLAGS = -O2 -g -I.
#CFLAGS = -g -I. -rdynamic
CFLAGS = -g -I.
#CFLAGS = -g3 -o0 -ggdb -g -I. 
endif
TARGET = lumimqttd 
LIBS+= -lmosquitto -lasound -ljson-c -lpthread -lssl -lcrypto
SOURCES = cfg.c log.c lumimqttd.c tts.c volume.c leds.c
HEADERS = cfg.h log.h lumimqttd.h tts.h volume.h leds.h

ifdef USE_MPD
LIBS+= -lmpdclient
SOURCES+= mpd.c
HEADERS+= mpd.h
else
LIBS+= -lmpg123 -lout123
SOURCES+= player.c
HEADERS+= player.h
endif
ifdef USE_BLE
LIBS+= -lbluetooth
SOURCES+= ble.c
HEADERS+= ble.h
endif

OBJECTS = $(SOURCES:.c=.o)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -Wall -g -c $< -o $@

all: build

build: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET) -lc

clean:
	rm -rf $(TARGET) $(OBJECTS)
