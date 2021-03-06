// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#include <boost/version.hpp>
#include <ircd/asio.h>

/// Boost version indicator for compiled header files.
decltype(ircd::boost_version)
ircd::boost_version
{
	BOOST_VERSION / 100000,
	BOOST_VERSION / 100 % 1000,
	BOOST_VERSION % 100,
};

char ircd_boost_version_str_buf[32];
decltype(ircd::boost_version_str)
ircd::boost_version_str
(
	ircd_boost_version_str_buf,
	::snprintf(ircd_boost_version_str_buf, sizeof(ircd_boost_version_str_buf),
	           "%u.%u.%u",
	           boost_version[0],
	           boost_version[1],
	           boost_version[2])
);

/// Record of the ID of the thread static initialization took place on.
decltype(ircd::ios::static_thread_id)
ircd::ios::static_thread_id
{
    std::this_thread::get_id()
};

/// "main" thread for IRCd; the one the main context landed on.
decltype(ircd::ios::main_thread_id)
ircd::ios::main_thread_id;

decltype(ircd::ios::user)
ircd::ios::user;

void
ircd::ios::init(asio::io_context &user)
{
	// Sample the ID of this thread. Since this is the first transfer of
	// control to libircd after static initialization we have nothing to
	// consider a main thread yet. We need something set for many assertions
	// to pass until ircd::main() is entered which will reset this to where
	// ios.run() is really running.
	main_thread_id = std::this_thread::get_id();

	// Set a reference to the user's ios_service
	ios::user = &user;
}

void
ircd::ios::post(std::function<void ()> function)
{
	boost::asio::post(get(), std::move(function));
}

void
ircd::ios::dispatch(std::function<void ()> function)
{
	boost::asio::dispatch(get(), std::move(function));
}

boost::asio::io_context &
ircd::ios::get()
{
	assert(user);
	return *user;
}
