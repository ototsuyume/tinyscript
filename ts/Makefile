
	
SOURCE = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SOURCE))
		
CXX = gcc

		
all:ts

ts:$(OBJS)
	$(CXX) -o $@ $^

.PHONY:clean
clean:
	rm -f *.o *.d ts

include $(SOURCE:.c)	
		
%.o:%.c
	$(CXX) -Wall $(CFLAGS) -c $< -o $@

%.d: %.c
	@set -e; rm -f $@; \
	$(CXX) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$



