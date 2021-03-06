// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

std::string
ircd::m::pretty(const event &event)
{
	std::string ret;
	std::stringstream s;
	pubsetbuf(s, ret, 4096);
	pretty(s, event);
	resizebuf(s, ret);
	return ret;
}

std::ostream &
ircd::m::pretty(std::ostream &s,
                const event &e)
{
	using prototype = void (std::ostream &, const event &);
	static mods::import<prototype> pretty
	{
		"m_event", "pretty__event"
	};

	pretty(s, e);
	return s;
}

std::string
ircd::m::pretty_oneline(const event &event,
                        const bool &content_keys)
{
	std::string ret;
	std::stringstream s;
	pubsetbuf(s, ret, 4096);
	pretty_oneline(s, event, content_keys);
	resizebuf(s, ret);
	return ret;
}

std::ostream &
ircd::m::pretty_oneline(std::ostream &s,
                        const event &e,
                        const bool &content_keys)
{
	using prototype = void (std::ostream &, const event &, const bool &);
	static mods::import<prototype> pretty_oneline
	{
		"m_event", "pretty_oneline__event"
	};

	pretty_oneline(s, e, content_keys);
	return s;
}

std::string
ircd::m::pretty_msgline(const event &event)
{
	std::string ret;
	std::stringstream s;
	pubsetbuf(s, ret, 4096);
	pretty_msgline(s, event);
	resizebuf(s, ret);
	return ret;
}

std::ostream &
ircd::m::pretty_msgline(std::ostream &s,
                        const event &e)
{
	using prototype = void (std::ostream &, const event &);
	static mods::import<prototype> pretty_msgline
	{
		"m_event", "pretty_msgline__event"
	};

	pretty_msgline(s, e);
	return s;
}

std::string
ircd::m::pretty(const event::prev &prev)
{
	std::string ret;
	std::stringstream s;
	pubsetbuf(s, ret, 4096);
	pretty(s, prev);
	resizebuf(s, ret);
	return ret;
}

std::ostream &
ircd::m::pretty(std::ostream &s,
                const event::prev &prev)
{
	using prototype = void (std::ostream &, const event::prev &);
	static mods::import<prototype> pretty
	{
		"m_event", "pretty__prev"
	};

	pretty(s, prev);
	return s;
}

std::string
ircd::m::pretty_oneline(const event::prev &prev)
{
	std::string ret;
	std::stringstream s;
	pubsetbuf(s, ret, 4096);
	pretty_oneline(s, prev);
	resizebuf(s, ret);
	return ret;
}

std::ostream &
ircd::m::pretty_oneline(std::ostream &s,
                        const event::prev &prev)
{
	using prototype = void (std::ostream &, const event::prev &);
	static mods::import<prototype> pretty_oneline
	{
		"m_event", "pretty_oneline__prev"
	};

	pretty_oneline(s, prev);
	return s;
}

ircd::m::id::event
ircd::m::make_id(const event &event,
                 id::event::buf &buf)
{
	const crh::sha256::buf hash{event};
	return make_id(event, buf, hash);
}

ircd::m::id::event
ircd::m::make_id(const event &event,
                 id::event::buf &buf,
                 const const_buffer &hash)
{
	char readable[b58encode_size(sha256::digest_size)];
	const id::event ret
	{
		buf, b58encode(readable, hash), my_host()
	};

	buf.assigned(ret);
	return ret;
}

bool
ircd::m::before(const event &a,
                const event &b)
{
	const event::prev prev{b};
	return prev.prev_events_has(at<"event_id"_>(a));
}

bool
ircd::m::operator>=(const event &a, const event &b)
{
	assert(json::get<"room_id"_>(a) == json::get<"room_id"_>(b));
	return at<"depth"_>(a) >= at<"depth"_>(b);
}

bool
ircd::m::operator<=(const event &a, const event &b)
{
	assert(json::get<"room_id"_>(a) == json::get<"room_id"_>(b));
	return at<"depth"_>(a) <= at<"depth"_>(b);
}

bool
ircd::m::operator>(const event &a, const event &b)
{
	assert(json::get<"room_id"_>(a) == json::get<"room_id"_>(b));
	return at<"depth"_>(a) > at<"depth"_>(b);
}

bool
ircd::m::operator<(const event &a, const event &b)
{
	assert(json::get<"room_id"_>(a) == json::get<"room_id"_>(b));
	return at<"depth"_>(a) < at<"depth"_>(b);
}

bool
ircd::m::operator==(const event &a, const event &b)
{
	assert(json::get<"room_id"_>(a) == json::get<"room_id"_>(b));
	return at<"event_id"_>(a) == at<"event_id"_>(b);
}

bool
ircd::m::bad(const id::event &event_id)
{
	bool ret {false};
	index(event_id, std::nothrow, [&ret]
	(const event::idx &event_idx)
	{
		ret = event_idx == 0;
	});

	return ret;
}

bool
ircd::m::good(const id::event &event_id)
{
	return index(event_id, std::nothrow) != 0;
}

bool
ircd::m::exists(const id::event &event_id,
                const bool &good)
{
	return good?
		m::good(event_id):
		m::exists(event_id);
}

bool
ircd::m::exists(const id::event &event_id)
{
	auto &column
	{
		dbs::event_idx
	};

	return has(column, event_id);
}

void
ircd::m::check_size(const event &event)
{
	const size_t &event_size
	{
		serialized(event)
	};

	if(event_size > size_t(event::max_size))
		throw m::BAD_JSON
		{
			"Event is %zu bytes which is larger than the maximum %zu bytes",
			event_size,
			size_t(event::max_size)
		};
}

bool
ircd::m::check_size(std::nothrow_t,
                    const event &event)
{
	const size_t &event_size
	{
		serialized(event)
	};

	return event_size <= size_t(event::max_size);
}

ircd::string_view
ircd::m::membership(const event &event)
{
	return json::get<"membership"_>(event)?
		string_view{json::get<"membership"_>(event)}:
		unquote(json::get<"content"_>(event).get("membership"));
}

size_t
ircd::m::degree(const event &event)
{
	return degree(event::prev{event});
}

size_t
ircd::m::degree(const event::prev &prev)
{
	size_t ret{0};
	json::for_each(prev, [&ret]
	(const auto &, const json::array &prevs)
	{
		ret += prevs.count();
	});

	return ret;
}

size_t
ircd::m::count(const event::prev &prev)
{
	size_t ret{0};
	m::for_each(prev, [&ret](const event::id &event_id)
	{
		++ret;
	});

	return ret;
}

void
ircd::m::for_each(const event::prev &prev,
                  const event::id::closure &closure)
{
	m::for_each(prev, event::id::closure_bool{[&closure]
	(const event::id &event_id)
	{
		closure(event_id);
		return true;
	}});
}

bool
ircd::m::for_each(const event::prev &prev,
                  const event::id::closure_bool &closure)
{
	return json::until(prev, [&closure]
	(const auto &key, const json::array &prevs)
	{
		for(const json::array &prev : prevs)
			if(!closure(event::id(unquote(prev.at(0)))))
				return false;

		return true;
	});
}

bool
ircd::m::my(const event &event)
{
	const auto &origin(json::get<"origin"_>(event));
	const auto &eid(json::get<"event_id"_>(event));
	return
		origin?
			my_host(origin):
		eid?
			my(event::id(eid)):
		false;
}

bool
ircd::m::my(const id::event &event_id)
{
	return self::host(event_id.host());
}

//
// event
//

/// The maximum size of an event we will create. This may also be used in
/// some contexts for what we will accept, but the protocol limit and hard
/// worst-case buffer size is still event::MAX_SIZE.
ircd::conf::item<size_t>
ircd::m::event::max_size
{
	{ "name",     "m.event.max_size" },
	{ "default",   65507L            },
};

//
// event::event
//

ircd::m::event::event(const id &id,
                      const mutable_buffer &buf)
:event
{
	index(id), buf
}
{
}

ircd::m::event::event(const idx &idx,
                      const mutable_buffer &buf)
{
	assert(bool(dbs::events));

	db::gopts opts;
	for(size_t i(0); i < dbs::event_column.size(); ++i)
	{
		const db::cell cell
		{
			dbs::event_column[i], byte_view<string_view>{idx}, opts
		};

		db::assign(*this, cell, byte_view<string_view>{idx});
	}

	const json::object obj
	{
		string_view
		{
			data(buf), json::print(buf, *this)
		}
	};

	new (this) m::event(obj);
}

namespace ircd::m
{
	static json::object make_hashes(const mutable_buffer &out, const sha256::buf &hash);
}

ircd::json::object
ircd::m::hashes(const mutable_buffer &out,
                const event &event)
{
	const sha256::buf hash_
	{
		hash(event)
	};

	return make_hashes(out, hash_);
}

ircd::json::object
ircd::m::event::hashes(const mutable_buffer &out,
                       json::iov &event,
                       const string_view &content)
{
	const sha256::buf hash_
	{
		hash(event, content)
	};

	return make_hashes(out, hash_);
}

ircd::json::object
ircd::m::make_hashes(const mutable_buffer &out,
                     const sha256::buf &hash)
{
	static const auto b64bufsz(b64encode_size(sizeof(hash)));
	thread_local char hashb64buf[b64bufsz];
	const json::members hashes
	{
		{ "sha256", b64encode_unpadded(hashb64buf, hash) }
	};

	return json::stringify(mutable_buffer{out}, hashes);
}

ircd::sha256::buf
ircd::m::event::hash(json::iov &event,
                     const string_view &content)
{
	const json::iov::push _content
	{
		event, { "content", content }
	};

	return m::hash(event);
}

ircd::sha256::buf
ircd::m::hash(const event &event)
{
	thread_local char buf[64_KiB];
	string_view preimage;

	//TODO: tuple::keys::selection
	if(defined(json::get<"signatures"_>(event)) ||
	   defined(json::get<"hashes"_>(event)))
	{
		m::event event_{event};
		json::get<"signatures"_>(event_) = {};
		json::get<"hashes"_>(event_) = {};
		preimage =
		{
			stringify(buf, event_)
		};
	}
	else preimage =
	{
		stringify(buf, event)
	};

	const sha256::buf hash
	{
		sha256{preimage}
	};

	return hash;
}

bool
ircd::m::verify_hash(const event &event)
{
	const sha256::buf hash
	{
		m::hash(event)
	};

	return verify_hash(event, hash);
}

bool
ircd::m::verify_hash(const event &event,
                     const sha256::buf &hash)
{
	static const size_t hashb64sz
	{
		size_t(hash.size() * 1.34) + 1
	};

	thread_local char b64buf[hashb64sz];
	return verify_sha256b64(event, b64encode_unpadded(b64buf, hash));
}

bool
ircd::m::verify_sha256b64(const event &event,
                          const string_view &b64)
try
{
	const json::object &object
	{
		at<"hashes"_>(event)
	};

	const string_view &hash
	{
		unquote(object.at("sha256"))
	};

	return hash == b64;
}
catch(const json::not_found &)
{
	return false;
}

ircd::json::object
ircd::m::event::signatures(const mutable_buffer &out,
                           json::iov &event,
                           const json::iov &content)
{
	const ed25519::sig sig
	{
		sign(event, content)
	};

	thread_local char sigb64buf[b64encode_size(sizeof(sig))];
	const json::members sigb64
	{
		{ self::public_key_id, b64encode_unpadded(sigb64buf, sig) }
	};

	const json::members sigs
	{
		{ my_host(), sigb64 }
    };

	return json::stringify(mutable_buffer{out}, sigs);
}

ircd::m::event
ircd::m::signatures(const mutable_buffer &out_,
                    const m::event &event_)
{
	thread_local char content[64_KiB];
	m::event event
	{
		essential(event_, content)
	};

	thread_local char buf[64_KiB];
	const json::object &preimage
	{
		stringify(buf, event)
	};

	const ed25519::sig sig
	{
		sign(preimage)
	};

	thread_local char sigb64buf[b64encode_size(sizeof(sig))];
	const json::member my_sig
	{
		my_host(), json::members
		{
			{ self::public_key_id, b64encode_unpadded(sigb64buf, sig) }
		}
	};

	static const size_t SIG_MAX{64};
	thread_local std::array<json::member, SIG_MAX> sigs;

	size_t i(0);
	sigs.at(i++) = my_sig;
	for(const auto &other : json::get<"signatures"_>(event_))
		if(!my_host(unquote(other.first)))
			sigs.at(i++) = { other.first, other.second };

	event = event_;
	mutable_buffer out{out_};
	json::get<"signatures"_>(event) = json::stringify(out, sigs.data(), sigs.data() + i);
	return event;
}

ircd::ed25519::sig
ircd::m::event::sign(json::iov &event,
                     const json::iov &contents)
{
	return sign(event, contents, self::secret_key);
}

ircd::ed25519::sig
ircd::m::event::sign(json::iov &event,
                     const json::iov &contents,
                     const ed25519::sk &sk)
{
	ed25519::sig sig;
	essential(event, contents, [&sk, &sig]
	(json::iov &event)
	{
		sig = m::sign(event, sk);
	});

	return sig;
}

ircd::ed25519::sig
ircd::m::sign(const event &event)
{
	return sign(event, self::secret_key);
}

ircd::ed25519::sig
ircd::m::sign(const event &event,
              const ed25519::sk &sk)
{
	thread_local char buf[64_KiB];
	const string_view preimage
	{
		stringify(buf, event)
	};

	return event::sign(preimage, sk);
}

ircd::ed25519::sig
ircd::m::event::sign(const json::object &event)
{
	return sign(event, self::secret_key);
}

ircd::ed25519::sig
ircd::m::event::sign(const json::object &event,
                     const ed25519::sk &sk)
{
	//TODO: skip rewrite
	thread_local char buf[64_KiB];
	const string_view preimage
	{
		stringify(buf, event)
	};

	return sign(preimage, sk);
}

ircd::ed25519::sig
ircd::m::event::sign(const string_view &event)
{
	return sign(event, self::secret_key);
}

ircd::ed25519::sig
ircd::m::event::sign(const string_view &event,
                     const ed25519::sk &sk)
{
	const ed25519::sig sig
	{
		sk.sign(event)
	};

	return sig;
}
bool
ircd::m::verify(const event &event)
{
	const string_view &origin
	{
		at<"origin"_>(event)
	};

	return verify(event, origin);
}

bool
ircd::m::verify(const event &event,
                const string_view &origin)
{
	const json::object &signatures
	{
		at<"signatures"_>(event)
	};

	const json::object &origin_sigs
	{
		signatures.at(origin)
	};

	for(const auto &p : origin_sigs)
		if(verify(event, origin, unquote(p.first)))
			return true;

	return false;
}

bool
ircd::m::verify(const event &event,
                const string_view &origin,
                const string_view &keyid)
try
{
	const m::node::id::buf node_id
	{
		m::node::id::origin, origin
	};

	const m::node node
	{
		node_id
	};

	bool ret{false};
	node.key(keyid, [&ret, &event, &origin, &keyid]
	(const ed25519::pk &pk)
	{
		ret = verify(event, pk, origin, keyid);
	});

	return ret;
}
catch(const m::NOT_FOUND &e)
{
	log::derror
	{
		"Failed to verify %s because key %s for %s :%s",
		string_view{json::get<"event_id"_>(event)},
		keyid,
		origin,
		e.what()
	};

	return false;
}

bool
ircd::m::verify(const event &event,
                const ed25519::pk &pk,
                const string_view &origin,
                const string_view &keyid)
{
	const json::object &signatures
	{
		at<"signatures"_>(event)
	};

	const json::object &origin_sigs
	{
		signatures.at(origin)
	};

	const ed25519::sig sig
	{
		[&origin_sigs, &keyid](auto &buf)
		{
			b64decode(buf, unquote(origin_sigs.at(keyid)));
		}
	};

	return verify(event, pk, sig);
}

bool
ircd::m::verify(const event &event_,
                const ed25519::pk &pk,
                const ed25519::sig &sig)
{
	thread_local char content[64_KiB];
	m::event event
	{
		essential(event_, content)
	};

	thread_local char buf[64_KiB];
	const json::object &preimage
	{
		stringify(buf, event)
	};

	return event::verify(preimage, pk, sig);
}

bool
ircd::m::event::verify(const json::object &event,
                       const ed25519::pk &pk,
                       const ed25519::sig &sig)
{
	//TODO: skip rewrite
	thread_local char buf[64_KiB];
	const string_view preimage
	{
		stringify(buf, event)
	};

	return verify(preimage, pk, sig);
}

bool
ircd::m::event::verify(const string_view &event,
                       const ed25519::pk &pk,
                       const ed25519::sig &sig)
{
	return pk.verify(event, sig);
}

void
ircd::m::event::essential(json::iov &event,
                          const json::iov &contents,
                          const closure_iov_mutable &closure)
{
	const auto &type
	{
		event.at("type")
	};

	if(type == "m.room.aliases")
	{
		const json::iov::push _content{event,
		{
			"content", json::members
			{
				{ "aliases", contents.at("aliases") }
			}
		}};

		closure(event);
	}
	else if(type == "m.room.create")
	{
		const json::iov::push _content{event,
		{
			"content", json::members
			{
				{ "creator", contents.at("creator") }
			}
		}};

		closure(event);
	}
	else if(type == "m.room.history_visibility")
	{
		const json::iov::push _content{event,
		{
			"content", json::members
			{
				{ "history_visibility", contents.at("history_visibility") }
			}
		}};

		closure(event);
	}
	else if(type == "m.room.join_rules")
	{
		const json::iov::push _content{event,
		{
			"content", json::members
			{
				{ "join_rule", contents.at("join_rule") }
			}
		}};

		closure(event);
	}
	else if(type == "m.room.member")
	{
		const json::iov::push _content{event,
		{
			"content", json::members
			{
				{ "membership", contents.at("membership") }
			}
		}};

		closure(event);
	}
	else if(type == "m.room.power_levels")
	{
		const json::iov::push _content{event,
		{
			"content", json::members
			{
				{ "ban", contents.at("ban")                        },
				{ "events", contents.at("events")                  },
				{ "events_default", contents.at("events_default")  },
				{ "kick", contents.at("kick")                      },
				{ "redact", contents.at("redact")                  },
				{ "state_default", contents.at("state_default")    },
				{ "users", contents.at("users")                    },
				{ "users_default", contents.at("users_default")    },
			}
		}};

		closure(event);
	}
	else if(type == "m.room.redaction")
	{
		// This simply finds the redacts key and swaps it with jsundefined for
		// the scope's duration. The redacts key will still be present and
		// visible in the json::iov which is incorrect if directly serialized.
		// However, this iov is turned into a json::tuple (m::event) which ends
		// up being serialized for signing. That serialization is where the
		// jsundefined redacts value is ignored.
		auto &redacts{event.at("redacts")};
		json::value temp(std::move(redacts));
		redacts = json::value{};
		const unwind _{[&redacts, &temp]
		{
			redacts = std::move(temp);
		}};

		const json::iov::push _content
		{
			event, { "content", "{}" }
		};

		closure(event);
	}
	else
	{
		const json::iov::push _content
		{
			event, { "content", "{}" }
		};

		closure(event);
	}
}

ircd::m::event
ircd::m::essential(m::event event,
                   const mutable_buffer &contentbuf)
{
	const auto &type
	{
		json::at<"type"_>(event)
	};

	json::object &content
	{
		json::get<"content"_>(event)
	};

	mutable_buffer essential
	{
		contentbuf
	};

	if(type == "m.room.aliases")
	{
		content = json::stringify(essential, json::members
		{
			{ "aliases", unquote(content.at("aliases")) }
		});
	}
	else if(type == "m.room.create")
	{
		content = json::stringify(essential, json::members
		{
			{ "creator", unquote(content.at("creator")) }
		});
	}
	else if(type == "m.room.history_visibility")
	{
		content = json::stringify(essential, json::members
		{
			{ "history_visibility", unquote(content.at("history_visibility")) }
		});
	}
	else if(type == "m.room.join_rules")
	{
		content = json::stringify(essential, json::members
		{
			{ "join_rule", unquote(content.at("join_rule")) }
		});
	}
	else if(type == "m.room.member")
	{
		content = json::stringify(essential, json::members
		{
			{ "membership", unquote(content.at("membership")) }
		});
	}
	else if(type == "m.room.power_levels")
	{
		content = json::stringify(essential, json::members
		{
			{ "ban", unquote(content.at("ban"))                       },
			{ "events", unquote(content.at("events"))                 },
			{ "events_default", unquote(content.at("events_default")) },
			{ "kick", unquote(content.at("kick"))                     },
			{ "redact", unquote(content.at("redact"))                 },
			{ "state_default", unquote(content.at("state_default"))   },
			{ "users", unquote(content.at("users"))                   },
			{ "users_default", unquote(content.at("users_default"))   },
		});
	}
	else if(type == "m.room.redaction")
	{
		json::get<"redacts"_>(event) = string_view{};
		content = "{}"_sv;
	}
	else
	{
		content = "{}"_sv;
	}

	json::get<"signatures"_>(event) = {};
	return event;
}

//
// event::prev
//

bool
ircd::m::event::prev::prev_events_has(const event::id &event_id)
const
{
	for(const json::array &p : json::get<"prev_events"_>(*this))
		if(unquote(p.at(0)) == event_id)
			return true;

	return false;
}

bool
ircd::m::event::prev::prev_states_has(const event::id &event_id)
const
{
	for(const json::array &p : json::get<"prev_state"_>(*this))
		if(unquote(p.at(0)) == event_id)
			return true;

	return false;
}

bool
ircd::m::event::prev::auth_events_has(const event::id &event_id)
const
{
	for(const json::array &p : json::get<"auth_events"_>(*this))
		if(unquote(p.at(0)) == event_id)
			return true;

	return false;
}

size_t
ircd::m::event::prev::prev_events_count()
const
{
	return json::get<"prev_events"_>(*this).count();
}

size_t
ircd::m::event::prev::prev_states_count()
const
{
	return json::get<"prev_state"_>(*this).count();
}

size_t
ircd::m::event::prev::auth_events_count()
const
{
	return json::get<"auth_events"_>(*this).count();
}

ircd::m::event::id
ircd::m::event::prev::auth_event(const uint &idx)
const
{
	return std::get<0>(auth_events(idx));
}

ircd::m::event::id
ircd::m::event::prev::prev_state(const uint &idx)
const
{
	return std::get<0>(prev_states(idx));
}

ircd::m::event::id
ircd::m::event::prev::prev_event(const uint &idx)
const
{
	return std::get<0>(prev_events(idx));
}

std::tuple<ircd::m::event::id, ircd::string_view>
ircd::m::event::prev::auth_events(const uint &idx)
const
{
	const json::array &auth_event
	{
		at<"auth_events"_>(*this).at(idx)
	};

	return
	{
		unquote(auth_event.at(0)), unquote(auth_event[1])
	};
}

std::tuple<ircd::m::event::id, ircd::string_view>
ircd::m::event::prev::prev_states(const uint &idx)
const
{
	const json::array &state_event
	{
		at<"prev_state"_>(*this).at(idx)
	};

	return
	{
		unquote(state_event.at(0)), unquote(state_event[1])
	};
}

std::tuple<ircd::m::event::id, ircd::string_view>
ircd::m::event::prev::prev_events(const uint &idx)
const
{
	const json::array &prev_event
	{
		at<"prev_events"_>(*this).at(idx)
	};

	return
	{
		unquote(prev_event.at(0)), unquote(prev_event[1])
	};
}

//
// event::fetch
//

decltype(ircd::m::event::fetch::default_opts)
ircd::m::event::fetch::default_opts
{};

void
ircd::m::prefetch(const event::id &event_id,
                  const event::fetch::opts &opts)
{
	prefetch(index(event_id), opts);
}

void
ircd::m::prefetch(const event::id &event_id,
                  const string_view &key)
{
	prefetch(index(event_id), key);
}

void
ircd::m::prefetch(const event::idx &event_idx,
                  const event::fetch::opts &opts)
{
	const vector_view<const string_view> cols
	{
		opts.keys
	};

	for(const auto &col : cols)
		if(col)
			prefetch(event_idx, col);
}

void
ircd::m::prefetch(const event::idx &event_idx,
                  const string_view &key)
{
	const auto &column_idx
	{
		json::indexof<event>(key)
	};

	auto &column
	{
		dbs::event_column.at(column_idx)
	};

	db::prefetch(column, byte_view<string_view>{event_idx});
}

ircd::const_buffer
ircd::m::get(const event::id &event_id,
             const string_view &key,
             const mutable_buffer &out)
{
	const auto &ret
	{
		get(std::nothrow, index(event_id), key, out)
	};

	if(!ret)
		throw m::NOT_FOUND
		{
			"%s for %s not found in database",
			key,
			string_view{event_id}
		};

	return ret;
}

ircd::const_buffer
ircd::m::get(const event::idx &event_idx,
             const string_view &key,
             const mutable_buffer &out)
{
	const const_buffer ret
	{
		get(std::nothrow, event_idx, key, out)
	};

	if(!ret)
		throw m::NOT_FOUND
		{
			"%s for event_idx[%lu] not found in database",
			key,
			event_idx
		};

	return ret;
}

ircd::const_buffer
ircd::m::get(std::nothrow_t,
             const event::id &event_id,
             const string_view &key,
             const mutable_buffer &buf)
{
	return get(std::nothrow, index(event_id), key, buf);
}

ircd::const_buffer
ircd::m::get(std::nothrow_t,
             const event::idx &event_idx,
             const string_view &key,
             const mutable_buffer &buf)
{
	const_buffer ret;
	get(std::nothrow, event_idx, key, [&buf, &ret]
	(const string_view &value)
	{
		ret = { data(buf), copy(buf, value) };
	});

	return ret;
}

void
ircd::m::get(const event::id &event_id,
             const string_view &key,
             const event::fetch::view_closure &closure)
{
	if(!get(std::nothrow, index(event_id), key, closure))
		throw m::NOT_FOUND
		{
			"%s for %s not found in database",
			key,
			string_view{event_id}
		};
}

void
ircd::m::get(const event::idx &event_idx,
             const string_view &key,
             const event::fetch::view_closure &closure)
{
	if(!get(std::nothrow, event_idx, key, closure))
		throw m::NOT_FOUND
		{
			"%s for event_idx[%lu] not found in database",
			key,
			event_idx
		};
}

bool
ircd::m::get(std::nothrow_t,
             const event::id &event_id,
             const string_view &key,
             const event::fetch::view_closure &closure)
{
	return get(std::nothrow, index(event_id), key, closure);
}

bool
ircd::m::get(std::nothrow_t,
             const event::idx &event_idx,
             const string_view &key,
             const event::fetch::view_closure &closure)
{
	const auto &column_idx
	{
		json::indexof<event>(key)
	};

	auto &column
	{
		dbs::event_column.at(column_idx)
	};

	return column(byte_view<string_view>{event_idx}, std::nothrow, closure);
}

void
ircd::m::seek(event::fetch &fetch,
              const event::id &event_id)
{
	if(!seek(fetch, event_id, std::nothrow))
		throw m::NOT_FOUND
		{
			"%s not found in database", event_id
		};
}

bool
ircd::m::seek(event::fetch &fetch,
              const event::id &event_id,
              std::nothrow_t)
{
	const auto &event_idx
	{
		index(event_id, std::nothrow)
	};

	return seek(fetch, event_idx, std::nothrow);
}

void
ircd::m::seek(event::fetch &fetch,
              const event::idx &event_idx)
{
	if(!seek(fetch, event_idx, std::nothrow))
		throw m::NOT_FOUND
		{
			"%lu not found in database", event_idx
		};
}

bool
ircd::m::seek(event::fetch &fetch,
              const event::idx &event_idx,
              std::nothrow_t)
{
	const string_view &key
	{
		byte_view<string_view>(event_idx)
	};

	db::seek(fetch.row, key);
	fetch.valid = fetch.row.valid(key);
	if(!fetch.valid)
		return false;

	auto &event{static_cast<m::event &>(fetch)};
	assign(event, fetch.row, key);
	return true;
}

ircd::m::event::idx
ircd::m::index(const event &event)
try
{
	return index(at<"event_id"_>(event));
}
catch(const json::not_found &)
{
	throw m::NOT_FOUND
	{
		"Cannot find index for event without an event_id."
	};
}

ircd::m::event::idx
ircd::m::index(const event &event,
               std::nothrow_t)
try
{
	return index(at<"event_id"_>(event), std::nothrow);
}
catch(const json::not_found &)
{
	return 0;
}

ircd::m::event::idx
ircd::m::index(const event::id &event_id)
{
	const auto ret
	{
		index(event_id, std::nothrow)
	};

	if(!ret)
		throw m::NOT_FOUND
		{
			"no index found for %s",
			string_view{event_id}

		};

	return ret;
}

ircd::m::event::idx
ircd::m::index(const event::id &event_id,
               std::nothrow_t)
{
	event::idx ret{0};
	index(event_id, std::nothrow, [&ret]
	(const event::idx &event_idx)
	{
		ret = event_idx;
	});

	return ret;
}

bool
ircd::m::index(const event::id &event_id,
               std::nothrow_t,
               const event::closure_idx &closure)
{
	auto &column
	{
		dbs::event_idx
	};

	return column(event_id, std::nothrow, [&closure]
	(const string_view &value)
	{
		const event::idx &event_idx
		{
			byte_view<event::idx>(value)
		};

		closure(event_idx);
	});
}

void
ircd::m::event::fetch::event_id(const idx &idx,
                                const id::closure &closure)
{
	if(!get(std::nothrow, idx, "event_id", closure))
		throw m::NOT_FOUND
		{
			"%lu not found in database", idx
		};
}

bool
ircd::m::event::fetch::event_id(const idx &idx,
                                std::nothrow_t,
                                const id::closure &closure)
{
	return get(std::nothrow, idx, "event_id", closure);
}

/// Seekless constructor.
ircd::m::event::fetch::fetch(const opts *const &opts)
:row
{
	*dbs::events,
	string_view{},
	opts? opts->keys : default_opts.keys,
	cell,
	opts? opts->gopts : default_opts.gopts
}
,valid
{
	false
}
{
}

/// Seek to event_id and populate this event from database.
/// Throws if event not in database.
ircd::m::event::fetch::fetch(const event::id &event_id,
                             const opts *const &opts)
:fetch
{
	index(event_id), opts
}
{
}

/// Seek to event_id and populate this event from database.
/// Event is not populated if not found in database.
ircd::m::event::fetch::fetch(const event::id &event_id,
                             std::nothrow_t,
                             const opts *const &opts)
:fetch
{
	index(event_id, std::nothrow), std::nothrow, opts
}
{
}

/// Seek to event_idx and populate this event from database.
/// Throws if event not in database.
ircd::m::event::fetch::fetch(const event::idx &event_idx,
                             const opts *const &opts)
:fetch
{
	event_idx, std::nothrow, opts
}
{
	if(!valid)
		throw m::NOT_FOUND
		{
			"idx %zu not found in database", event_idx
		};
}

/// Seek to event_idx and populate this event from database.
/// Event is not populated if not found in database.
ircd::m::event::fetch::fetch(const event::idx &event_idx,
                             std::nothrow_t,
                             const opts *const &opts)
:row
{
	*dbs::events,
	byte_view<string_view>{event_idx},
	opts? opts->keys : default_opts.keys,
	cell,
	opts? opts->gopts : default_opts.gopts
}
,valid
{
	row.valid(byte_view<string_view>{event_idx})
}
{
	if(valid)
		assign(*this, row, byte_view<string_view>{event_idx});
}

//
// event::fetch::opts
//

ircd::m::event::fetch::opts::opts(const db::gopts &gopts,
                                  const event::keys::selection &selection)
:opts
{
	selection, gopts
}
{
}

ircd::m::event::fetch::opts::opts(const event::keys::selection &selection,
                                  const db::gopts &gopts)
:opts
{
	event::keys{selection}, gopts
}
{
}

ircd::m::event::fetch::opts::opts(const event::keys &keys,
                                  const db::gopts &gopts)
:keys{keys}
,gopts{gopts}
{
}

//
// event::conforms
//

namespace ircd::m
{
	const size_t event_conforms_num{num_of<event::conforms::code>()};
	extern const std::array<string_view, event_conforms_num> event_conforms_reflects;
}

decltype(ircd::m::event_conforms_reflects)
ircd::m::event_conforms_reflects
{
	"INVALID_OR_MISSING_EVENT_ID",
	"INVALID_OR_MISSING_ROOM_ID",
	"INVALID_OR_MISSING_SENDER_ID",
	"MISSING_TYPE",
	"MISSING_ORIGIN",
	"INVALID_ORIGIN",
	"INVALID_OR_MISSING_REDACTS_ID",
	"MISSING_MEMBERSHIP",
	"INVALID_MEMBERSHIP",
	"MISSING_CONTENT_MEMBERSHIP",
	"INVALID_CONTENT_MEMBERSHIP",
	"MISSING_PREV_EVENTS",
	"MISSING_PREV_STATE",
	"DEPTH_NEGATIVE",
	"DEPTH_ZERO",
	"MISSING_SIGNATURES",
	"MISSING_ORIGIN_SIGNATURE",
	"MISMATCH_ORIGIN_SENDER",
	"MISMATCH_ORIGIN_EVENT_ID",
	"SELF_REDACTS",
	"SELF_PREV_EVENT",
	"SELF_PREV_STATE",
	"DUP_PREV_EVENT",
	"DUP_PREV_STATE",
};

std::ostream &
ircd::m::operator<<(std::ostream &s, const event::conforms &conforms)
{
	thread_local char buf[1024];
	s << conforms.string(buf);
	return s;
}

ircd::string_view
ircd::m::reflect(const event::conforms::code &code)
try
{
	return event_conforms_reflects.at(code);
}
catch(const std::out_of_range &e)
{
	return "??????"_sv;
}

ircd::m::event::conforms::code
ircd::m::event::conforms::reflect(const string_view &name)
{
	const auto it
	{
		std::find(begin(event_conforms_reflects), end(event_conforms_reflects), name)
	};

	if(it == end(event_conforms_reflects))
		throw std::out_of_range
		{
			"There is no event::conforms code by that name."
		};

	return code(std::distance(begin(event_conforms_reflects), it));
}

ircd::m::event::conforms::conforms(const event &e,
                                   const uint64_t &skip)
:conforms{e}
{
	report &= ~skip;
}

ircd::m::event::conforms::conforms(const event &e)
:report{0}
{
	if(!valid(m::id::EVENT, json::get<"event_id"_>(e)))
		set(INVALID_OR_MISSING_EVENT_ID);

	if(!valid(m::id::ROOM, json::get<"room_id"_>(e)))
		set(INVALID_OR_MISSING_ROOM_ID);

	if(!valid(m::id::USER, json::get<"sender"_>(e)))
		set(INVALID_OR_MISSING_SENDER_ID);

	if(empty(json::get<"type"_>(e)))
		set(MISSING_TYPE);

	if(empty(json::get<"origin"_>(e)))
		set(MISSING_ORIGIN);

	//TODO: XXX
	if(false)
		set(INVALID_ORIGIN);

	if(empty(json::get<"signatures"_>(e)))
		set(MISSING_SIGNATURES);

	if(empty(json::object{json::get<"signatures"_>(e).get(json::get<"origin"_>(e))}))
		set(MISSING_ORIGIN_SIGNATURE);

	if(!has(INVALID_OR_MISSING_SENDER_ID))
		if(json::get<"origin"_>(e) != m::id::user{json::get<"sender"_>(e)}.host())
			set(MISMATCH_ORIGIN_SENDER);

	if(!has(INVALID_OR_MISSING_EVENT_ID))
		if(json::get<"origin"_>(e) != m::id::event{json::get<"event_id"_>(e)}.host())
			set(MISMATCH_ORIGIN_EVENT_ID);

	if(json::get<"type"_>(e) == "m.room.redaction")
		if(!valid(m::id::EVENT, json::get<"redacts"_>(e)))
			set(INVALID_OR_MISSING_REDACTS_ID);

	if(json::get<"redacts"_>(e))
		if(json::get<"redacts"_>(e) == json::get<"event_id"_>(e))
			set(SELF_REDACTS);

	if(json::get<"type"_>(e) == "m.room.member")
		if(empty(json::get<"membership"_>(e)))
			set(MISSING_MEMBERSHIP);

	if(json::get<"type"_>(e) == "m.room.member")
		if(!all_of<std::islower>(json::get<"membership"_>(e)))
			set(INVALID_MEMBERSHIP);

	if(json::get<"type"_>(e) == "m.room.member")
		if(empty(unquote(json::get<"content"_>(e).get("membership"))))
			set(MISSING_CONTENT_MEMBERSHIP);

	if(json::get<"type"_>(e) == "m.room.member")
		if(!all_of<std::islower>(unquote(json::get<"content"_>(e).get("membership"))))
			set(INVALID_CONTENT_MEMBERSHIP);

	if(json::get<"type"_>(e) != "m.room.create")
		if(empty(json::get<"prev_events"_>(e)))
			set(MISSING_PREV_EVENTS);

	/*
	if(json::get<"type"_>(e) != "m.room.create")
		if(!empty(json::get<"state_key"_>(e)))
			if(empty(json::get<"prev_state"_>(e)))
				set(MISSING_PREV_STATE);
	*/

	if(json::get<"depth"_>(e) != json::undefined_number && json::get<"depth"_>(e) < 0)
		set(DEPTH_NEGATIVE);

	if(json::get<"type"_>(e) != "m.room.create")
		if(json::get<"depth"_>(e) == 0)
			set(DEPTH_ZERO);

	const prev p{e};
	size_t i{0}, j{0};
	for(const json::array &pe : json::get<"prev_events"_>(p))
	{
		if(unquote(pe.at(0)) == json::get<"event_id"_>(e))
			set(SELF_PREV_EVENT);

		j = 0;
		for(const json::array &pe_ : json::get<"prev_events"_>(p))
			if(i != j++)
				if(pe_.at(0) == pe.at(0))
					set(DUP_PREV_EVENT);

		++i;
	}

	i = 0;
	for(const json::array &ps : json::get<"prev_state"_>(p))
	{
		if(unquote(ps.at(0)) == json::get<"event_id"_>(e))
			set(SELF_PREV_STATE);

		j = 0;
		for(const json::array &ps_ : json::get<"prev_state"_>(p))
			if(i != j++)
				if(ps_.at(0) == ps.at(0))
					set(DUP_PREV_STATE);

		++i;
	}
}

void
ircd::m::event::conforms::operator|=(const code &code)
&
{
	set(code);
}

void
ircd::m::event::conforms::del(const code &code)
{
	report &= ~(1UL << code);
}

void
ircd::m::event::conforms::set(const code &code)
{
	report |= (1UL << code);
}

ircd::string_view
ircd::m::event::conforms::string(const mutable_buffer &out)
const
{
	mutable_buffer buf{out};
	for(uint64_t i(0); i < num_of<code>(); ++i)
	{
		if(!has(code(i)))
			continue;

		if(begin(buf) != begin(out))
			consume(buf, copy(buf, " "_sv));

		consume(buf, copy(buf, m::reflect(code(i))));
	}

	return { data(out), begin(buf) };
}

bool
ircd::m::event::conforms::has(const code &code)
const
{
	return report & (1UL << code);
}

bool
ircd::m::event::conforms::has(const uint &code)
const
{
	return (report & (1UL << code)) == code;
}

bool
ircd::m::event::conforms::operator!()
const
{
	return clean();
}

ircd::m::event::conforms::operator bool()
const
{
	return !clean();
}

bool
ircd::m::event::conforms::clean()
const
{
	return report == 0;
}
