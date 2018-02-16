// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#include <ircd/m/m.h>

ircd::m::filter::filter(const user &user,
                        const string_view &filter_id,
                        const mutable_buffer &buf)
{
	get(user, filter_id, [this, &buf]
	(const json::object &filter)
	{
		const size_t len
		{
			copy(buf, string_view{filter})
		};

		new (this) m::filter
		{
			json::object
			{
				data(buf), len
			}
		};
	});
}

ircd::string_view
ircd::m::filter::set(const mutable_buffer &idbuf,
                     const user &user,
                     const json::object &filter)
{
	const auto user_room_id
	{
		user.room_id()
	};

	const m::room room
	{
		user_room_id
	};

	const sha256::buf hash
	{
		sha256{filter}
	};

	const string_view filter_id
	{
		b64encode_unpadded(idbuf, hash)
	};

	send(room, user.user_id, "ircd.filter", filter_id, filter);
	return filter_id;
}

void
ircd::m::filter::get(const user &user,
                     const string_view &filter_id,
                     const closure &closure)
{
	if(!get(std::nothrow, user, filter_id, closure))
		throw m::NOT_FOUND
		{
			"Filter not found"
		};
}

bool
ircd::m::filter::get(std::nothrow_t,
                     const user &user,
                     const string_view &filter_id,
                     const closure &closure)
{
	const auto user_room_id
	{
		user.room_id()
	};

	const m::room room
	{
		user_room_id
	};

	return room.get(std::nothrow, "ircd.filter", filter_id, [&closure]
	(const m::event &event)
	{
		const json::object &content
		{
			at<"content"_>(event)
		};

		closure(content);
	});
}