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
#define HAVE_IRCD_M_USERS_H

namespace ircd::m::users
{
	bool for_each(const string_view &id_lower_bound, const user::closure_bool &);
	bool for_each(const user::closure_bool &);
	void for_each(const user::closure &);
}
