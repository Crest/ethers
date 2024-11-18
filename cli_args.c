// vim: ft=c:ts=8 :


#include <libxo/xo.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "scan.h"
#include "cli_args.h"

#if __STDC_VERSION__ >= 202311L
static_assert(true); // Workaround for https://github.com/clangd/clangd/issues/1167
#endif

#pragma clang diagnostic push

// Set *very* aggressive diagnostics specific to clang (version 19.1.3).
// These diagnostics aren't stable between compiler versions.
// If used like this they're **EXPECTED** to break break the build,
// but they're also useful for catching errors.
// Use `make debug` to build with these diagnostics.
#ifdef RACONIC
#pragma clang diagnostic error   "-Weverything"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wpre-c11-compat"
#pragma clang diagnostic ignored "-Wpre-c23-compat"
#pragma clang diagnostic ignored "-Wc2y-extensions"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wnullability-extension"
#pragma clang diagnostic ignored "-Wvla"
#pragma clang diagnostic ignored "-Wgnu-statement-expression"
#pragma clang diagnostic ignored "-Wgnu-conditional-omitted-operand"
#pragma clang diagnostic ignored "-Wgnu-case-range"
#pragma clang diagnostic ignored "-Wgnu-designator"
#endif

// Define string constants as macros (constexpr is a C23 feature).
#define PROG_NAME   "ethers"
#define ETHERS_PATH "/etc/ethers"

static const char usage_message[] =
	"usage: " PROG_NAME
	" [-h]"          /* -h         : help                   */
	" [-q]"          /* -q         : quiet                  */
	" [-v]"          /* -v         : verbose                */
	" [-f <ethers>]" /* -f <ether> : path to ethers(5) file */
	" [-m <min>]"    /* -m <min>   : minimum allowed MAC    */
	" [-M <max>]"    /* -M <max>   : maximum allowed MAC    */
	" [<name> ...]";

static inline const char *_Nonnull
bool_to_string(const bool boolean)
{
	return boolean ? "true" : "false";
}

struct addr_buffer { char addr[sizeof("xx:xx:xx:xx:xx:xx")]; };

static struct addr_buffer
addr_to_string(const struct ether_addr addr)
{
	struct addr_buffer buffer;
	ether_ntoa_r(&addr, buffer.addr);
	return buffer;
}

static inline void
open_container(const char name[const static 1]) {
	if (xo_open_marker(name) < 0) {
		xo_err(EX_IOERR, "Failed to open container marker: \"%s\"", name);
	}
	if (xo_open_container(name) < 0) {
		xo_err(EX_IOERR, "Failed to open container: \"%s\"", name);
	}
}

static inline void
close_container(const char name[const static 1]) {
	if (xo_close_marker(name) < 0) {
		xo_err(EX_IOERR, "Failed to close container marker: \"%s\"", name);
	}
}

static inline void
emit_label(const char label[const static 1]) {
	if (xo_emit("{Lc:/%s}\n", label) < 0) {
		xo_err(EX_IOERR, "Failed to emit label: \"%s\"", label);
	}
}

static inline void
emit_help(const struct cli_args args[const static 1]) {
	if (xo_emit("{P:\t}{Lwc:Help}{P:       }{D: = }{:help}\n"  , bool_to_string(args->help   )) < 0) {
		xo_err(EX_IOERR, "Failed to emit help argument");
	}
}

static inline void
emit_quiet(const struct cli_args args[const static 1]) {
	if (xo_emit("{P:\t}{Lwc:Quiet}{P:      }{D: = }{:quiet}\n"  , bool_to_string(args->quiet  )) < 0) {
		xo_err(EX_IOERR, "Failed to emit quiet argument");
	}
}

static inline void
emit_usage(const struct cli_args args[const static 1]) {
	if (xo_emit("{P:\t}{Lwc:Usage}{P:      }{D: = }{:usage}\n"  , bool_to_string(args->usage  )) < 0) {
		xo_err(EX_IOERR, "Failed to emit usage argument");
	}
}

static inline void
emit_verbose(const struct cli_args args[const static 1]) {
	if (xo_emit("{P:\t}{Lwc:Verbose}{P:    }{D: = }{:verbose}\n", bool_to_string(args->verbose)) < 0) {
		xo_err(EX_IOERR, "Failed to emit verbose argument");
	}
}

static inline void
emit_min_mac(const struct cli_args args[const static 1]) {
	if (xo_emit("{P:\t}{Lwc:Minimum MAC}{P:}{D: = }{:min-mac}\n", addr_to_string(args->min_mac).addr) < 0) {
		xo_err(EX_IOERR, "Failed to emit minimum MAC address argument");
	}
}

static inline void
emit_max_mac(const struct cli_args args[const static 1]) {
	if (xo_emit("{P:\t}{Lwc:Maximum MAC}{P:}{D: = }{:max-mac}\n", addr_to_string(args->max_mac).addr) < 0) {
		xo_err(EX_IOERR, "Failed to emit maximum MAC address argument");
	}
}

static inline void
emit_hostname(const char *_Nonnull const *_Nonnull const name) {
	if (xo_emit("{P:\t}{Lwc:Name}{P:       }{D: = }{l:name}\n", *name) < 0) {
		xo_err(EX_IOERR, "Failed to emit hostname argument");
	}
}

void
print_cli_args(const struct cli_args args[const static 1])
{
	static const char container[] = "arguments";

	open_container(container);
	{
		const char *_Nonnull const *_Nonnull const start = args->names_start;
		const char *_Nonnull const *_Nonnull const end   = args->names_end;

		emit_label("CLI arguments");

		emit_help(args);
		emit_quiet(args);
		emit_usage(args);
		emit_verbose(args);
		emit_min_mac(args);
		emit_max_mac(args);

		for (const char *_Nonnull const *_Nonnull name = start; name != end; name++) {
			emit_hostname(name);
		}
	}
	close_container(container);
}

void __attribute__((noreturn))
print_usage_and_exit(void)
{
	xo_errx(EX_USAGE, "usage: %s", usage_message);
	exit(EX_USAGE);
}

void __attribute__((noreturn))
print_help_and_exit(void)
{
	xo_emit("{Lwc:usage}{:help}\n", usage_message);
	exit(0);
}

struct cli_args
parse_cli_args(int argc, char **argv)
{
	// Start with the default values.
	static const char *_Nonnull const empty_name = "";
	static const char ethers_path[] = ETHERS_PATH;
	struct cli_args args = {
		.names_start = &empty_name,
		.names_end   = &empty_name,
		.ethers_path = ethers_path,
		.min_mac     = { .octet = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 } },
		.max_mac     = { .octet = { 0x02, 0x00, 0xff, 0xff, 0xff, 0xff } },
		.help        = false,
		.quiet       = false,
		.usage       = false,
		.verbose     = false
	};

	// Process the CLI options using traditional getopt(3).
	// There are no mandatory options.
	int option;
	while ((option = getopt(argc, argv, "hqvm:M:f:")) != -1) {
		switch (option) {
		case 'h': // The help option takes no argument.
			args.help  = true;
			args.quiet = false;
			break;

		case 'q': // The quiet option takes no argument.
			args.verbose = false;
			args.quiet   = true;
			break;

		case 'v': // The verbose option takes no argument.
			args.verbose = true;
			args.quiet   = false;
			break;

		case 'f': // The ethers(5) file option argument must be a possible path (not empty, not too long).:w
			if (optarg[0] == '\0') {
				xo_errx(EX_DATAERR, "The -f <ether> argument is empty.");
			} else if (strlen(optarg) >= PATH_MAX) {
				xo_errx(EX_DATAERR, "The -f <ether> argument is too long.");
			}
			args.ethers_path = optarg;
			break;

		case 'm': // The minimum MAC address option argument must be a valid MAC address.
			if (is_null(scan_addr(valid_string(optarg), &args.min_mac))) {
				xo_errx(EX_DATAERR, "Invalid -m <min_mac> argument '%s'", optarg);
			}
			break;

		case 'M': // The maximum MAC address option argument must be a valid MAC address.
			if (is_null(scan_addr(valid_string(optarg), &args.max_mac))) {
				xo_errx(EX_DATAERR, "Invalid -M <max_mac> argument '%s'", optarg);
			}
			break;

		default: // Encountered an invalid option.
			args.usage = true;
			args.quiet = false;
			break;
		}
	}

	// Any remaining arguments are the [<host> ...] arguments
	argc -= optind;
	argv += optind;
	if (argc >= 1) {
		args.names_start = (const char *_Nonnull const *_Nonnull const) &argv[0   ];
		args.names_end   = (const char *_Nonnull const *_Nonnull const) &argv[argc];
	}

	// The minimum MAC address address must not be larger than the maximum MAC address.
	if (memcmp(&args.min_mac, &args.max_mac, sizeof(struct ether_addr)) > 0) {
		xo_errx(EX_DATAERR, "The -m <min_mac> argument is larger than the -M <max_mac> argument");
	}

	return args;
}

#pragma clang diagnostic pop
