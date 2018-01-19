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
#define HAVE_IRCD_DB_DATABASE_COLUMN_H

// This file is not part of the standard include stack because it requires
// RocksDB symbols which we cannot forward declare. It is used internally
// and does not need to be included by general users of IRCd.

struct ircd::db::database::column final
:std::enable_shared_from_this<database::column>
,rocksdb::ColumnFamilyDescriptor
{
	database *d;
	std::type_index key_type;
	std::type_index mapped_type;
	database::descriptor descriptor;
	comparator cmp;
	prefix_transform prefix;
	custom_ptr<rocksdb::ColumnFamilyHandle> handle;

  public:
	operator const rocksdb::ColumnFamilyOptions &();
	operator const rocksdb::ColumnFamilyHandle *() const;
	operator const database &() const;

	operator rocksdb::ColumnFamilyOptions &();
	operator rocksdb::ColumnFamilyHandle *();
	operator database &();

	explicit column(database *const &d, const database::descriptor &);
	column() = delete;
	column(column &&) = delete;
	column(const column &) = delete;
	column &operator=(column &&) = delete;
	column &operator=(const column &) = delete;
	~column() noexcept;

	friend void flush(column &, const bool &blocking);
};