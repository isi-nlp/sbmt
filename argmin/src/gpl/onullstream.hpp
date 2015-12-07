/*  $Id: onullstream.hpp 1257 2006-09-19 01:17:10Z jturian $ */
/*!
 *  \file onullstream.hpp
 *  $LastChangedDate: 2006-09-18 18:17:10 -0700 (Mon, 18 Sep 2006) $
 *  $Revision: 1257 $
 */
/*!
 *  \class onullstream
 *  \brief WRITEME
 *
 *  \author Dietmar Kuehl
 *  \note This code, by Dietmar Kuehl, was cribbed from comp.lang.c++,
 *  message <7kehjm$jdj$2@news.BelWue.DE>#1/1
 *  http://groups.google.com/group/comp.lang.c++/msg/4a81a74500f9f4d3?dmode=source
 *
 */

#ifndef __ONULLSTREAM_HPP__
#define  __ONULLSTREAM_HPP__

#include <streambuf>
#include <ostream>

template <class cT, class traits = std::char_traits<cT> >
class basic_nullbuf: public std::basic_streambuf<cT, traits>
{
	typename traits::int_type overflow(typename traits::int_type c)
	{
		return traits::not_eof(c); // indicate success
	}
};

template <class cT, class traits = std::char_traits<cT> >
class basic_onullstream: public std::basic_ostream<cT, traits>
{
	public:
		basic_onullstream():
			std::basic_ios<cT, traits>(&m_sbuf),
			std::basic_ostream<cT, traits>(&m_sbuf)
		{
			init(&m_sbuf);
		}

	private:
		basic_nullbuf<cT, traits> m_sbuf;
};

typedef basic_onullstream<char> onullstream;
typedef basic_onullstream<wchar_t> wonullstream;

#endif /* __ONULLSTREAM_HPP__ */
