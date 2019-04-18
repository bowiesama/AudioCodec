/*
 *  Copyright (c) 2014 Broadsoft. All Rights Reserved.
 *
 *  Authors: 
 *           Danail Kirov
 */

#include <stdint.h>
#include <assert.h>

#ifdef _MSC_VER
#define inline _inline
#endif

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

//
// Bit Stream Writer
//
typedef struct BswContext {
	uint32_t	bit_buf;
	int			bit_left;
	uint8_t		*buf, *buf_ptr, *buf_end;
	int			size_in_bits;
} BswContext;

static inline void bsw_init(BswContext *s, uint8_t *buffer, int buffer_size)
{
	if (buffer_size < 0) 
	{
		buffer_size = 0;
		buffer = NULL;
	}

	s->size_in_bits = 8 * buffer_size;
	s->buf = buffer;
	s->buf_end = s->buf + buffer_size;
	s->buf_ptr = s->buf;
	s->bit_left = 32;
	s->bit_buf = 0;
}

static inline void bsw_flush(BswContext *s)
{
	if (s->bit_left < 32)
		s->bit_buf <<= s->bit_left;

	while (s->bit_left < 32) 
	{
		*s->buf_ptr++ = s->bit_buf >> 24;
		s->bit_buf <<= 8;
		s->bit_left += 8;
	}

	s->bit_left = 32;
	s->bit_buf = 0;
}

static inline void bsw_put_bits(BswContext *s, int n, unsigned int value)
{
	unsigned int bit_buf;
	int bit_left;

	assert(n <= 31 && value < (1U << n));

	bit_buf = s->bit_buf;
	bit_left = s->bit_left;

	if (n < bit_left) 
	{
		bit_buf = (bit_buf << n) | value;
		bit_left -= n;
	}
	else 
	{
		bit_buf <<= bit_left;
		bit_buf |= value >> (n - bit_left);
		assert(s->buf_ptr + 3 < s->buf_end );

		s->buf_ptr[3] = bit_buf;
		s->buf_ptr[2] = bit_buf >> 8;
		s->buf_ptr[1] = bit_buf >> 16;
		s->buf_ptr[0] = bit_buf >> 24;

		s->buf_ptr += 4;
		bit_left += 32 - n;
		bit_buf = value;
	}

	s->bit_buf = bit_buf;
	s->bit_left = bit_left;
}


//
// Bit Stream Reader
//

typedef struct BsrContext {
	uint8_t  *buffer;
	uint8_t  *buf_ptr;
	uint8_t  *buf_end;
	uint32_t bit_buf;
	int      bit_left;

} BsrContext;

//
// slightly faster version than the bsr_get_bits_safe()
// but it may read a couple of octets from the void and that 
// could be a problem on some platforms.
//
static inline uint32_t bsr_get_bits(BsrContext *s, int n)
{
	uint32_t value;
	uint32_t bit_buf;
	int bit_left;
	int i;

	bit_buf = s->bit_buf;
	bit_left = s->bit_left;

	if (n < bit_left) 
	{
		bit_left -= n;
		value = bit_buf >> bit_left;
		bit_buf <<= 32 - bit_left;
		bit_buf >>= 32 - bit_left;
	}
	else 
	{
		value = bit_buf;
		n -= bit_left;
		
		value <<= n;

		bit_buf = 0;
		bit_left = 0;

		for (i = 0; i < 4 ; i++, s->buf_ptr++)
		{
			bit_buf <<= 8;
			bit_buf |= *s->buf_ptr;
			bit_left += 8;
		}

		bit_left -= n;
		value |= bit_buf >> bit_left;
		
		bit_buf <<= 32 - bit_left;
		bit_buf >>= 32 - bit_left;
	}

	s->bit_buf = bit_buf;
	s->bit_left = bit_left;

	return value;
}


static inline uint32_t bsr_get_bits_safe(BsrContext *s, int n)
{
	uint32_t value;
	uint32_t bit_buf;
	int bit_left;
	int i;

	bit_buf = s->bit_buf;
	bit_left = s->bit_left;

	if (n < bit_left) 
	{
		bit_left -= n;
		value = bit_buf >> bit_left;
		bit_buf <<= 32 - bit_left;
		bit_buf >>= 32 - bit_left;
	}
	else 
	{
		int octets_left = min(4, s->buf_end - s->buf_ptr); 

		value = bit_buf;
		n -= bit_left;
		
		value <<= n;

		bit_buf = 0;
		bit_left = 0;

		for (i = 0; i < octets_left ; i++, s->buf_ptr++)
		{
			bit_buf <<= 8;
			bit_buf |= *s->buf_ptr;
			bit_left += 8;
		}

		bit_left -= n;
		value |= bit_buf >> bit_left;
		
		bit_buf <<= 32 - bit_left;
		bit_buf >>= 32 - bit_left;
	}

	s->bit_buf = bit_buf;
	s->bit_left = bit_left;

	return value;
}

static inline int bsr_init(	BsrContext *s,
							uint8_t *buffer,
							int bit_size)
{
    int buffer_size;
    int ret = 0;

    if (bit_size < 0 || !buffer) {
        bit_size    = 0;
        buffer      = NULL;
        ret         = -1;
    }

    buffer_size = (bit_size + 7) >> 3;

    s->buffer    = buffer;
	s->buf_ptr   = s->buffer;
	s->buf_end   = s->buffer + buffer_size;

	s->bit_buf   = 0;
	s->bit_left  = 0;

    return ret;
}
