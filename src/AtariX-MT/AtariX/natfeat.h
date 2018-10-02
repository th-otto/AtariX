#ifndef _NF_NATFEAT_H
#define _NF_NATFEAT_H

#ifdef __cplusplus
extern "C" {
#endif

uint32 nf_get_id(uint32 args);
sint32 nf_call(uint32 args);


INLINE uint32 nf_getparameter(uint32 args, int i)
{
	if (i < 0)
		return 0;

	return m68ki_read_32(args + i * 4);
}


INLINE void Atari2Host_memcpy(void *_dst, uint32 src, size_t count)
{
	unsigned char *dst = (unsigned char *)_dst;
	while (count > 0)
	{
		*dst++ = m68ki_read_8(src);
		src++;
		count--;
	}
}

INLINE void Host2Atari_memcpy(uint32 dst, const void *_src, size_t count)
{
	const unsigned char *src = (const unsigned char *)_src;
	while (count > 0)
	{
		m68ki_write_8(dst, *src++);
		dst++;
		count--;
	}
}

#ifdef __cplusplus
}
#endif

#endif
