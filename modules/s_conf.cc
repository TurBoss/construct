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

extern "C" void default_conf(const string_view &prefix);
extern "C" void rehash_conf(const string_view &prefix, const bool &existing);
extern "C" void reload_conf();
extern "C" void refresh_conf();
static void init_conf_item(conf::item<> &);

// This module registers with conf::on_init to be called back
// when a conf item is initialized; when this module is unloaded
// we have to unregister that listener using this state.
decltype(conf::on_init)::const_iterator
conf_on_init_iter
{
	end(conf::on_init)
};

mapi::header
IRCD_MODULE
{
	"Server Configuration", []
	{
		conf_on_init_iter = conf::on_init.emplace(end(conf::on_init), init_conf_item);
		reload_conf();
	}, []
	{
		assert(conf_on_init_iter != end(conf::on_init));
		conf::on_init.erase(conf_on_init_iter);
	}
};

/// Set to false to quiet errors from a conf item failing to set
bool
item_error_log
{
	true
};

static void
on_run()
{
	// Suppress errors for this scope.
	const unwind uw{[] { item_error_log = true; }};
	item_error_log = false;
	rehash_conf({}, false);
}

/// Waits for the daemon to transition to the RUN state so we can gather all
/// of the registered conf items and save any new ones to the !conf room.
/// We can't do that on this module init for two reason:
/// - More conf items will load in other modules after this module.
/// - Events can't be safely sent to the !conf room until the RUN state.
const ircd::runlevel_changed
rehash_on_run{[]
(const auto &runlevel)
{
	if(runlevel == ircd::runlevel::RUN)
		ctx::context
		{
			"confhash", 256_KiB, on_run, ctx::context::POST
		};
}};

const m::room::id::buf
conf_room_id
{
	"conf", ircd::my_host()
};

m::room
conf_room
{
	conf_room_id
};

extern "C" m::event::id::buf
set_conf_item(const m::user::id &sender,
              const string_view &key,
              const string_view &val)
{
	return send(conf_room, sender, "ircd.conf.item", key,
	{
		{ "value", val }
	});
}

extern "C" void
get_conf_item(const string_view &key,
              const std::function<void (const string_view &)> &closure)
{
	conf_room.get("ircd.conf.item", key, [&closure]
	(const m::event &event)
	{
		const auto &value
		{
			unquote(at<"content"_>(event).at("value"))
		};

		closure(value);
	});
}

static void
conf_updated(const m::event &event)
noexcept try
{
	const auto &content
	{
		at<"content"_>(event)
	};

	const auto &key
	{
		at<"state_key"_>(event)
	};

	const string_view &value
	{
		unquote(content.at("value"))
	};

	if(runlevel == runlevel::START && !conf::exists(key))
		return;

	// Conf items marked with a persist=false property are not read from
	// the conf room into the item, even if the value exists in the room.
	if(conf::exists(key) && !conf::persists(key))
		return;

	log::debug
	{
		"Updating conf [%s] => %s", key, value
	};

	ircd::conf::set(key, value);
}
catch(const std::exception &e)
{
	if(item_error_log) log::error
	{
		"Failed to set conf item '%s' :%s",
		json::get<"state_key"_>(event),
		e.what()
	};
}

static void
handle_conf_updated(const m::event &event,
                    m::vm::eval &)
{
	conf_updated(event);
}

const m::hookfn<m::vm::eval &>
conf_updated_hook
{
	handle_conf_updated,
	{
		{ "_site",       "vm.effect"       },
		{ "room_id",     "!conf"           },
		{ "type",        "ircd.conf.item"  },
	}
};

static void
init_conf_items()
{
	const m::room::state state
	{
		conf_room
	};

	state.for_each("ircd.conf.item", []
	(const m::event &event)
	{
		conf_updated(event);
	});
}

static void
init_conf_item(conf::item<> &item)
{
	const m::room::state state
	{
		conf_room
	};

	state.get(std::nothrow, "ircd.conf.item", item.name, []
	(const m::event &event)
	{
		conf_updated(event);
	});
}

static void
handle_init_conf_items(const m::event &,
                       m::vm::eval &eval)
{
	init_conf_items();
}

const m::hookfn<m::vm::eval &>
init_conf_items_hook
{
	handle_init_conf_items,
	{
		{ "_site",       "vm.effect"      },
		{ "room_id",     "!ircd"          },
		{ "type",        "m.room.member"  },
		{ "membership",  "join"           },
		{ "state_key",   "@ircd"          },
	}
};

static m::event::id::buf
create_conf_item(const string_view &key,
                 const conf::item<> &item)
try
{
	thread_local char vbuf[4_KiB];
	const string_view &val
	{
		item.get(vbuf)
	};

	return set_conf_item(m::me.user_id, key, val);
}
catch(const std::exception &e)
{
	if(item_error_log) log::error
	{
		"Failed to create conf item '%s' :%s",
		key,
		e.what()
	};

	return {};
}

static void
create_conf_room(const m::event &,
                 m::vm::eval &)
{
	m::create(conf_room_id, m::me.user_id);
	rehash_conf({}, true);
}

const m::hookfn<m::vm::eval &>
create_conf_room_hook
{
	create_conf_room,
	{
		{ "_site",       "vm.effect"      },
		{ "room_id",     "!ircd"          },
		{ "type",        "m.room.create"  },
	}
};

void
rehash_conf(const string_view &prefix,
            const bool &existing)
{
	const m::room::state state
	{
		conf_room
	};

	for(const auto &p : conf::items)
	{
		const auto &key{p.first};
		if(prefix && !startswith(key, prefix))
			continue;

		const auto &item{p.second}; assert(item);

		// Conf items marked with a persist=false property are not written
		// to the conf room.
		if(!item->feature.get("persist", true))
			continue;

		// Use the `existing` argument to toggle a force-overwrite
		if(!existing)
			if(state.has("ircd.conf.item", key))
				continue;

		create_conf_item(key, *item);
	}
}

void
default_conf(const string_view &prefix)
{
	for(const auto &p : conf::items)
	{
		const auto &key{p.first};
		if(prefix && !startswith(key, prefix))
			continue;

		const auto &item{p.second}; assert(item);
		const auto value
		{
			unquote(item->feature["default"])
		};

		conf::set(key, value);
	}
}

void
reload_conf()
{
	init_conf_items();
}

void
refresh_conf()
{
	ircd::conf::reset();
}
