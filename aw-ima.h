
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

#include <string.h>

#if __GNUC__
# define _ima_alwaysinline inline __attribute__((always_inline))
# define _ima_unused __attribute__((__unused__))
#elif _MSC_VER
# define _ima_alwaysinline __forceinline
# define _ima_unused
#endif

#if __GNUC__
# define _ima_packed __attribute__((packed))
#elif _MSC_VER
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

#define ima_fourcc(a,b,c,d) \
        ((ima_u32_t) (ima_u8_t) (d) | ((ima_u32_t) (ima_u8_t)(c) << 8) | \
        ((ima_u32_t) (ima_u8_t) (b) << 16) | ((ima_u32_t) (ima_u8_t) (a) << 24))

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char ima_s8_t;
typedef unsigned char ima_u8_t;
typedef signed short ima_s16_t;
typedef unsigned short ima_u16_t;
typedef signed int ima_s32_t;
typedef unsigned int ima_u32_t;
#if _MSC_VER
typedef signed __int64 ima_s64_t;
typedef unsigned __int64 ima_u64_t;
#else
typedef signed long long ima_s64_t;
typedef unsigned long long ima_u64_t;
#endif
typedef float ima_f32_t;
typedef double ima_f64_t;

#ifdef IMA_FLOAT_OUTPUT
# define IMA_OUTPUT_SAMPLE(x) ((ima_f32_t) (x) * 0.0000305185f) /* 1.0 / 32767.0 */
typedef ima_f32_t ima_output_t;
#else
# define IMA_OUTPUT_SAMPLE(x) (x)
typedef ima_s16_t ima_output_t;
#endif

struct ima_info {
	const void *blocks;
	ima_u64_t size;
	ima_f64_t sample_rate;
	ima_u64_t frame_count;
	ima_u32_t channel_count;
};

struct ima_channel_state {
	ima_s32_t index;
	ima_s32_t predict;
};

struct ima_decode_state {
	struct ima_channel_state channels[8];
};

#define ima_init(decode_state) do { \
		memset((decode_state), 0, sizeof (struct ima_decode_state)); \
	} while (0)

#if _MSC_VER
# pragma pack(push, 1)
#endif

struct _ima_packed caf_header {
	ima_u32_t type;
	ima_u16_t version;
	ima_u16_t flags;
};

struct _ima_packed caf_chunk {
	ima_u32_t type;
	ima_s64_t size;
};

struct _ima_packed caf_audio_description {
	ima_f64_t sample_rate;
	ima_u32_t format_id;
	ima_u32_t format_flags;
	ima_u32_t bytes_per_packet;
	ima_u32_t frames_per_packet;
	ima_u32_t channels_per_frame;
	ima_u32_t bits_per_channel;
};

struct _ima_packed caf_packet_table {
	ima_s64_t packet_count;
	ima_s64_t frame_count;
	ima_s32_t priming_frames;
	ima_s32_t remainder_frames;
};

struct _ima_packed caf_data {
	ima_u32_t edit_count;
};

#define IMA_BLOCK_DATA_SIZE (32)

struct _ima_packed ima_block {
	ima_u16_t preamble;
	ima_u8_t data[IMA_BLOCK_DATA_SIZE];
};

#if _MSC_VER
# pragma pack(pop)
#endif

static _ima_alwaysinline ima_u16_t ima_bswap16(ima_u16_t v) {
        return
                (v << 0x08 & 0xff00u) |
                (v >> 0x08 & 0x00ffu);
}

static _ima_alwaysinline ima_u32_t ima_bswap32(ima_u32_t v) {
        return
                (v << 0x18 & 0xff000000ul) |
                (v << 0x08 & 0x00ff0000ul) |
                (v >> 0x08 & 0x0000ff00ul) |
                (v >> 0x18 & 0x000000fful);
}

static _ima_alwaysinline ima_u64_t ima_bswap64(ima_u64_t v) {
        return
                (v << 0x38 & 0xff00000000000000ull) |
                (v << 0x28 & 0x00ff000000000000ull) |
                (v << 0x18 & 0x0000ff0000000000ull) |
                (v << 0x08 & 0x000000ff00000000ull) |
                (v >> 0x08 & 0x00000000ff000000ull) |
                (v >> 0x18 & 0x0000000000ff0000ull) |
                (v >> 0x28 & 0x000000000000ff00ull) |
                (v >> 0x38 & 0x00000000000000ffull);
}

#if __BIG_ENDIAN__ || __ARMEB__
static _ima_alwaysinline ima_u16_t ima_btoh16(ima_u16_t v) { return v; }
static _ima_alwaysinline ima_u32_t ima_btoh32(ima_u32_t v) { return v; }
static _ima_alwaysinline ima_u64_t ima_btoh64(ima_u64_t v) { return v; }

static _ima_alwaysinline ima_u16_t ima_ltoh16(ima_u16_t v) { return ima_bswap16(v); }
static _ima_alwaysinline ima_u32_t ima_ltoh32(ima_u32_t v) { return ima_bswap32(v); }
static _ima_alwaysinline ima_u64_t ima_ltoh64(ima_u64_t v) { return ima_bswap64(v); }

static _ima_alwaysinline ima_u16_t ima_htob16(ima_u16_t v) { return v; }
static _ima_alwaysinline ima_u32_t ima_htob32(ima_u32_t v) { return v; }
static _ima_alwaysinline ima_u64_t ima_htob64(ima_u64_t v) { return v; }

static _ima_alwaysinline ima_u16_t ima_htol16(ima_u16_t v) { return ima_bswap16(v); }
static _ima_alwaysinline ima_u32_t ima_htol32(ima_u32_t v) { return ima_bswap32(v); }
static _ima_alwaysinline ima_u64_t ima_htol64(ima_u64_t v) { return ima_bswap64(v); }
#else
static _ima_alwaysinline ima_u16_t ima_btoh16(ima_u16_t v) { return ima_bswap16(v); }
static _ima_alwaysinline ima_u32_t ima_btoh32(ima_u32_t v) { return ima_bswap32(v); }
static _ima_alwaysinline ima_u64_t ima_btoh64(ima_u64_t v) { return ima_bswap64(v); }

static _ima_alwaysinline ima_u16_t ima_ltoh16(ima_u16_t v) { return v; }
static _ima_alwaysinline ima_u32_t ima_ltoh32(ima_u32_t v) { return v; }
static _ima_alwaysinline ima_u64_t ima_ltoh64(ima_u64_t v) { return v; }

static _ima_alwaysinline ima_u16_t ima_htob16(ima_u16_t v) { return ima_bswap16(v); }
static _ima_alwaysinline ima_u32_t ima_htob32(ima_u32_t v) { return ima_bswap32(v); }
static _ima_alwaysinline ima_u64_t ima_htob64(ima_u64_t v) { return ima_bswap64(v); }

static _ima_alwaysinline ima_u16_t ima_htol16(ima_u16_t v) { return v; }
static _ima_alwaysinline ima_u32_t ima_htol32(ima_u32_t v) { return v; }
static _ima_alwaysinline ima_u64_t ima_htol64(ima_u64_t v) { return v; }
#endif

static const int ima_index_table[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const int ima_step_table[89] = {
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
		diff = step >> 3; \
		if (nibble & 4) diff += step; \
		if (nibble & 2) diff += step >> 1; \
		if (nibble & 1) diff += step >> 2; \
		if (nibble & 8) predict -= diff; else predict += diff; \
		step = ima_step_table[index]; \
		predict = ima_clamp_predict(predict); \
		*output = IMA_OUTPUT_SAMPLE(predict); \
		output += channel_count; \
	} while (0)

static _ima_alwaysinline void ima_decode_block(
		ima_output_t *_ima_restrict output, unsigned channel_count,
		const struct ima_block *block, unsigned decode_count,
		struct ima_channel_state *state) {
	int index, predict, step, diff, nibble;
	unsigned i;

	index = ima_btoh16(block->preamble) & 0x7f;
	predict = (ima_s16_t) ima_btoh16(block->preamble) & ~0x7f;

	if (index == state->index) {
		if ((diff = predict - state->predict) < 0)
			diff = -diff;

		if (diff <= 0x7f)
			predict = state->predict;
	}

	step = ima_step_table[index];

	for (i = 0; i < (decode_count >> 1); ++i) {
		ima_decode_sample(block->data[i] & 0xf);
		ima_decode_sample(block->data[i] >> 4);
	}

	if (_ima_unlikely((decode_count & 1) != 0))
		ima_decode_sample(block->data[decode_count >> 1] & 0xf);

	state->index = index;
	state->predict = predict;
}

static void ima_decode(
		ima_output_t *_ima_restrict output, ima_u64_t frame_offset, unsigned frame_count,
		const void *data, unsigned channel_count,
		struct ima_decode_state *state) _ima_unused;
static void ima_decode(
		ima_output_t *_ima_restrict output, ima_u64_t frame_offset, unsigned frame_count,
		const void *data, unsigned channel_count,
		struct ima_decode_state *state) {
	const struct ima_block *blocks;
	unsigned i, remain_count, decode_count;

	remain_count = frame_count;

	blocks = (const struct ima_block *) data;
	blocks += frame_offset / (IMA_BLOCK_DATA_SIZE * 2) * channel_count;

	while (remain_count > 0) {
		if (_ima_unlikely(remain_count < IMA_BLOCK_DATA_SIZE * 2))
			decode_count = remain_count;
		else
			decode_count = IMA_BLOCK_DATA_SIZE * 2;

		for (i = 0; i < channel_count; ++i)
			ima_decode_block(
				output + i, channel_count, blocks++, decode_count,
				&state->channels[i]);

		remain_count -= decode_count;
		output += decode_count * channel_count;
	}
}

static int ima_parse(struct ima_info *info, const void *data) _ima_unused;
static int ima_parse(struct ima_info *info, const void *data) {
	const struct caf_header *header = (const struct caf_header *) data;
	const struct caf_chunk *chunk = (const struct caf_chunk *) &header[1];
	const struct caf_audio_description *desc;
	const struct caf_packet_table *pakt;
	const struct ima_block *blocks;
	union { ima_f64_t f; ima_u64_t u; } conv64;
	ima_s64_t chunk_size;
	unsigned chunk_type;

	if (ima_btoh32(header->type) != ima_fourcc('c', 'a', 'f', 'f'))
		return -1;

	if (ima_btoh16(header->version) != 1)
		return -2;

	for (;;) {
		chunk_type = ima_btoh32(chunk->type);
		chunk_size = ima_btoh64(chunk->size);

		if (chunk_type == ima_fourcc('d', 'e', 's', 'c'))
			desc = (const struct caf_audio_description *) &chunk[1];
		else if (chunk_type == ima_fourcc('p', 'a', 'k', 't'))
			pakt = (const struct caf_packet_table *) &chunk[1];
		else if (chunk_type == ima_fourcc('d', 'a', 't', 'a')) {
			blocks = (const struct ima_block *) &((const struct caf_data *) &chunk[1])[1];
			break;
		}

		chunk = (const struct caf_chunk *) ((const ima_u8_t *) &chunk[1] + chunk_size);
	}

	if (ima_btoh32(desc->format_id) != ima_fourcc('i', 'm', 'a', '4'))
		return -3;

	info->blocks = blocks;
	info->size = chunk_size;
	info->frame_count = ima_btoh64(pakt->frame_count);
	info->channel_count = ima_btoh32(desc->channels_per_frame);

	conv64.u = ima_btoh64(*(const ima_u64_t *) &desc->sample_rate);
	info->sample_rate = conv64.f;

	return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_IMA_H */

