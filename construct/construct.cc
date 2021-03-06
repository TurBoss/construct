// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#include <ircd/ircd.h>
#include <RB_INC_SYS_RESOURCE_H
#include <ircd/asio.h>
#include "lgetopt.h"
#include "construct.h"

static bool startup_checks();
static void applyargs();
static void enable_coredumps();
static int print_version();

const char *const fatalerrstr
{R"(
***
*** A fatal error has occurred. Please contact the developer with the message below.
*** Create a coredump by reproducing the error using the -debug command-line option.
***

%s
)"};

const char *const usererrstr
{R"(
***
*** A fatal startup error has occurred. Please fix the problem to continue. ***
***

%s
)"};

bool printversion;
bool cmdline;
bool quietmode;
bool debugmode;
bool nolisten;
bool noautomod;
bool checkdb;
bool pitrecdb;
bool nojs;
bool nodirect;
bool noaio;
const char *configfile;
const char *execute;
lgetopt opts[] =
{
	{ "help",       nullptr,        lgetopt::USAGE,   "Print this text" },
	{ "version",    &printversion,  lgetopt::BOOL,    "Print version and exit" },
	{ "configfile", &configfile,    lgetopt::STRING,  "File to use for ircd.conf" },
	{ "debug",      &debugmode,     lgetopt::BOOL,    "Enable options for debugging" },
	{ "quiet",      &quietmode,     lgetopt::BOOL,    "Suppress log messages at the terminal." },
	{ "console",    &cmdline,       lgetopt::BOOL,    "Drop to a command line immediately after startup" },
	{ "execute",    &execute,       lgetopt::STRING,  "Execute command lines immediately after startup" },
	{ "nolisten",   &nolisten,      lgetopt::BOOL,    "Normal execution but without listening sockets" },
	{ "noautomod",  &noautomod,     lgetopt::BOOL,    "Normal execution but without autoloading modules" },
	{ "checkdb",    &checkdb,       lgetopt::BOOL,    "Perform complete checks of databases when opening" },
	{ "pitrecdb",   &pitrecdb,      lgetopt::BOOL,    "Allow Point-In-Time-Recover if DB reports corruption after crash" },
	{ "nojs",       &nojs,          lgetopt::BOOL,    "Disable SpiderMonkey JS subsystem from initializing. (noop when not available)." },
	{ "nodirect",   &nodirect,      lgetopt::BOOL,    "Disable direct IO (O_DIRECT) for unsupporting filesystems." },
	{ "noaio",      &noaio,         lgetopt::BOOL,    "Disable the AIO interface in favor of traditional syscalls. " },
	{ nullptr,      nullptr,        lgetopt::STRING,  nullptr },
};

std::unique_ptr<boost::asio::io_context> ios
{
	// Having trouble with static dtor in clang so this has tp be dynamic
	std::make_unique<boost::asio::io_context>()
};

int main(int argc, char *const *argv)
try
{
	umask(077);       // better safe than sorry --SRB

	// '-' switched arguments come first; this function incs argv and decs argc
	parseargs(&argc, &argv, opts);
	applyargs();

	// cores are not dumped without consent of the user to maintain the privacy
	// of cryptographic key material in memory at the time of the crash.
	if(RB_DEBUG_LEVEL || ircd::debugmode)
		enable_coredumps();

	if(!startup_checks())
		return EXIT_FAILURE;

	if(printversion)
		return print_version();

	if(quietmode)
		ircd::log::console_disable();

	// The matrix origin is the first positional argument after any switched
	// arguments. The matrix origin is the hostpart of MXID's for the server.
	const ircd::string_view origin
	{
		argc > 0?
			argv[0]:
			nullptr
	};

	// The hostname is the unique name for this specific server. This is
	// generally the same as origin; but if origin is example.org with an
	// SRV record redirecting to matrix.example.org then hostname is
	// matrix.example.org. In clusters serving a single origin, all
	// hostnames must be different.
	const ircd::string_view hostname
	{
		argc > 1?     // hostname given on command line
			argv[1]:
		argc > 0?     // hostname matches origin
			argv[0]:
			nullptr
	};

	// at least one hostname argument is required for now.
	if(!hostname)
		throw ircd::user_error
		{
			"Must specify the origin after any switched parameters."
		};

	// Associates libircd with our io_context and posts the initial routines
	// to that io_context. Execution of IRCd will then occur during ios::run()
	// note: only supports service for one hostname at this time.
	ircd::init(*ios, origin, hostname);

	// libircd does no signal handling (or at least none that you ever have to
	// care about); reaction to all signals happens out here instead. Handling
	// is done properly through the io_context which registers the handler for
	// the platform and then safely posts the received signal to the io_context
	// event loop. This means we lose the true instant hardware-interrupt gratitude
	// of signals but with the benefit of unconditional safety and cross-
	// platformness with windows etc.
	const construct::signals signals{*ios};

	// If the user wants to immediately drop to a command line without having to
	// send a ctrl-c for it, that is provided here.
	if(cmdline)
		construct::console::spawn();

	if(execute)
		construct::console::execute({execute});

	// Execution.
	// Blocks until a clean exit from a quit() or an exception comes out of it.
	ios->run();
}
catch(const ircd::user_error &e)
{
	if(ircd::debugmode)
		throw;

	fprintf(stderr, usererrstr, e.what());
	return EXIT_FAILURE;
}
catch(const std::exception &e)
{
	if(ircd::debugmode)
		throw;

	/*
	* Why EXIT_FAILURE here?
	* Because if ircd_die_cb() is called it's because of a fatal
	* error inside libcharybdis, and we don't know how to handle the
	* exception, so it is logical to return a FAILURE exit code here.
	*    --nenolod
	*
	* left the comment but the code is gone -jzk
	*/
	fprintf(stderr, fatalerrstr, e.what());
	return EXIT_FAILURE;
}

int
print_version()
{
	printf("VERSION :%s\n",
	       RB_VERSION);

	#ifdef CUSTOM_BRANDING
	printf("VERSION :based on %s-%s\n",
	       PACKAGE_NAME,
	       PACKAGE_VERSION);
	#endif

	return EXIT_SUCCESS;
}

bool
startup_checks()
try
{
	namespace fs = ircd::fs;

	fs::chdir(fs::get(fs::PREFIX));
	return true;
}
catch(const std::exception &e)
{
	fprintf(stderr, usererrstr, e.what());
	return false;
}

void
#ifdef HAVE_SYS_RESOURCE_H
enable_coredumps()
try
{
	//
	// Setup corefile size immediately after boot -kre
	//

	rlimit rlim;    // resource limits
	ircd::syscall(getrlimit, RLIMIT_CORE, &rlim);

	// Set corefilesize to maximum
	rlim.rlim_cur = rlim.rlim_max;
	ircd::syscall(setrlimit, RLIMIT_CORE, &rlim);
}
catch(const std::exception &e)
{
	std::cerr << "Failed to adjust rlimit: " << e.what() << std::endl;
}
#else
enable_coredumps()
{
}
#endif

void
applyargs()
{
	if(debugmode)
		ircd::debugmode.set("true");
	else
		ircd::debugmode.set("false");

	if(nolisten)
		ircd::net::listen.set("false");
	else
		ircd::net::listen.set("true");

	if(noautomod)
		ircd::mods::autoload.set("false");
	else
		ircd::mods::autoload.set("true");

	if(checkdb)
		ircd::db::open_check.set("true");
	else
		ircd::db::open_check.set("false");

	if(pitrecdb)
		ircd::db::open_recover.set("point");
	else
		ircd::db::open_recover.set("absolute");

	if(nodirect)
		ircd::fs::fd::opts::direct_io_enable.set("false");
	else
		ircd::fs::fd::opts::direct_io_enable.set("true");

	if(noaio)
		ircd::fs::aio::enable.set("false");
	else
		ircd::fs::aio::enable.set("true");
}
