
#include <assert.h>
#include <fcntl.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "aw-ima.h"
#include "aw-wav.h"

static struct ima_info ima_info;
static struct ima_decode_state ima_state;

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

void write_all(int fd, const void *p, size_t n) {
	ssize_t err;

	do {
		if ((err = write(fd, p, n)) < 0) {
			perror("Write failed");
			break;
		}
	} while ((n -= err) > 0);
}

#define BUFFER_SIZE (16384)
static int16_t buffer[BUFFER_SIZE / sizeof (int16_t)];

int wav_fd;

static void queue(unsigned src, unsigned buf) {
	unsigned frame_count = BUFFER_SIZE / (ima_info.channel_count * sizeof (int16_t));
	size_t size;

	if (frame_count > ima_info.frame_count - ima_state.offset)
		frame_count = ima_info.frame_count - ima_state.offset;

	ima_decode(buffer, frame_count, ima_info.blocks, ima_info.channel_count, &ima_state);
	size = frame_count * ima_info.channel_count * sizeof (int16_t);

	if (size > 0) {
		alBufferData(
			buf, AL_FORMAT_STEREO16,
			buffer, size, ima_info.sample_rate);

		alSourceQueueBuffers(src, 1, &buf);

		if (wav_fd > 0)
			write_all(wav_fd, buffer, size);
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

	ima_init(&ima_state);

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
	uint32_t wav_buf[WAV_HEADER_SIZE / 4 + 1];
	struct wav_info wav_info;
	off_t file_end, data_off;

	char *caf = NULL;
	char *wav = NULL;

	if (argc > 1)
		caf = argv[1];

	if (argc > 2)
		wav = argv[2];

	if (caf == NULL) {
		fputs("Usage: test <caf> [wav]\n", stderr);
		return 1;
	}

	if (load(caf) < 0)
		return 1;

	if (wav != NULL) {
		if ((wav_fd = open(wav, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0)
			perror("Open failed");
		else {
			write_all(wav_fd, wav_buf, WAV_HEADER_SIZE);
			data_off = lseek(wav_fd, 0, SEEK_CUR);
			assert(data_off == WAV_HEADER_SIZE);
		}
	}

	play();

	if (wav_fd > 0) {
		file_end = lseek(wav_fd, 0, SEEK_CUR);
		lseek(wav_fd, 0, SEEK_SET);

		wav_info.blocks = NULL;
		wav_info.size = file_end - data_off;
		wav_info.sample_rate = ima_info.sample_rate;
		wav_info.frame_count = ima_info.frame_count;
		wav_info.sample_format = WAV_FORMAT_INT16;
		wav_info.channel_count = ima_info.channel_count;

		wav_write(wav_buf, &wav_info);
		write_all(wav_fd, wav_buf, WAV_HEADER_SIZE);

		close(wav_fd);
	}

	return 0;
}

