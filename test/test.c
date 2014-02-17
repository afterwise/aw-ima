
#include <fcntl.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "aw-ima.h"

static struct ima_info ima_info;
static uint64_t ima_offs;

static int load(const char *path) {
	struct stat stat;
	void *ptr;
	int fd;

	if ((fd = open(path, O_RDONLY)) < 0)
		return perror("Open failed"), -1;

	if (fstat(fd, &stat) < 0)
		return close(fd), perror("Stat failed"), -1;

	if ((ptr = mmap(NULL, stat.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0)) == NULL)
		return close(fd), perror("Mmap failed"), -1;

	close(fd);

	if (ima_parse(&ima_info, ptr) < 0)
		return close(fd), fputs("Parse failed\n", stderr), -1;

	return 0;
}

#define BUFFER_SIZE (16384)
static int16_t buffer[BUFFER_SIZE / sizeof (int16_t)];

static void queue(unsigned src, unsigned buf) {
	unsigned frame_count = BUFFER_SIZE / (ima_info.channel_count * sizeof (int16_t));
	size_t size;

	if (frame_count > ima_info.frame_count - ima_offs)
		frame_count = ima_info.frame_count - ima_offs;

	ima_decode(buffer, ima_offs, frame_count, ima_info.blocks, ima_info.channel_count);
	ima_offs += frame_count;
	size = frame_count * ima_info.channel_count * sizeof (int16_t);

	if (size > 0) {
		alBufferData(
			buf, AL_FORMAT_STEREO16,
			buffer, size, ima_info.sample_rate);

		alSourceQueueBuffers(src, 1, &buf);
	}
}

static void play() {
	ALCdevice *dev;
	ALCcontext *ctx;
	unsigned src;
	unsigned bufs[2];
	unsigned buf;
	int v;
	float zero[3];

	memset(zero, 0, sizeof zero);

	dev = alcOpenDevice(NULL);
	ctx = alcCreateContext(dev, NULL);
	alcMakeContextCurrent(ctx);

	alGenSources(1, &src);
	alGenBuffers(2, bufs);

	alSourcef(src, AL_PITCH, 1.0f);
	alSourcef(src, AL_GAIN, 1.0f);
	alSourcefv(src, AL_POSITION, zero);
	alSourcefv(src, AL_VELOCITY, zero);
	alSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcei(src, AL_LOOPING, AL_FALSE);

	queue(src, bufs[0]);
	queue(src, bufs[1]);

	alSourcePlay(src);

	for (;;) {
		alGetSourcei(src, AL_BUFFERS_PROCESSED, &v);

		while (v-- > 0) {
			alSourceUnqueueBuffers(src, 1, &buf);
			queue(src, buf);
		}

		alGetSourcei(src, AL_SOURCE_STATE, &v);

		if (v == AL_STOPPED)
			break;

		usleep(1000);
	}

        alDeleteSources(1, &src);
        alDeleteBuffers(2, bufs);
        alcMakeContextCurrent(NULL);
        alcDestroyContext(ctx);
        alcCloseDevice(dev);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fputs("Usage: test <caf>\n", stderr);
		return 1;
	}

	if (load(argv[1]) < 0)
		return 1;

	play();
	return 0;
}

