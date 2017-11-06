/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

/**
 * @file
 *
 * Ring buffer implementation for network stack drivers
 */

#include <memory.h>
#include <algorithm>

#include "RingBuffer.h"

bufElementType* RingBuffer::get_buf()
{
	return buf + begin;
}

size_t RingBuffer::produce(size_t n)
{
	n = std::min(n, getFree());
	if (n == 0) {
		return n;
	}
	const size_t first_chunk = std::min(n, size - end);
	end = (end + first_chunk) % size;
	if (first_chunk < n) {
		const size_t second_chunk = n - first_chunk;
		end = (end + second_chunk) % size;
	}
	if (begin == end) {
		wrap = true;
	}
	return n;
}

void RingBuffer::reset()
{
	consume(count());
}

size_t RingBuffer::consume(size_t n)
{
	n = std::min(n, count());
	if (n == 0) {
		return n;
	}
	if (wrap) {
		wrap = false;
	}
	const size_t first_chunk = std::min(n, size - begin);
	begin = (begin + first_chunk) % size;
	if (first_chunk < n) {
		const size_t second_chunk = n - first_chunk;
		begin = (begin + second_chunk) % size;
	}
	return n;
}

size_t RingBuffer::write(const bufElementType * data, size_t n)
{
	n = std::min(n, getFree());

	if (n == 0) {
		return n;
	}
	const size_t first_chunk = std::min(n, size - end);
	memcpy(buf + end, data, first_chunk * sizeof(bufElementType));
	end = (end + first_chunk) % size;
	if (first_chunk < n) {
		const size_t second_chunk = n - first_chunk;
		memcpy(buf + end, data + first_chunk, second_chunk * sizeof(bufElementType));
		end = (end + second_chunk) % size;
	}
	if (begin == end) {
		wrap = true;
	}
	return n;
}

size_t RingBuffer::read(bufElementType * dest, size_t n)
{
	n = std::min(n, count());

	if (n == 0) {
		return n;
	}
	if (wrap) {
		wrap = false;
	}
	const size_t first_chunk = std::min(n, size - begin);
	memcpy(dest, buf + begin, first_chunk * sizeof(bufElementType));
	begin = (begin + first_chunk) % size;
	if (first_chunk < n) {
		const size_t second_chunk = n - first_chunk;
		memcpy(dest + first_chunk, buf + begin, second_chunk * sizeof(bufElementType));
		begin = (begin + second_chunk) % size;
	}
	return n;
}

size_t RingBuffer::count()
{
	if (end == begin) {
		return wrap ? size : 0;
	}
	else if (end > begin) {
		return end - begin;
	}
	else {
		return size + end - begin;
	}
}

size_t RingBuffer::getFree()
{
	return size - count();
}

