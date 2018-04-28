// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

using namespace ircd;

mapi::header
IRCD_MODULE
{
	"Federation :General Library and Utils"
};

extern "C" void
feds__version(const m::room::id &room_id, std::ostream &out)
{
	struct req
	:m::v1::version
	{
		char origin[256];
		char buf[16_KiB];

		req(m::v1::version::opts &&opts)
		:m::v1::version{buf, std::move(opts)}
		{}
	};

	std::list<req> reqs;
	const m::room::origins origins{room_id};
	origins.for_each([&out, &reqs]
	(const string_view &origin)
	{
		m::v1::version::opts opts;
		opts.remote = origin;
		opts.dynamic = false; try
		{
			reqs.emplace_back(std::move(opts));
		}
		catch(const std::exception &e)
		{
			out << "! " << origin << " " << e.what() << std::endl;
			return;
		}

		strlcpy(reqs.back().origin, origin);
	});

	auto all
	{
		ctx::when_all(begin(reqs), end(reqs))
	};

	all.wait(30s, std::nothrow);

	for(auto &req : reqs) try
	{
		if(req.wait(1ms, std::nothrow))
		{
			const auto code{req.get()};
			const json::object &response{req};
			out << "+ " << std::setw(40) << std::left << req.origin
			    << " " << string_view{response}
			    << std::endl;
		}
		else cancel(req);
	}
	catch(const std::exception &e)
	{
		out << "- " << std::setw(40) << std::left << req.origin
		    << " " << e.what()
		    << std::endl;
	}
}

extern "C" void
feds__event(const m::event::id &event_id, std::ostream &out)
{
	const m::room::id::buf room_id{[&event_id]
	{
		const m::event::fetch event
		{
			event_id
		};

		return m::room::id::buf{at<"room_id"_>(event)};
	}()};

	struct req
	:m::v1::event
	{
		char origin[256];
		char buf[96_KiB];

		req(const m::event::id &event_id, m::v1::event::opts opts)
		:m::v1::event{event_id, buf, std::move(opts)}
		{}
	};

	std::list<req> reqs;
	const m::room::origins origins{room_id};
	origins.for_each([&out, &event_id, &reqs]
	(const string_view &origin)
	{
		m::v1::event::opts opts;
		opts.remote = origin;
		opts.dynamic = false; try
		{
			reqs.emplace_back(event_id, std::move(opts));
		}
		catch(const std::exception &e)
		{
			out << "! " << origin << " " << e.what() << std::endl;
			return;
		}

		strlcpy(reqs.back().origin, origin);
	});

	auto all
	{
		ctx::when_all(begin(reqs), end(reqs))
	};

	all.wait(30s, std::nothrow);

	for(auto &req : reqs) try
	{
		if(req.wait(1ms, std::nothrow))
		{
			const auto code{req.get()};
			out << "+ " << req.origin << " " << http::status(code) << std::endl;
		}
		else cancel(req);
	}
	catch(const std::exception &e)
	{
		out << "- " << req.origin << " " << e.what() << std::endl;
	}

	return;
}