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
#define HAVE_IRCD_CTX_PROF_H

/// Profiling for the context system.
///
/// These facilities provide tools and statistics. The primary purpose here is
/// to alert developers of unwanted context behavior, in addition to optimizing
/// the overall performance of the context system.
///
/// The original use case is for the embedded database backend. Function calls
/// are made which may conduct blocking I/O before returning. This will hang the
/// current userspace context while it is running and thus BLOCK EVERY CONTEXT
/// in the entire IRCd. Since this is still an asynchronous system, it just
/// doesn't have callbacks: we do not do I/O without a cooperative yield.
/// Fortunately there are mechanisms to mitigate this -- but we have to know
/// for sure. A database call which has been passed over for mitigation may
/// start doing some blocking flush under load, etc. The profiler will alert
/// us of this so it doesn't silently degrade performance.
///
namespace ircd::ctx::prof
{
	enum class event;

	// lowlevel
	ulong rdtsc();

	// state accessors
	const ulong &total_slice_cycles();
	const ulong &cur_slice_start();
	ulong cur_slice_cycles();

	// called at the appropriate point to mark the event (internal use).
	void mark(const event &);
}

/// Profiling events for marking. These are currently used internally at the
/// appropriate point to mark(): the user of ircd::ctx has no reason to mark()
/// these events; this interface is not quite developed for general use yet.
enum class ircd::ctx::prof::event
{
	SPAWN,             // Context spawn requested
	JOIN,              // Context join requested
	JOINED,            // Context join completed
	CUR_ENTER,         // Current context entered
	CUR_LEAVE,         // Current context leaving
	CUR_YIELD,         // Current context yielding
	CUR_CONTINUE,      // Current context continuing
	CUR_INTERRUPT,     // Current context detects interruption
	CUR_TERMINATE,     // Current context detects termination
};

namespace ircd::ctx::prof::settings
{
	extern conf::item<double> stack_usage_warning;     // percentage
	extern conf::item<double> stack_usage_assertion;   // percentage

	extern conf::item<ulong> slice_warning;     // Warn when the yield-to-yield cycles exceeds
	extern conf::item<ulong> slice_interrupt;   // Interrupt exception when exceeded (not a signal)
	extern conf::item<ulong> slice_assertion;   // abort() when exceeded (not a signal, must yield)
}
