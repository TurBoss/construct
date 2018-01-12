// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once
#define HAVE_IRCD_UTIL_SYSCALL_H

// Declaring this here eliminates the need to include <unistd.h> for it
extern "C" long syscall(long, ...) noexcept;

namespace ircd::util
{
	template<class function, class... args> long syscall(function&& f, args&&... a);
	template<long number, class... args> long syscall(args&&... a);

	template<class function, class... args> long syscall_nointr(function&& f, args&&... a);
	template<long number, class... args> long syscall_nointr(args&&... a);
}

/// Posix system call template to check for returned error value and throw the
/// approps errno in the proper std::system_error exception. Note the usage
/// here, the libc wrapper function is the first argument i.e:
/// syscall(::foo, bar, baz) not syscall(::foo(bar, baz));
///
template<class function,
         class... args>
long
ircd::util::syscall(function&& f,
                    args&&... a)
{
	const auto ret
	{
		f(std::forward<args>(a)...)
	};

	if(unlikely(ret == -1))
		throw std::system_error
		{
			errno, std::system_category()
		};

	return ret;
}

/// Posix system call template to check for returned error value and throw the
/// approps errno in the proper std::system_error exception. This template
/// requires a system call number in the parameters. The arguments are only
/// the actual arguments passed to the syscall because the number is given
/// in the template.
///
template<long number,
         class... args>
long
ircd::util::syscall(args&&... a)
{
	const auto ret
	{
		::syscall(number, std::forward<args>(a)...)
	};

	if(unlikely(ret == -1))
		throw std::system_error
		{
			errno, std::system_category()
		};

	return ret;
}

/// Uninterruptible posix system call template to check for returned error
/// value and throw the approps errno in the proper std::system_error
/// exception. Note the usage here, the libc wrapper function is the first
/// argument i.e: syscall(::foo, bar, baz) not syscall(::foo(bar, baz));
///
/// The syscall is restarted until it no longer returns with EINTR.
///
template<class function,
         class... args>
long
ircd::util::syscall_nointr(function&& f,
                           args&&... a)
{
	long ret; do
	{
		ret = f(std::forward<args>(a)...);
	}
	while(unlikely(ret == -1 && errno == EINTR));

	if(unlikely(ret == -1))
		throw std::system_error
		{
			errno, std::system_category()
		};

	return ret;
}

/// Uninterruptible posix system call template to check for returned error
/// value and throw the approps errno in the proper std::system_error
/// exception. This template requires a system call number in the parameters.
/// The arguments are only the actual arguments passed to the syscall because
/// the number is given in the template.
///
/// The syscall is restarted until it no longer returns with EINTR.
///
template<long number,
         class... args>
long
ircd::util::syscall_nointr(args&&... a)
{
	long ret; do
	{
		ret = ::syscall(number, std::forward<args>(a)...);
	}
	while(unlikely(ret == -1 && errno == EINTR));

	if(unlikely(ret == -1))
		throw std::system_error
		{
			errno, std::system_category()
		};

	return ret;
}