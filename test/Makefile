
test: test.o
	clang -framework OpenAL -o $@ $^

%.o: %.c ../aw-ima.h aw-wav/aw-wav.h
	clang -I.. -Iaw-wav -c $< -o $@

aw-wav/aw-wav.h:
	git clone git@github.com:afterwise/aw-wav.git && \
	touch -c $@

.PHONY: clean
clean:
	rm -f test test.o
	rm -rf aw-wav

