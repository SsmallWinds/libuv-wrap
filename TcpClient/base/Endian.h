// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#pragma once

#include <stdint.h>
#ifdef __GNUC__
#include <endian.h>
#else
#include <WinSock2.h>
#endif

namespace net
{

	// the inline assembler code makes type blur,
	// so we disable warnings for a while.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
	inline uint64_t hostToNetwork64(uint64_t host64)
	{
#ifdef __GNUC__
		return htobe64(host64);
#else
		uint64_t   ret = 0;
		uint32_t   high, low;
		low = host64 & 0xFFFFFFFF;
		high = (host64 >> 32) & 0xFFFFFFFF;
		low = htonl(low);
		high = htonl(high);
		ret = low;
		ret <<= 32;
		ret |= high;
		return   ret;
#endif

	}

	inline uint32_t hostToNetwork32(uint32_t host32)
	{
#ifdef __GNUC__
		return htobe32(host32);
#else
		htonl(host32);
#endif
	}

	inline uint16_t hostToNetwork16(uint16_t host16)
	{
#ifdef __GNUC__
		return htobe16(host16);
#else
		htons(host16);
#endif
	}

	inline uint64_t networkToHost64(uint64_t net64)
	{
#ifdef __GNUC__
		return be64toh(net64);
#else
		uint64_t   ret = 0;
		uint32_t   high, low;

		low = net64 & 0xFFFFFFFF;
		high = (net64 >> 32) & 0xFFFFFFFF;
		low = ntohl(low);
		high = ntohl(high);

		ret = low;
		ret <<= 32;
		ret |= high;
		return   ret;
#endif
	}

	inline uint32_t networkToHost32(uint32_t net32)
	{
#ifdef __GNUC__
		return be32toh(net32);
#else
		ntohl(net32);
#endif
	}

	inline uint16_t networkToHost16(uint16_t net16)
	{
#ifdef __GNUC__
		return be16toh(net16);
#else
		ntohs(net16);
#endif
	}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
} // namespace net
