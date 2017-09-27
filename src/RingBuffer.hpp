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

#ifndef ZT_RINGBUFFER_HPP
#define ZT_RINGBUFFER_HPP

#include <memory.h>
#include <algorithm>

namespace ZeroTier {

	template<typename T> class RingBuffer {

	private:
		T * buf;
		size_t size;
		size_t begin;
		size_t end;
		bool wrap;

	public:
		/**
		* create a RingBuffer with space for up to size elements.
		*/
		explicit RingBuffer(size_t size)
			: size(size),
			begin(0),
			end(0),
			wrap(false)
		{
			buf = new T[size];
		}

		RingBuffer(const RingBuffer<T> & ring)
		{
			this(ring.size);
			begin = ring.begin;
			end = ring.end;
			memcpy(buf, ring.buf, sizeof(T) * size);
		}

		~RingBuffer()
		{
			delete[] buf;
		}

		// get a reference to the underlying buffer
		T* get_buf()
		{
			return buf + begin;
		}

		// adjust buffer index pointer as if we copied data in
		size_t produce(size_t n)
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

		// adjust buffer index pointer as if we copied data out
		size_t consume(size_t n)
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

		size_t write(const T * data, size_t n)
		{
			n = std::min(n, getFree());

			if (n == 0) {
				return n;
			}
			const size_t first_chunk = std::min(n, size - end);
			memcpy(buf + end, data, first_chunk * sizeof(T));
			end = (end + first_chunk) % size;
			if (first_chunk < n) {
				const size_t second_chunk = n - first_chunk;
				memcpy(buf + end, data + first_chunk, second_chunk * sizeof(T));
				end = (end + second_chunk) % size;
			}
			if (begin == end) {
				wrap = true;
			}
			return n;
		}

		size_t read(T * dest, size_t n)
		{
			n = std::min(n, count());

			if (n == 0) {
				return n;
			}
			if (wrap) {
				wrap = false;
			}
			const size_t first_chunk = std::min(n, size - begin);
			memcpy(dest, buf + begin, first_chunk * sizeof(T));
			begin = (begin + first_chunk) % size;
			if (first_chunk < n) {
				const size_t second_chunk = n - first_chunk;
				memcpy(dest + first_chunk, buf + begin, second_chunk * sizeof(T));
				begin = (begin + second_chunk) % size;
			}
			return n;
		}

		size_t count() {
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

		size_t getFree() {
			return size - count();
		}
 	};
}
#endif // ZT_RINGBUFFER_HPP
