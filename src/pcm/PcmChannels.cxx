/*
 * Copyright (C) 2003-2013 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "PcmChannels.hxx"
#include "PcmBuffer.hxx"
#include "Traits.hxx"
#include "AudioFormat.hxx"
#include "util/ConstBuffer.hxx"

#include <assert.h>

template<typename D, typename S>
static void
MonoToStereo(D dest, S src, S end)
{
	while (src != end) {
		const auto value = *src++;

		*dest++ = value;
		*dest++ = value;
	}

}

template<SampleFormat F, class Traits=SampleTraits<F>>
static typename Traits::value_type
StereoToMono(typename Traits::value_type _a,
	     typename Traits::value_type _b)
{
	typename Traits::sum_type a(_a);
	typename Traits::sum_type b(_b);

	return typename Traits::value_type((a + b) / 2);
}

template<SampleFormat F, class Traits=SampleTraits<F>>
static typename Traits::pointer_type
StereoToMono(typename Traits::pointer_type dest,
	     typename Traits::const_pointer_type src,
	     typename Traits::const_pointer_type end)
{
	while (src != end) {
		const auto a = *src++;
		const auto b = *src++;

		*dest++ = StereoToMono<F, Traits>(a, b);
	}

	return dest;
}

template<SampleFormat F, class Traits=SampleTraits<F>>
static typename Traits::pointer_type
NToStereo(typename Traits::pointer_type dest,
	  unsigned src_channels,
	  typename Traits::const_pointer_type src,
	  typename Traits::const_pointer_type end)
{
	assert((end - src) % src_channels == 0);

	while (src != end) {
		typename Traits::sum_type sum = *src++;
		for (unsigned c = 1; c < src_channels; ++c)
			sum += *src++;

		typename Traits::value_type value(sum / int(src_channels));

		/* TODO: this is actually only mono ... */
		*dest++ = value;
		*dest++ = value;
	}

	return dest;
}

template<SampleFormat F, class Traits=SampleTraits<F>>
static typename Traits::pointer_type
NToM(typename Traits::pointer_type dest,
     unsigned dest_channels,
     unsigned src_channels,
     typename Traits::const_pointer_type src,
     typename Traits::const_pointer_type end)
{
	assert((end - src) % src_channels == 0);

	while (src != end) {
		typename Traits::sum_type sum = *src++;
		for (unsigned c = 1; c < src_channels; ++c)
			sum += *src++;

		typename Traits::value_type value(sum / int(src_channels));

		/* TODO: this is actually only mono ... */
		for (unsigned c = 0; c < dest_channels; ++c)
			*dest++ = value;
	}

	return dest;
}

template<SampleFormat F, class Traits=SampleTraits<F>>
static ConstBuffer<typename Traits::value_type>
ConvertChannels(PcmBuffer &buffer,
		unsigned dest_channels,
		unsigned src_channels,
		ConstBuffer<typename Traits::value_type> src)
{
	assert(src.size % src_channels == 0);

	const size_t dest_size = src.size / src_channels * dest_channels;
	auto dest = buffer.GetT<typename Traits::value_type>(dest_size);

	if (src_channels == 1 && dest_channels == 2)
		MonoToStereo(dest, src.begin(), src.end());
	else if (src_channels == 2 && dest_channels == 1)
		StereoToMono<F>(dest, src.begin(), src.end());
	else if (dest_channels == 2)
		NToStereo<F>(dest, src_channels, src.begin(), src.end());
	else
		NToM<F>(dest, dest_channels,
			src_channels, src.begin(), src.end());

	return { dest, dest_size };
}

ConstBuffer<int16_t>
pcm_convert_channels_16(PcmBuffer &buffer,
			unsigned dest_channels,
			unsigned src_channels,
			ConstBuffer<int16_t> src)
{
	return ConvertChannels<SampleFormat::S16>(buffer, dest_channels,
						  src_channels, src);
}

ConstBuffer<int32_t>
pcm_convert_channels_24(PcmBuffer &buffer,
			unsigned dest_channels,
			unsigned src_channels,
			ConstBuffer<int32_t> src)
{
	return ConvertChannels<SampleFormat::S24_P32>(buffer, dest_channels,
						      src_channels, src);
}

ConstBuffer<int32_t>
pcm_convert_channels_32(PcmBuffer &buffer,
			unsigned dest_channels,
			unsigned src_channels,
			ConstBuffer<int32_t> src)
{
	return ConvertChannels<SampleFormat::S32>(buffer, dest_channels,
						  src_channels, src);
}

ConstBuffer<float>
pcm_convert_channels_float(PcmBuffer &buffer,
			   unsigned dest_channels,
			   unsigned src_channels,
			   ConstBuffer<float> src)
{
	return ConvertChannels<SampleFormat::FLOAT>(buffer, dest_channels,
						    src_channels, src);
}
