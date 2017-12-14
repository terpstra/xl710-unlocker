TARGETS = xl710_unlock

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%:	%.c
	gcc -Wall -O2 $< -o $@
