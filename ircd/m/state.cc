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


/// Convenience to make a key and then get a value
void
ircd::m::state::get(const string_view &root,
                    const string_view &type,
                    const string_view &state_key,
                    const val_closure &closure)
{
	if(!get(std::nothrow, root, type, state_key, closure))
		throw m::NOT_FOUND
		{
			"type='%s' state_key='%s' not found in tree %s",
			type,
			state_key,
			root
		};
}

/// Convenience to make a key and then get a value (doesn't throw NOT_FOUND)
bool
ircd::m::state::get(std::nothrow_t,
                    const string_view &root,
                    const string_view &type,
                    const string_view &state_key,
                    const val_closure &closure)
{
	char key[KEY_MAX_SZ];
	return get(std::nothrow, root, make_key(key, type, state_key), closure);
}

/// throws m::NOT_FOUND if the exact key and its value does not exist.
void
ircd::m::state::get(const string_view &root,
                    const json::array &key,
                    const val_closure &closure)
{
	if(!get(std::nothrow, root, key, closure))
		throw m::NOT_FOUND
		{
			"%s not found in tree %s",
			string_view{key},
			root
		};
}

/// Recursive query to find the leaf value for the given key, starting from
/// the given root node ID. Value can be viewed in the closure. Returns false
/// if the exact key and its value does not exist in the tree; no node ID's
/// are ever returned here.
bool
ircd::m::state::get(std::nothrow_t,
                    const string_view &root,
                    const json::array &key,
                    const val_closure &closure)
{
	bool ret{false};
	char nextbuf[ID_MAX_SZ];
	string_view nextid{root};
	while(nextid) get_node(nextid, [&](const node &node)
	{
		auto pos(node.find(key));
		if(pos < node.keys() && node.key(pos) == key)
		{
			ret = true;
			nextid = {};
			closure(node.val(pos));
			return;
		}

		const auto c(node.childs());
		if(c && pos >= c)
			pos = c - 1;

		if(node.has_child(pos))
			nextid = { nextbuf, strlcpy(nextbuf, node.child(pos)) };
		else
			nextid = {};
	});

	return ret;
}

size_t
ircd::m::state::count(const string_view &root)
{
	return count(root, []
	(const json::array &key, const string_view &val)
	{
		return true;
	});
}

size_t
ircd::m::state::count(const string_view &root,
                      const iter_bool_closure &closure)
{
	size_t ret{0};
	for_each(root, [&ret, &closure]
	(const json::array &key, const string_view &val)
	{
		ret += closure(key, val);
	});

	return ret;
}

void
ircd::m::state::for_each(const string_view &root,
                         const iter_closure &closure)
{
	test(root, [&closure]
	(const json::array &key, const string_view &val)
	{
		closure(key, val);
		return false;
	});
}

void
ircd::m::state::for_each(const string_view &root,
                         const string_view &type,
                         const iter_closure &closure)
{
	test(root, type, [&closure]
	(const json::array &key, const string_view &val)
	{
		closure(key, val);
		return false;
	});
}

bool
ircd::m::state::test(const string_view &root,
                     const iter_bool_closure &closure)
{
	return dfs(root, [&closure]
	(const json::array &key, const string_view &val, const uint &, const uint &)
	{
		return closure(key, val);
	});
}

bool
ircd::m::state::test(const string_view &root,
                     const string_view &type,
                     const iter_bool_closure &closure)
{
	char buf[KEY_MAX_SZ];
	const json::array key
	{
		make_key(buf, type)
	};

	return dfs(root, key, [&closure]
	(const json::array &key, const string_view &val, const uint &, const uint &)
	{
		return closure(key, val);
	});
}

namespace ircd::m::state
{
	bool _dfs_recurse(const search_closure &, const node &, const json::array &key, int &);
}

bool
ircd::m::state::dfs(const string_view &root,
                    const search_closure &closure)
{
	return dfs(root, json::array{}, closure);
}

bool
ircd::m::state::dfs(const string_view &root,
                    const json::array &key,
                    const search_closure &closure)
{
	bool ret{true};
	get_node(root, [&closure, &key, &ret]
	(const auto &node)
	{
		int depth(-1);
		ret = _dfs_recurse(closure, node, key, depth);
	});

	return ret;
}

bool
ircd::m::state::_dfs_recurse(const search_closure &closure,
                             const node &node,
                             const json::array &key,
                             int &depth)
{
	++depth;
	const unwind down{[&depth]
	{
		--depth;
	}};

	const node::rep rep{node};
	const auto kpos{rep.find(key)};
	for(uint pos(kpos); pos < rep.kn || pos < rep.cn; ++pos)
	{
		if(!empty(rep.chld[pos]))
		{
			bool ret{false};
			get_node(rep.chld[pos], [&closure, &key, &depth, &ret]
			(const auto &node)
			{
				ret = _dfs_recurse(closure, node, key, depth);
			});

			if(ret)
				return true;
		}

		if(rep.kn <= pos)
			continue;

		if(!empty(key) && !prefix_eq(key, rep.keys[pos]))
			break;

		if(closure(rep.keys[pos], rep.vals[pos], depth, pos))
			return true;
	}

	return false;
}

// Internal insertion operations
namespace ircd::m::state
{
	static mutable_buffer _getbuffer(const uint8_t &height);
	static string_view _insert_overwrite(db::txn &, const json::array &key, const string_view &val, const mutable_buffer &idbuf, node::rep &, const size_t &pos);
	static string_view _insert_leaf_nonfull(db::txn &, const json::array &key, const string_view &val, const mutable_buffer &idbuf, node::rep &, const size_t &pos);
	static json::object _insert_leaf_full(const int8_t &height, db::txn &, const json::array &key, const string_view &val, node::rep &, const size_t &pos, node::rep &push);
	static string_view _insert_branch_nonfull(db::txn &, const mutable_buffer &idbuf, node::rep &, const size_t &pos, node::rep &pushed);
	static json::object _insert_branch_full(const int8_t &height, db::txn &, node::rep &, const size_t &pos, node::rep &push, const node::rep &pushed);
	static string_view _insert(int8_t &height, db::txn &, const json::array &key, const string_view &val, const node &node, const mutable_buffer &idbuf, node::rep &push);
	static string_view _create(db::txn &, const mutable_buffer &root, const string_view &type, const string_view &state_key, const string_view &val);
}

/// State update from an event. Leaves the root node ID in the root buffer;
/// returns view.
///
ircd::m::state::id
ircd::m::state::insert(db::txn &txn,
                       const mutable_buffer &rootout,
                       const string_view &rootin,
                       const event &event)
{
	const auto &type{at<"type"_>(event)};
	const auto &state_key{at<"state_key"_>(event)};
	const auto &event_id{at<"event_id"_>(event)};

	if(type == "m.room.create")
	{
		assert(empty(rootin));
		return _create(txn, rootout, type, state_key, event_id);
	}

	assert(!empty(rootin));
	return insert(txn, rootout, rootin, type, state_key, event_id);
}

ircd::m::state::id
ircd::m::state::_create(db::txn &txn,
                        const mutable_buffer &root,
                        const string_view &type,
                        const string_view &state_key,
                        const string_view &val)
{
	assert(type == "m.room.create");
	assert(defined(state_key));

	// Because this is a new tree and nothing is read from the DB, all
	// writes here are just copies into the txn and these buffers can
	// remain off-stack.
	const critical_assertion ca;
	thread_local char key[KEY_MAX_SZ];
	thread_local char node[NODE_MAX_SZ];

	node::rep rep;
	rep.keys[0] = make_key(key, type, state_key);
	rep.kn = 1;
	rep.vals[0] = val;
	rep.vn = 1;
	rep.chld[0] = string_view{};
	rep.cn = 1;

	return set_node(txn, root, rep.write(node));
}

/// State update for room_id inserting (type,state_key) = event_id into the
/// tree. Leaves the root node ID in the root buffer; returns view.
ircd::m::state::id
ircd::m::state::insert(db::txn &txn,
                       const mutable_buffer &rootout,
                       const string_view &rootin,
                       const string_view &type,
                       const string_view &state_key,
                       const m::id::event &event_id)
{
	// The insertion process reads from the DB and will yield this ircd::ctx
	// so the key buffer must stay on this stack.
	char key[KEY_MAX_SZ];
	return insert(txn, rootout, rootin, make_key(key, type, state_key), event_id);
}

ircd::m::state::id
ircd::m::state::insert(db::txn &txn,
                       const mutable_buffer &rootout,
                       const string_view &rootin,
                       const json::array &key,
                       const m::id::event &event_id)
{
	node::rep push;
	int8_t height{0};
	string_view root{rootin};
	get_node(root, [&](const node &node)
	{
		root = _insert(height, txn, key, event_id, node, rootout, push);
	});

	if(push.kn)
		root = push.write(txn, rootout);

	return root;
}

ircd::m::state::id
ircd::m::state::_insert(int8_t &height,
                        db::txn &txn,
                        const json::array &key,
                        const string_view &val,
                        const node &node,
                        const mutable_buffer &idbuf,
                        node::rep &push)
{
	// Recursion metrics
	const unwind down{[&height]{ --height; }};
	if(unlikely(++height >= MAX_HEIGHT))
		throw assertive{"recursion limit exceeded"};

	// This function assumes that any node argument is a previously "existing"
	// node which means it contains at least one key/value.
	assert(node.keys() > 0);
	assert(node.keys() == node.vals());

	node::rep rep{node};
	const auto pos{node.find(key)};

	if(keycmp(node.key(pos), key) == 0)
		return _insert_overwrite(txn, key, val, idbuf, rep, pos);

	if(node.childs() == 0 && rep.full())
		return _insert_leaf_full(height, txn, key, val, rep, pos, push);

	if(node.childs() == 0 && !rep.full())
		return _insert_leaf_nonfull(txn, key, val, idbuf, rep, pos);

	if(empty(node.child(pos)))
		return _insert_leaf_nonfull(txn, key, val, idbuf, rep, pos);

	// These collect data from the next level.
	node::rep pushed;
	string_view child;

	// Recurse
	get_node(node.child(pos), [&](const auto &node)
	{
		child = _insert(height, txn, key, val, node, idbuf, pushed);
	});

	// Child was pushed but that will stop here.
	if(pushed.kn && !rep.full())
		return _insert_branch_nonfull(txn, idbuf, rep, pos, pushed);

	// Most complex branch
	if(pushed.kn && rep.full())
		return _insert_branch_full(height, txn, rep, pos, push, pushed);

	// Indicates no push, and the child value is just an ID of a node.
	rep.chld[pos] = child;
	return rep.write(txn, idbuf);
}

ircd::json::object
ircd::m::state::_insert_branch_full(const int8_t &height,
                                    db::txn &txn,
                                    node::rep &rep,
                                    const size_t &pos,
                                    node::rep &push,
                                    const node::rep &pushed)
{
	rep.shr(pos);

	rep.keys[pos] = pushed.keys[0];
	++rep.kn;

	rep.vals[pos] = pushed.vals[0];
	++rep.vn;

	rep.chld[pos] = pushed.chld[0];
	rep.chld[pos + 1] = pushed.chld[1];
	++rep.cn;

	size_t i(0);
	node::rep left;
	for(; i < rep.kn / 2; ++i)
	{
		left.keys[left.kn++] = rep.keys[i];
		left.vals[left.vn++] = rep.vals[i];
		left.chld[left.cn++] = rep.chld[i];
	}
	left.chld[left.cn++] = rep.chld[i];

	push.keys[push.kn++] = rep.keys[i];
	push.vals[push.vn++] = rep.vals[i];

	node::rep right;
	for(++i; i < rep.kn; ++i)
	{
		right.keys[right.kn++] = rep.keys[i];
		right.vals[right.vn++] = rep.vals[i];
		right.chld[right.cn++] = rep.chld[i];
	}
	right.chld[right.cn++] = rep.chld[i];

	thread_local char lc[ID_MAX_SZ], rc[ID_MAX_SZ];
	push.chld[push.cn++] = left.write(txn, lc);
	push.chld[push.cn++] = right.write(txn, rc);

	const auto ret
	{
		push.write(_getbuffer(height))
	};

	// Courtesy reassignment of all the references in `push` after rewrite.
	push = state::node{ret};
	return ret;
}

ircd::json::object
ircd::m::state::_insert_leaf_full(const int8_t &height,
                                  db::txn &txn,
                                  const json::array &key,
                                  const string_view &val,
                                  node::rep &rep,
                                  const size_t &pos,
                                  node::rep &push)
{
	rep.shr(pos);

	rep.keys[pos] = key;
	++rep.kn;

	rep.vals[pos] = val;
	++rep.vn;

	size_t i(0);
	node::rep left;
	for(; i < rep.kn / 2; ++i)
	{
		left.keys[left.kn++] = rep.keys[i];
		left.vals[left.vn++] = rep.vals[i];
		left.chld[left.cn++] = string_view{};
	}

	push.keys[push.kn++] = rep.keys[i];
	push.vals[push.vn++] = rep.vals[i];

	node::rep right;
	for(++i; i < rep.kn; ++i)
	{
		right.keys[right.kn++] = rep.keys[i];
		right.vals[right.vn++] = rep.vals[i];
		right.chld[right.cn++] = string_view{};
	}

	thread_local char lc[ID_MAX_SZ], rc[ID_MAX_SZ];
	push.chld[push.cn++] = left.write(txn, lc);
	push.chld[push.cn++] = right.write(txn, rc);

	const auto ret
	{
		push.write(_getbuffer(height))
	};

	// Courtesy reassignment of all the references in `push` after rewrite.
	push = state::node{ret};
	return ret;
}

ircd::m::state::id
ircd::m::state::_insert_branch_nonfull(db::txn &txn,
                                       const mutable_buffer &idbuf,
                                       node::rep &rep,
                                       const size_t &pos,
                                       node::rep &pushed)
{
	rep.shr(pos);

	rep.keys[pos] = pushed.keys[0];
	++rep.kn;

	rep.vals[pos] = pushed.vals[0];
	++rep.vn;

	rep.chld[pos] = pushed.chld[0];
	rep.chld[pos + 1] = pushed.chld[1];
	++rep.cn;

	return rep.write(txn, idbuf);
}

ircd::m::state::id
ircd::m::state::_insert_leaf_nonfull(db::txn &txn,
                                     const json::array &key,
                                     const string_view &val,
                                     const mutable_buffer &idbuf,
                                     node::rep &rep,
                                     const size_t &pos)
{
	rep.shr(pos);

	rep.keys[pos] = key;
	++rep.kn;

	rep.vals[pos] = val;
	++rep.vn;

	rep.chld[pos] = string_view{};
	++rep.cn;

	return rep.write(txn, idbuf);
}

ircd::m::state::id
ircd::m::state::_insert_overwrite(db::txn &txn,
                                  const json::array &key,
                                  const string_view &val,
                                  const mutable_buffer &idbuf,
                                  node::rep &rep,
                                  const size_t &pos)
{
	rep.keys[pos] = key;
	rep.vals[pos] = val;

	return rep.write(txn, idbuf);
}

/// This function returns a thread_local buffer intended for writing temporary
/// nodes which may be "pushed" down the tree during the btree insertion
/// process. This is an alternative to allocating such space in each stack
/// frame when only one or two are ever used at a time -- but because more than
/// one may be used at a time during complex rebalances we have the user pass
/// their current recursion depth which is used to partition the buffer so they
/// don't overwrite their own data.
ircd::mutable_buffer
ircd::m::state::_getbuffer(const uint8_t &height)
{
	static const size_t buffers{2};
	using buffer_type = std::array<char, NODE_MAX_SZ>;
	thread_local std::array<buffer_type, buffers> buffer;
	return buffer.at(height % buffer.size());
}

/// View a node by ID. This makes a DB query and may yield ircd::ctx.
void
ircd::m::state::get_node(const string_view &node_id,
                         const node_closure &closure)
{
	assert(bool(dbs::state_node));
	auto &column{dbs::state_node};
	column(node_id, closure);
}

/// Writes a node to the db::txn and returns the id of this node (a hash) into
/// the buffer.
ircd::m::state::id
ircd::m::state::set_node(db::txn &iov,
                         const mutable_buffer &hashbuf,
                         const json::object &node)
{
	const sha256::buf hash
	{
		sha256{node}
	};

	const auto hashb64
	{
		b64encode_unpadded(hashbuf, hash)
	};

	db::txn::append
	{
		iov, dbs::state_node,
		{
			db::op::SET,
			hashb64,       // key
			node,          // val
		}
	};

	return hashb64;
}

/// Creates a key array from the most common key pattern of a matrix
/// room (type,state_key).
ircd::json::array
ircd::m::state::make_key(const mutable_buffer &out,
                         const string_view &type,
                         const string_view &state_key)
{
	const json::value key_parts[]
	{
		type, state_key
	};

	const json::value key
	{
		key_parts, 2
	};

	return { data(out), json::print(out, key) };
}

ircd::json::array
ircd::m::state::make_key(const mutable_buffer &out,
                         const string_view &type)
{
	const json::value key_parts[]
	{
		type
	};

	const json::value key
	{
		key_parts, 1
	};

	return { data(out), json::print(out, key) };
}

bool
ircd::m::state::prefix_eq(const json::array &a,
                          const json::array &b)
{
	ushort i(0);
	auto ait(begin(a));
	auto bit(begin(b));
	for(; ait != end(a) && bit != end(b) && i < 2; ++ait, ++bit)
	{
		assert(surrounds(*ait, '"'));
		assert(surrounds(*bit, '"'));

		if(*ait == *bit)
		{
			if(i)
				return false;
		}
		else ++i;
	}

	return ait != end(a) || bit != end(b)? i == 0 : i < 2;
}

/// Compares two keys. Keys are arrays of strings which become safely
/// concatenated for a linear lexical comparison. Returns -1 if a less
/// than b; 0 if equal; 1 if a greater than b.
int
ircd::m::state::keycmp(const json::array &a,
                       const json::array &b)
{
	auto ait(begin(a));
	auto bit(begin(b));
	for(; ait != end(a) && bit != end(b); ++ait, ++bit)
	{
		assert(surrounds(*ait, '"'));
		assert(surrounds(*bit, '"'));

		if(*ait < *bit)
			return -1;

		if(*bit < *ait)
			return 1;
	}

	assert(ait == end(a) || bit == end(b));
	return ait == end(a) && bit != end(b)?  -1:
	       ait == end(a) && bit == end(b)?   0:
	                                         1;
}

//
// rep
//

ircd::m::state::node::rep::rep(const node &node)
:kn{node.keys(keys.data(), keys.size())}
,vn{node.vals(vals.data(), vals.size())}
,cn{node.childs(chld.data(), chld.size())}
{
}

ircd::m::state::id
ircd::m::state::node::rep::write(db::txn &txn,
                                 const mutable_buffer &idbuf)
{
	thread_local char buf[NODE_MAX_SZ];
	return set_node(txn, idbuf, write(buf));
}

ircd::json::object
ircd::m::state::node::rep::write(const mutable_buffer &out)
{
	assert(kn == vn);
	assert(cn <= kn + 1);
	assert(!childs() || childs() > kn);
	assert(!duplicates());

	assert(kn > 0 && vn > 0);
	assert(kn <= NODE_MAX_KEY);
	assert(vn <= NODE_MAX_VAL);
	assert(cn <= NODE_MAX_DEG);

	json::value keys[kn];
	{
		for(size_t i(0); i < kn; ++i)
			keys[i] = this->keys[i];
	}

	json::value vals[vn];
	{
		for(size_t i(0); i < vn; ++i)
			vals[i] = this->vals[i];
	};

	json::value chld[cn];
	{
		for(size_t i(0); i < cn; ++i)
			chld[i] = this->chld[i];
	};

	json::iov iov;
	const json::iov::push push[]
	{
		{ iov, { "k"_sv, { keys, kn } } },
		{ iov, { "v"_sv, { vals, vn } } },
		{ iov, { "c"_sv, { chld, cn } } },
	};

	return { data(out), json::print(out, iov) };
}

/// Shift right.
void
ircd::m::state::node::rep::shr(const size_t &pos)
{
	std::copy_backward(begin(keys) + pos, begin(keys) + kn, begin(keys) + kn + 1);
	std::copy_backward(begin(vals) + pos, begin(vals) + vn, begin(vals) + vn + 1);
	std::copy_backward(begin(chld) + pos, begin(chld) + cn, begin(chld) + cn + 1);
}

size_t
ircd::m::state::node::rep::find(const json::array &parts)
const
{
	size_t i{0};
	for(; i < kn; ++i)
		if(keycmp(parts, keys[i]) <= 0)
			return i;

	return i;
}

size_t
ircd::m::state::node::rep::childs()
const
{
	size_t ret(0);
	for(size_t i(0); i < cn; ++i)
		if(!empty(unquote(chld[i])))
			++ret;

	return ret;
}

bool
ircd::m::state::node::rep::duplicates()
const
{
	for(size_t i(0); i < kn; ++i)
		for(size_t j(0); j < kn; ++j)
			if(j != i && keys[i] == keys[j])
				return true;

	for(size_t i(0); i < cn; ++i)
		if(!empty(unquote(chld[i])))
			for(size_t j(0); j < cn; ++j)
				if(j != i && chld[i] == chld[j])
					return true;

	return false;
}

bool
ircd::m::state::node::rep::overfull()
const
{
	assert(kn == vn);
	return kn > NODE_MAX_KEY;
}

bool
ircd::m::state::node::rep::full()
const
{
	assert(kn == vn);
	return kn >= NODE_MAX_KEY;
}

//
// node
//

// Count values that actually lead to other nodes
bool
ircd::m::state::node::has_child(const size_t &pos)
const
{
	return !empty(child(pos));
}

// Count values that actually lead to other nodes
bool
ircd::m::state::node::has_key(const json::array &key)
const
{
	const auto pos(find(key));
	if(pos >= keys())
		return false;

	return keycmp(this->key(pos), key) == 0;
}

/// Find position for a val in node. Uses the keycmp(). If there is one
/// key in node, and the argument compares less than or equal to the key,
/// 0 is returned, otherwise 1 is returned. If there are two keys in node
/// and argument compares less than both, 0 is returned; equal to key[0],
/// 0 is returned; greater than key[0] and less than or equal to key[1],
/// 1 is returned; greater than both: 2 is returned. Note that there can
/// be one more childs() than keys() in a node (this is usually a "full
/// node") but there might not be, and the returned pos might be out of
/// range.
size_t
ircd::m::state::node::find(const json::array &parts)
const
{
	size_t ret{0};
	for(const json::array key : json::get<"k"_>(*this))
		if(keycmp(parts, key) <= 0)
			return ret;
		else
			++ret;

	return ret;
}

size_t
ircd::m::state::node::childs(state::id *const &out,
                             const size_t &max)
const
{
	size_t i(0);
	for(const string_view &c : json::get<"c"_>(*this))
		if(likely(i < max))
			out[i++] = unquote(c);

	return i;
}

size_t
ircd::m::state::node::vals(string_view *const &out,
                           const size_t &max)
const
{
	size_t i(0);
	for(const string_view &v : json::get<"v"_>(*this))
		if(likely(i < max))
			out[i++] = unquote(v);

	return i;
}

size_t
ircd::m::state::node::keys(json::array *const &out,
                           const size_t &max)
const
{
	size_t i(0);
	for(const json::array &k : json::get<"k"_>(*this))
		if(likely(i < max))
			out[i++] = k;

	return i;
}

ircd::m::state::id
ircd::m::state::node::child(const size_t &pos)
const
{
	const json::array &children
	{
		json::get<"c"_>(*this, json::empty_array)
	};

	return unquote(children[pos]);
}

// Get value at position pos (throws out_of_range)
ircd::string_view
ircd::m::state::node::val(const size_t &pos)
const
{
	const json::array &values
	{
		json::get<"v"_>(*this, json::empty_array)
	};

	return unquote(values[pos]);
}

// Get key at position pos (throws out_of_range)
ircd::json::array
ircd::m::state::node::key(const size_t &pos)
const
{
	const json::array &keys
	{
		json::get<"k"_>(*this, json::empty_array)
	};

	return keys[pos];
}

// Count children in node
size_t
ircd::m::state::node::childs()
const
{
	size_t ret(0);
	for(const auto &c : json::get<"c"_>(*this))
		ret += !empty(c) && c != json::empty_string;

	return ret;
}

// Count values in node
size_t
ircd::m::state::node::vals()
const
{
	return json::get<"v"_>(*this).count();
}

/// Count keys in node
size_t
ircd::m::state::node::keys()
const
{
	return json::get<"k"_>(*this).count();
}