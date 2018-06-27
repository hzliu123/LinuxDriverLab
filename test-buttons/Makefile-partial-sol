#The following four macros should be defined.
TARGET1=test_sigio
OBJECT1=test_sigio.o
TARGET2=   # add something
OBJECT2=   # add something
CC=gcc
LD_FLAGS=
C_FLAGS=-Wall
# end of user configuration
all: $(TARGET1) $(TARGET2)
$(TARGET1) : $(OBJECT1)
	$(CC) $(LD_FLAGS) $< -o $@
# add rule for	TARGET2


%.o : %.c
	$(CC) $(C_FLAGS) -c $<

.PHONY: clean
clean :
	-rm -f $(OBJECT1) $(TARGET1)   # add TARGET2 OBJECT2
