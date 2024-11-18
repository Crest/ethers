// vim: ft=c:ts=8 :

// Include library headers
#include <libxo/xo.h>

// Include system headers from subdirectories.
#include <net/ethernet.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

// Include system headers
#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <stdbool.h>
#include <unistd.h>

#include "allocator.h"
#include "cli_args.h"
#include "ethers_file.h"

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

static inline void
print_entry(const struct ether_addr addr[static const 1], const char name[static const 1])
{
	char buffer[sizeof("xx:xx:xx:xx:xx:xx")];
	if (xo_open_marker("entry") < 0) {
		xo_err(EX_IOERR, "Failed xo_open_marker(\"entry\")");
	}
	if (xo_open_instance("entries") < 0) {
		xo_err(EX_IOERR, "Failed xo_open_instance(\"entries\")");
	}
	if (xo_emit("{P:  - }{L:Address}{D: = }{:address}{D:, }{L:Hostname}{D: = }{:hostname}\n", ether_ntoa_r(addr, buffer), name) < 0) {
		xo_err(EX_IOERR, "Failed to emit entry");
	}
	if (xo_close_marker("entry") < 0) {
		xo_err(EX_IOERR, "Failed xo_close_marker(\"entry\")");
	}
}

static inline void
print_entries(const struct ethers_file file[static const 1]) 
{
	struct ethers_reader reader = ethers_reader_create(file);
	ssize_t              delta;
	struct ether_addr    addr[1];
	char                 name[MAXHOSTNAMELEN];

	if (xo_emit("{Lc:Entries}\n") < 0) {
		xo_err(EX_IOERR, "Failed xo_emit()");
	} else if (xo_open_marker("entries") < 0) {
		xo_err(EX_IOERR, "Failed xo_open_marker(\"entries\")");
	} else if (xo_open_list("entries") < 0) {
		xo_err(EX_IOERR, "Failed xo_open_list(\"entries\")");
	}

	for (delta = ethers_reader_read(&reader, addr, name); delta > 0; delta = ethers_reader_read(&reader, addr, name)) {
		print_entry(addr, name);
	}

	if (delta < 0) {
		const char *_Nonnull const ethers_path = file->args->ethers_path;
		xo_err(EX_DATAERR, "Failed to parse line %zu of ethers(5) file: %s", reader.line_number, ethers_path);
	} else if (xo_close_marker("entries") < 0) {
		xo_err(EX_IOERR, "Failed xo_close_marker(\"entries\")");
	}
}

static void
open_entries(void)
{
	static const char list[] = "entries";
	if (xo_open_marker(list) < 0) {
		xo_err(EX_IOERR, "xo_open_marker(\"%s\") failed", list);
	} else if (xo_open_list(list) < 0) {
		xo_err(EX_IOERR, "xo_open_list(\"%s\") failed", list);
	}
}

static void
close_entries(void)
{
	static const char list[] = "entries";
	if (xo_close_marker(list) < 0) {
		xo_err(EX_IOERR, "xo_close_marker(\"%s\") failed", list);
	}
}

static void
emit_entry(const struct ether_addr *_Nonnull const addr, const char *_Nonnull const name)
{
	char addr_buffer[sizeof("xx:xx:xx:xx:xx:xx")];
	if (xo_open_marker("entry") < 0) {
		xo_err(EX_IOERR, "xo_open_marker(\"entry\") failed");
	}
	if (xo_open_instance("entries") < 0) {
		xo_err(EX_IOERR, "xo_open_instance(\"entries\") failed");
	}
	if (xo_emit("{:address} {:hostname}\n", ether_ntoa_r(addr, addr_buffer), name) < 0) {
		xo_err(EX_IOERR, "Failed to write lease to standard output");
	}
	if (xo_close_marker("entry") < 0) {
		xo_err(EX_IOERR, "xo_close_marker(\"entry\") failed");
	}
}

static void
allocate_entries(const struct ethers_file file[const static 1])
{
	const struct cli_args      *_Nonnull const args        = file->args;
	const char *_Nonnull const *_Nonnull const start       = args->names_start;
	const char *_Nonnull const *_Nonnull const end         = args->names_end;
	const char *_Nonnull const                 ethers_path = args->ethers_path;
	const struct ether_addr                    min         = args->min_mac;
	const struct ether_addr                    max         = args->max_mac;

	open_entries();

	size_t count = (size_t)(end - start);
	const char *_Nonnull names[count];
	for (size_t i = 0; i < (size_t)(end - start); i++) {
		names[i] = start[i];
	}

	struct allocator allocator __attribute__((cleanup(allocator_cleanup))) = allocator_create(min, max);

	struct ethers_reader reader = ethers_reader_create(file); 
	struct ethers_buffer buffer = ETHERS_BUFFER_INIT;
	struct ethers_writer writer __attribute__((cleanup(ethers_writer_close))) = ethers_writer_create(file, &buffer);

	{
		ssize_t           delta;
		struct ether_addr addr[1];
		char              name[MAXHOSTNAMELEN];
		for (delta = ethers_reader_read(&reader, addr, name); delta > 0; delta = ethers_reader_read(&reader, addr, name)) {
			allocator_claim(allocator, addr);
			for (size_t i = 0; i < count; i++) {
				if (!strcmp(name, names[i])) {
					names[i] = names[--count];
					emit_entry(addr, name);
					break;
				}
			}
		}
		if (delta < 0) {
			xo_err(EX_DATAERR, "Failed read in line %zu of ethers file '%s'.", reader.line_number, ethers_path);
		}
	}

	for (size_t i = 0; i < count; i++) {
		const char *_Nonnull const name = names[i];
		struct ether_addr addr[1];
		const bool ok = allocator_alloc(allocator, addr);
		if (!ok) {
			xo_errx(EX_UNAVAILABLE, "Failed to allocate MAC address for hostname '%s'.", name);
		}
		emit_entry(addr, name);
		if (ethers_writer_write(&writer, addr, name) < 0) {
			xo_err(EX_OSERR, "Failed to write to memory stream");
		}
	}

	if (ethers_writer_flush(&writer) < 0) {
		xo_err(EX_OSERR, "Failed to write new mappings to ethers(5) file: %s", file->args->ethers_path);
	}

	close_entries();
}

int
main(int argc, char **argv)
{
	static const char prog_name[] = PROG_NAME;
	setprogname(prog_name);
	atexit(xo_finish_atexit);

	argc = xo_parse_args(argc, argv);

	xo_set_program(prog_name);
	xo_open_container(prog_name);

	const struct cli_args args = parse_cli_args(argc, argv);

	if (args.usage) {
		print_usage_and_exit();
	} else if (args.help) {
		print_help_and_exit();
	}

	if (args.verbose) {
		print_cli_args(&args);
	}

	const struct ethers_file file __attribute__((cleanup(ethers_file_cleanup))) = ethers_file_open(&args);

	if (args.verbose) {
		print_entries(&file);
	}

	allocate_entries(&file); 

	xo_close_container(prog_name);
	return EX_OK;
}

#pragma clang diagnostic pop
