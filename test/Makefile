
export CFLAGS += -Wall -Wextra -Itest/aw-bytes

test: test.o aw-fs/libaw-fs.a
	$(CC) $(LDFLAGS) -framework OpenAL -o $@ $^

%.o: %.c ../aw-ima.h aw-bytes/aw-types.h aw-fs/aw-fs.h aw-wav/aw-wav.h
	$(CC) $(CFLAGS) -I.. -Iaw-bytes -Iaw-fs -Iaw-wav -c $< -o $@

aw-fs/libaw-fs.a:
	$(MAKE) -Caw-fs

aw-bytes/aw-types.h:
	git clone git@github.com:afterwise/aw-bytes.git && \
	touch -c $@

aw-fs/aw-fs.h:
	git clone git@github.com:afterwise/aw-fs.git && \
	touch -c $@

aw-wav/aw-wav.h:
	git clone git@github.com:afterwise/aw-wav.git && \
	touch -c $@

.PHONY: clean
clean:
	rm -f test test.o
	rm -rf aw-bytes aw-fs aw-wav

