TARGETS = test_simple_event

OBJS = simple_event.o

ifdef TEST_SIO
CFLAGS += -DTEST_SIO
OBJS += simple_sio/simple_sio.o
endif

ifdef TEST_NIO
CFLAGS += -DTEST_NIO
OBJS += simple_netio/simple_netio.o
endif

include Makefile.common

