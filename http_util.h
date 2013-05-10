static inline uint8_t to_u8(size_t len)
{
	assert(len <= UINT8_MAX && "HTTP parser didn't check len <= U8MAX");
	return (uint8_t)len;
}

static inline uint16_t to_u16(size_t len)
{
	assert(len <= UINT16_MAX && "HTTP parser didn't check len <= U16MAX");
	return (uint16_t)len;
}
