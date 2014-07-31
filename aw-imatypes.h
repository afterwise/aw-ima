
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

#ifndef AW_IMATYPES_H
#define AW_IMATYPES_H

#include "aw-endian.h"

#if __GNUC__
# define _ima_packed __attribute__((packed))
#elif _MSC_VER
# define _ima_packed
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IMA_FLOAT_OUTPUT
# define IMA_OUTPUT_SAMPLE(x) ((f32) (x) * 0.0000305185f) /* 1.0 / 32767.0 */
typedef f32 ima_output_t;
#else
# define IMA_OUTPUT_SAMPLE(x) (x)
typedef s16 ima_output_t;
#endif

struct ima_info {
	const void *blocks;
	u64 size;
	f64 sample_rate;
	u64 frame_count;
	u32 channel_count;
};

struct ima_channel_state {
	s32 index;
	s32 predict;
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
	u32 type;
	u16 version;
	u16 flags;
};

struct _ima_packed caf_chunk {
	u32 type;
	s64 size;
};

struct _ima_packed caf_audio_description {
	f64 sample_rate;
	u32 format_id;
	u32 format_flags;
	u32 bytes_per_packet;
	u32 frames_per_packet;
	u32 channels_per_frame;
	u32 bits_per_channel;
};

struct _ima_packed caf_packet_table {
	s64 packet_count;
	s64 frame_count;
	s32 priming_frames;
	s32 remainder_frames;
};

struct _ima_packed caf_data {
	u32 edit_count;
};

#define IMA_BLOCK_DATA_SIZE (32)

struct _ima_packed ima_block {
	u16 preamble;
	u8 data[IMA_BLOCK_DATA_SIZE];
};

#if _MSC_VER
# pragma pack(pop)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_IMATYPES_H */

