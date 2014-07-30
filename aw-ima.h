
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

#include "aw-imatypes.h"

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
        ((u32) (u8) (d) | ((u32) (u8)(c) << 8) | \
        ((u32) (u8) (b) << 16) | ((u32) (u8) (a) << 24))

#ifdef __cplusplus
extern "C" {
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

	index = btoh16(block->preamble) & 0x7f;
	predict = (s16) btoh16(block->preamble) & ~0x7f;

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
		ima_output_t *_ima_restrict output, unsigned frame_count,
		const void *data, unsigned channel_count,
		struct ima_decode_state *state) {
	const struct ima_block *blocks;
	unsigned i, remain_count, decode_count;

	remain_count = frame_count;

	blocks = data;
	blocks += state->offset / (IMA_BLOCK_DATA_SIZE * 2) * channel_count;

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

	state->offset += frame_count;
}

static _ima_alwaysinline void ima_init(struct ima_decode_state *state) {
	memset(state, 0, sizeof (struct ima_decode_state));
}

static int ima_parse(struct ima_info *info, const void *data) {
	const struct caf_header *header = data;
	const struct caf_chunk *chunk = (const void *) &header[1];
	const struct caf_audio_description *desc;
	const struct caf_packet_table *pakt;
	const struct ima_block *blocks;
	union { f64 f; u64 u; } conv64;
	s64 chunk_size;
	unsigned chunk_type;

	if (btoh32(header->type) != ima_fourcc('c', 'a', 'f', 'f'))
		return -1;

	if (btoh16(header->version) != 1)
		return -2;

	for (;;) {
		chunk_type = btoh32(chunk->type);
		chunk_size = btoh64(chunk->size);

		if (chunk_type == ima_fourcc('d', 'e', 's', 'c'))
			desc = (const void *) &chunk[1];
		else if (chunk_type == ima_fourcc('p', 'a', 'k', 't'))
			pakt = (const void *) &chunk[1];
		else if (chunk_type == ima_fourcc('d', 'a', 't', 'a')) {
			blocks = (const void *) &((const struct caf_data *) &chunk[1])[1];
			break;
		}

		chunk = (const void *) ((const u8 *) &chunk[1] + chunk_size);
	}

	if (btoh32(desc->format_id) != ima_fourcc('i', 'm', 'a', '4'))
		return -3;

	info->blocks = blocks;
	info->size = chunk_size;
	info->frame_count = btoh64(pakt->frame_count);
	info->channel_count = btoh32(desc->channels_per_frame);

	conv64.u = btoh64(*(const u64 *) &desc->sample_rate);
	info->sample_rate = conv64.f;

	return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_IMA_H */

