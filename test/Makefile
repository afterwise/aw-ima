
test: test.o
	clang -framework OpenAL -o $@ $^

%.o: %.c ../aw-ima.h
	clang -I.. -c $< -o $@

.PHONY: clean
clean:
	rm -f test test.o

