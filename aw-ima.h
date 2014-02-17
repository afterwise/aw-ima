
/*
   Copyright (c) 2014 Malte Hildingsson, malte (at) afterwi.se

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#ifndef AW_IMA_H
#define AW_IMA_H

#include <stdint.h>
#include <string.h>
#if __linux__
# include <endian.h>
#elif __APPLE__
# include <libkern/OSByteOrder.h>
# define htobe16 OSSwapHostToBigInt16
# define htole16 OSSwapHostToLittleInt16
# define be16toh OSSwapBigToHostInt16
# define le16toh OSSwapLittleToHostInt16
# define htobe32 OSSwapHostToBigInt32
# define htole32 OSSwapHostToLittleInt32
# define be32toh OSSwapBigToHostInt32
# define le32toh OSSwapLittleToHostInt32
# define htobe64 OSSwapHostToBigInt64
# define htole64 OSSwapHostToLittleInt64
# define be64toh OSSwapBigToHostInt64
# define le64toh OSSwapLittleToHostInt64
#endif

#if __GNUC__
# define _ima_alwaysinline inline __attribute__((always_inline, nodebug))
# define _ima_packed __attribute__((packed))
#elif _MSC_VER
# define _ima_alwaysinline __forceinline
# define _ima_packed
#endif

#if _MSC_VER
# define _ima_restrict __restrict
#else
# define _ima_restrict __restrict__
#endif

#if __GNUC__
# define _ima_likely(x) __builtin_expect(!!(x), 1)
# define _ima_unlikely(x) __builtin_expect(!!(x), 0)
#else
# define _ima_likely(x) (x)
# define _ima_unlikely(x) (x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ima_info {
	const void *data;
	size_t size;
	double sample_rate;
	uint64_t frame_count;
	unsigned channel_count;
};

#if _MSC_VER
# pragma pack(push, 1)
#endif

struct _ima_packed caf_header {
	uint32_t type;
	uint16_t version;
	uint16_t flags;
};

struct _ima_packed caf_chunk {
	uint32_t type;
	int64_t size;
};

struct _ima_packed caf_audio_description {
	double sample_rate;
	uint32_t format_id;
	uint32_t format_flags;
	uint32_t bytes_per_packet;
	uint32_t frames_per_packet;
	uint32_t channels_per_frame;
	uint32_t bits_per_channel;
};

struct _ima_packed caf_packet_table {
	int64_t packet_count;
	int64_t frame_count;
	int32_t priming_frames;
	int32_t remainder_frames;
};

struct _ima_packed caf_data {
	uint32_t edit_count;
};

#define IMA_BLOCK_DATA_SIZE (32)

struct _ima_packed ima_block {
	uint16_t preamble;
	uint8_t data[IMA_BLOCK_DATA_SIZE];
};

#if _MSC_VER
# pragma pack(pop)
#endif

static int ima_index_table[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static int ima_step_table[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static _ima_alwaysinline int ima_clamp_index(int index) {
	if (_ima_unlikely(index < 0))
		index = 0;
	else if (_ima_unlikely(index > 88))
		index = 88;
	return index;
}

static _ima_alwaysinline int ima_clamp_predict(int predict) {
	if (_ima_unlikely(predict < -32768))
		predict = -32768;
	else if (_ima_unlikely(predict > 32767))
		predict = 32767;
	return predict;
}

#define ima_decode_sample(init_nibble) do { \
		nibble = (init_nibble); \
		index = ima_clamp_index(index + ima_index_table[nibble]); \
		sign = nibble & 8; \
		nibble = nibble & 7; \
		diff = step >> 3; \
		if (nibble & 4) diff += step; \
		if (nibble & 2) diff += step >> 1; \
		if (nibble & 1) diff += step >> 2; \
		if (sign) predict -= diff; else predict += diff; \
		step = ima_step_table[index]; \
		predict = ima_clamp_predict(predict); \
		*output = predict; \
		output += channel_count; \
	} while (0)

static _ima_alwaysinline void ima_decode_block(
		int16_t *_ima_restrict output, unsigned channel_count,
		const struct ima_block *block, unsigned decode_count) {
	int index, predict, step, diff, nibble, sign;
	unsigned i;

	index = ima_clamp_index(be16toh(block->preamble) & 0x7f);
	predict = ima_clamp_predict((int16_t) be16toh(block->preamble) & ~0x7f);
	step = ima_step_table[index];

	for (i = 0; i < (decode_count >> 1); ++i) {
		ima_decode_sample(block->data[i] & 0xf);
		ima_decode_sample(block->data[i] >> 4);
	}

	if (_ima_unlikely((decode_count & 1) != 0))
		ima_decode_sample(block->data[decode_count >> 1] & 0xf);
}

static size_t ima_decode(
		int16_t **output, uint64_t *frame_offset, unsigned frame_count,
		const void *data, unsigned channel_count) {
	const struct ima_block *blocks;
	int16_t *ptr = *output;
	uint64_t offset = *frame_offset;
	unsigned i, remain_count, decode_count;

	remain_count = frame_count;

	blocks = data;
	blocks += offset / (IMA_BLOCK_DATA_SIZE * 2) * channel_count;

	while (remain_count > 0) {
		decode_count = remain_count < IMA_BLOCK_DATA_SIZE * 2 ? remain_count : IMA_BLOCK_DATA_SIZE * 2;

		for (i = 0; i < channel_count; ++i)
			ima_decode_block(ptr + i, channel_count, blocks++, decode_count);

		remain_count -= decode_count;
		ptr += decode_count * channel_count;
	}

	*frame_offset = offset + frame_count;
	return frame_count * (channel_count * sizeof (int16_t));
}

static int ima_parse(struct ima_info *info, const void *data) {
	const struct caf_header *header = data;
	const struct caf_chunk *chunk = (const void *) &header[1];
	const struct caf_audio_description *desc;
	const struct caf_packet_table *pakt;
	const struct ima_block *blocks;
	union { double f; uint64_t u; } conv64;
	int64_t chunk_size;
	unsigned chunk_type;

	if (be32toh(header->type) != 'caff')
		return -1;

	if (be16toh(header->version) != 1)
		return -2;

	for (;;) {
		chunk_type = be32toh(chunk->type);
		chunk_size = be64toh(chunk->size);

		if (chunk_type == 'desc')
			desc = (const void *) &chunk[1];
		else if (chunk_type == 'pakt')
			pakt = (const void *) &chunk[1];
		else if (chunk_type == 'data') {
			blocks = (const void *) &((const struct caf_data *) &chunk[1])[1];
			break;
		}

		chunk = (const void *) ((const uint8_t *) &chunk[1] + chunk_size);
	}

	if (be32toh(desc->format_id) != 'ima4')
		return -3;

	info->data = blocks;
	info->size = chunk_size;
	info->frame_count = be64toh(pakt->frame_count);
	info->channel_count = be32toh(desc->channels_per_frame);

	conv64.u = be64toh(*(const uint64_t *) &desc->sample_rate);
	info->sample_rate = conv64.f;

	return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_IMA_H */

