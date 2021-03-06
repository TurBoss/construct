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
	"Client 7.4.2.3 :Join"
};

resource
join_resource
{
	"/_matrix/client/r0/join/",
	{
		"(7.4.2.3) Join room_id or alias.",
		resource::DIRECTORY,
	}
};

static resource::response
_post__join(client &client,
            const resource::request &request,
            const m::room::id &room_id);

static resource::response
_post__join(client &client,
            const resource::request &request,
            const m::room::alias &room_alias);

resource::response
post__join(client &client,
           const resource::request &request)
{
	if(request.parv.size() < 1)
		throw http::error
		{
			http::MULTIPLE_CHOICES, "/join room_id or room_alias required"
		};

	char idbuf[256];
	const auto &id
	{
		url::decode(idbuf, request.parv[0])
	};

	switch(m::sigil(id))
	{
		case m::id::ROOM:
			return _post__join(client, request, m::room::id{id});

		case m::id::ROOM_ALIAS:
			return _post__join(client, request, m::room::alias{id});

		default: throw m::UNSUPPORTED
		{
			"Cannot join a room using a '%s' MXID", reflect(m::sigil(id))
		};
	}
}

resource::method
method_post
{
	join_resource, "POST", post__join,
	{
		method_post.REQUIRES_AUTH
	}
};

static resource::response
_post__join(client &client,
            const resource::request &request,
            const m::room::alias &room_alias)
{
	m::join(room_alias, request.user_id);

	return resource::response
	{
		client, json::members
		{
			{ "room_id", m::room_id(room_alias) }
		}
	};
}

/// This function forwards the join request to the /rooms/{room_id}/join
/// module so they can reuse the same code. That's done with the m::join()
/// convenience linkage.
static resource::response
_post__join(client &client,
            const resource::request &request,
            const m::room::id &room_id)
{
	const m::room room
	{
		room_id
	};

	m::join(room, request.user_id);

	return resource::response
	{
		client, json::members
		{
			{ "room_id", room_id }
		}
	};
}
