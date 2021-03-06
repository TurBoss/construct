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
#define HAVE_IRCD_CTX_WHEN_H

namespace ircd::ctx
{
	template<class it> future<void> when_all(it first, const it &last);
	template<class it> future<it> when_any(it first, const it &last);
}

// Internal interface
namespace ircd::ctx::when
{
	template<class T> auto &state(const future<T> &);
	void all_then(promise<void> &p);
	template<class it> void any_then(promise<it> &p, it &f);
	template<class it> void set_all_then(promise<void> &p, it &f);
	template<class it> void set_any_then(promise<it> &p, it &f);
}

/// Returns a future which becomes ready when any of the futures in the
/// iteration become ready or are already ready. The future that when_any()
/// eventually indicates is then considered "observed" which means you
/// are required to do nothing when including it in the next invocation of
/// when_any() and it won't be considered ready or pending again and the
/// collection does not have to be modified in any way.
///
/// The returned future's payload is an iterator into the collection as if
/// it were the result of an std::find() etc; thus to know its index an
/// std::distance is often satisfactory.
template<class it>
ircd::ctx::future<it>
ircd::ctx::when_any(it first,
                    const it &last)
{
	promise<it> p;
	future<it> ret(p);
	for(auto f(first); f != last; ++f)
		if(is(state(*f), future_state::READY))
		{
			set(when::state(*f), future_state::OBSERVED);
			p.set_value(f);
			return ret;
		}

	for(; first != last; ++first)
		if(is(state(*first), future_state::PENDING))
			when::set_any_then(p, first);

	if(refcount(p.state()) <= 1)
		p.set_value(first);

	return ret;
}

/// Returns a future which becomes ready when all of the futures in the
/// collection become ready. This future has a void payload to minimize
/// its cost since this indication is positively unate.
template<class it>
ircd::ctx::future<void>
ircd::ctx::when_all(it first,
                    const it &last)
{
	promise<void> p;
	future<void> ret(p);
	for(; first != last; ++first)
		if(is(state(*first), future_state::PENDING))
			when::set_all_then(p, first);

	if(refcount(p.state()) <= 1)
		p.set_value();

	return ret;
}

template<class it>
void
ircd::ctx::when::set_any_then(promise<it> &p,
                              it &f)
{
	when::state(*f).then = [p, f]          // TODO: quash this alloc
	(shared_state_base &sb) mutable
	{
		if(sb.then)
			any_then(p, f);
	};
}

template<class it>
void
ircd::ctx::when::set_all_then(promise<void> &p,
                              it &f)
{
	when::state(*f).then = [p]             // TODO: quash this alloc
	(shared_state_base &sb) mutable
	{
		if(sb.then)
			all_then(p);
	};
}

template<class it>
void
ircd::ctx::when::any_then(promise<it> &p,
                          it &f)
{
	if(!p.valid())
		return;

	set(when::state(*f), future_state::OBSERVED);
	p.set_value(f);
}

inline void
ircd::ctx::when::all_then(promise<void> &p)
{
	if(!p.valid())
		return;

	if(refcount(p.state()) < 2)
		return p.set_value();

	return p.remove(p.state(), p);
}

/// In order for this template to be reusable with std::set iterations we
/// have to make a const_cast at some point; this internal function does that.
template<class T>
auto &
ircd::ctx::when::state(const future<T> &f)
{
	return const_cast<future<T> &>(f).state();
}
