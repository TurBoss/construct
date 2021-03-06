// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

namespace construct
{
	struct signals;
	struct console;

	extern struct console *console;
}

struct construct::signals
{
	std::unique_ptr<boost::asio::signal_set> signal_set;
	ircd::runlevel_changed runlevel_changed;

	void set_handle();
	void on_signal(const boost::system::error_code &, int) noexcept;
	void on_runlevel(const enum ircd::runlevel &);

  public:
	signals(boost::asio::io_context &ios);
};

struct construct::console
{
	static ircd::conf::item<size_t> stack_sz;
	static ircd::conf::item<size_t> input_max;
	static ircd::conf::item<size_t> ratelimit_bytes;
	static ircd::conf::item<ircd::milliseconds> ratelimit_sleep;

	static const ircd::string_view generic_message;
	static const ircd::string_view console_message;
	static std::once_flag seen_message;
	static std::deque<std::string> queue;

	std::string line;
	std::string record_path;
	ircd::module *module {nullptr};
	ircd::context context;
	ircd::runlevel_changed runlevel_changed;

	void show_message() const;
	void on_runlevel(const enum ircd::runlevel &);
	bool wait_running() const;
	bool next_command();
	void wait_input();

	bool cmd__record();
	int handle_line_bymodule();
	bool handle_line();
	void main();

	console();

	static bool active();
	static bool interrupt();
	static bool terminate();
	static bool execute(std::string cmd);
	static bool spawn();
};
