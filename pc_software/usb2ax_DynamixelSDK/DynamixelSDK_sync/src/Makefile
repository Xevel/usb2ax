TARGET		= libdxl.a
OBJS		= dxl_hal.o dynamixel.o
SRCS		= $(OBJS:.o=.c)
INCLUDEDIRS	+= -I../include
LIBDIRS		+= 
CFLAGS		= $(INCLUDEDIRS) -W -Wall -O2

CC			= gcc
AR			= ar

$(TARGET): $(OBJS)
	$(AR) rs $@ $^
	cp $(TARGET) ../lib
	
.c.o:
	$(CC) -c $< $(CFLAGS)
	
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "file deleted."

dep:
	gccmakedep $(SRCS)

