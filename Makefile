#TEST_SIO=1
#TEST_NIO=1

TARGETS = test_simple_event

CFLAGS += -g -Wall -Werror

ifdef TEST_SIO
CFLAGS += -DTEST_SIO
OBJS += simple_sio/simple_sio.o
endif

ifdef TEST_NIO
CFLAGS += -DTEST_NIO
OBJS += simple_netio/simple_netio.o
endif

all: $(TARGETS)

test_simple_event: simple_event.o $(OBJS)

.PHONY: clean

clean:
	rm -rf *.o *.dSYM
	rm -f $(TARGETS) $(OBJS)

