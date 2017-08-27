
TARGET = server 

INC_DIR+=../MessageBus/include 
SOURCE_DIR=./
LIB_DIR+=./
CFLAGS += -g -O2 -I$(INC_DIR) 

SOURCE_TYPE=.c
HEADER_TYPE=.h

SOURCES=$(foreach d,$(SOURCE_DIR),$(wildcard $(addprefix $(d)/*,$(SOURCE_TYPE))))
OBJS=$(addsuffix .o, $(basename $(SOURCES)))

LDFLAGS+= -L$(LIB_DIR) 
LDFLAGS+= -lssl -lcrypto 
.PHONY:all clean 

all:$(TARGET)

%.o:%.c
	@echo [Compiling...] $<
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(TARGET):$(OBJS)
	@echo [Linking...] $<
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS)  -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
