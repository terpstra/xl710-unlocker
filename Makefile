TARGETS = xl710_patch

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%:	%.c
	gcc -Wall -O2 $< -o $@
