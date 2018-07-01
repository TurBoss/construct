// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

static_assert(sizeof(ircd::cbor::item::header) == 1);
static_assert(std::is_standard_layout<ircd::cbor::item::header>());

namespace ircd::cbor
{

}