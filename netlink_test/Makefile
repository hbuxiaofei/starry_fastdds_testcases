#
# ip link add vnet0 type veth
# ip link delete vnet0
#

TARGET := netlink-test

DIRS = .

# CFLAGES = -pthread
CFLAGES =

FILES = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))

OBJS = $(patsubst %.c,%.o, $(FILES))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS) $(CFLAGES)

$(OBJS):%.o: %.c
	$(CC) -c $< -o $@ $(LIBS) $(CFLAGES)

clean:
	$(RM) $(OBJS) $(TARGET)

