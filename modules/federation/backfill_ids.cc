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
	"federation backfill event IDs"
};

resource
backfill_ids_resource
{
	"/_matrix/federation/v1/backfill_ids/",
	{
		"federation backfill ID's",
		resource::DIRECTORY,
	}
};

conf::item<size_t>
backfill_ids_limit_max
{
	{ "name",     "ircd.federation.backfill_ids.limit.max" },
	{ "default",  2048L /* TODO: hiiigherrrr */            },
};

conf::item<size_t>
backfill_ids_limit_default
{
	{ "name",     "ircd.federation.backfill_ids.limit.default" },
	{ "default",  64L                                          },
};

static size_t
calc_limit(const resource::request &request)
{
	const auto &limit
	{
		request.query["limit"]
	};

	if(!limit)
		return size_t(backfill_ids_limit_default);

	size_t ret
	{
		lex_cast<size_t>(limit)
	};

	return std::min(ret, size_t(backfill_ids_limit_max));
}

resource::response
get__backfill_ids(client &client,
                  const resource::request &request)
{
	m::room::id::buf room_id
	{
		url::decode(request.parv[0], room_id)
	};

	m::event::id::buf event_id
	{
		request.query["v"]?
			url::decode(request.query.at("v"), event_id):
			m::head(room_id)
	};

	const size_t limit
	{
		calc_limit(request)
	};

	m::room::messages it
	{
		room_id, event_id
	};

	const unique_buffer<mutable_buffer> buf
	{
		4_KiB
	};

	resource::response::chunked response
	{
		client, http::OK
	};

	json::stack out{buf, [&response]
	(const const_buffer &buf)
	{
		response.write(buf);
		return buf;
	}};

	json::stack::object top{out};
	json::stack::member pdus_m
	{
		top, "pdu_ids"
	};

	json::stack::array pdus
	{
		pdus_m
	};

	size_t count{0};
	for(; it && count < limit; ++count, --it)
		pdus.append(it.event_id());

	return {};
}

resource::method
method_get
{
	backfill_ids_resource, "GET", get__backfill_ids,
	{
		method_get.VERIFY_ORIGIN
	}
};
