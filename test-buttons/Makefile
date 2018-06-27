#The following four macros should be defined.
TARGETS=test_sigio test_select
OBJECTS=test_sigio.o test_select.o
CC=gcc
LD_FLAGS=
C_FLAGS=-Wall
# end of user configuration
all: $(TARGETS)
# add rule for	TARGETS
% : %.o
	$(CC) $(LD_FLAGS) $< -o $@

%.o : %.c
	$(CC) $(C_FLAGS) -c $<

.PHONY: clean
clean :
	-rm -f $(OBJECTS) $(TARGETS)
