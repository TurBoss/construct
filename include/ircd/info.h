// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.


#pragma once
#define HAVE_IRCD_INFO_H

// legacy
extern "C" const char *const ircd_name;
extern "C" const char *const ircd_version;

/// Information & metadata about the library.
namespace ircd::info
{
	struct line;
	struct tc_version;

	// Primary information
	extern const string_view name;
	extern const string_view version;
	extern const string_view user_agent;
	extern const string_view server_agent;

	// Build information
	extern const string_view tag;
	extern const string_view branch;
	extern const string_view commit;
	extern const time_t configured_time;
	extern const time_t startup_time;
	extern const string_view configured;
	extern const string_view compiled;
	extern const string_view startup;

	// System / platform information
	extern const size_t page_size;
	extern const size_t max_align;
	extern const size_t hardware_concurrency;
	extern const size_t destructive_interference;
	extern const size_t constructive_interference;
	extern const size_t rlimit_as;
	extern const size_t rlimit_data;
	extern const size_t rlimit_rss;
	extern const size_t rlimit_nofile;
	extern const size_t rlimit_rttime;

	extern const int glibc_version[3];
	extern const string_view glibc_version_str;
	extern const int aio_reqprio_max;
	extern const size_t iov_max;
	#ifdef HAVE_SYS_UTSNAME_H
	extern const ::utsname utsname;
	#endif

	// Extended information
	extern const std::vector<info::line> myinfo;
	extern const std::vector<string_view> credits;

	void dump();
	void init();
}

struct ircd::info::line
{
	std::string key;
	std::string valstr;
	uint64_t valnum;
	std::string desc;
};
