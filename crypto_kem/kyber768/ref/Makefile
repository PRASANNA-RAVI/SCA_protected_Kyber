CC     = arm-none-eabi-gcc
CFLAGS = -Wall -Wextra -O3 -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g -MD $(DEFINES)
AR     = arm-none-eabi-gcc-ar
OPENCM3DIR  = ./../../../libopencm3
DEFINES     = -DSTM32F4

CC_HOST = gcc
CFLAGS_HOST = -Wall -Wextra -O3 -MD $(DEFINES)
AR_HOST = ar

HEADERS = api.h cbd.h indcpa.h ntt.h params.h poly.h polyvec.h reduce.h verify.h
SOURCES = cbd.c indcpa.c kem.c ntt.c poly.c polyvec.c precomp.c reduce.c verify.c
OBJECTS = cbd.o indcpa.o kem.o ntt.o poly.o polyvec.o precomp.o reduce.o verify.o

OBJECTS_HOST = $(patsubst %.o,%_host.o,$(OBJECTS))

libpqm4.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

libpqhost.a: $(OBJECTS_HOST)
	$(AR_HOST) rcs $@ $(OBJECTS_HOST)

%_host.o: %.c $(HEADERS)
	$(CC_HOST) -I$(INCPATH) -I$(OPENCM3DIR)/include $(CFLAGS_HOST) -c -o $@ $<

%.o: %.c $(HEADERS)
	$(CC) -I$(INCPATH) -I$(OPENCM3DIR)/include $(CFLAGS) -c -o $@ $<

.PHONY: clean
.INTERMEDIATE: $(OBJECTS) $(OBJECTS_HOST)

clean:
	-rm -f libpqhost.a
	-rm -f libpqm4.a
	-rm -f *.o
