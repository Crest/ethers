// vim: ft=c:ts=8 :

#ifndef ETHERS_FILE_H
#define ETHERS_FILE_H

#include <sys/param.h>
#include <net/ethernet.h>
#include <stdio.h>

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

struct ethers_file {
	const struct cli_args *_Nonnull const args;
	const struct valid                    map;
	const int                             fd;
	const int                             reserved;
};

struct ethers_reader {
	const struct ethers_file *_Nonnull const file;
	struct valid                             input;
	size_t                                   line_number;
};

struct ethers_buffer {
	char   *_Nullable buffer;
	size_t            size;
};

struct ethers_writer {
	const struct ethers_file *_Nonnull const file;
	FILE                     *_Nonnull const stream;
	struct ethers_buffer     *_Nonnull const buffer;
};

struct ethers_file   ethers_file_open(const struct cli_args args[const static 1]);
void                 ethers_file_close(const struct ethers_file file);
void                 ethers_file_cleanup(const struct ethers_file file[const static 1]);

struct ethers_reader ethers_reader_create(const struct ethers_file *_Nonnull const file);
ssize_t              ethers_reader_read(struct ethers_reader reader[const static 1], struct ether_addr addr[const static 1], char name[const static MAXHOSTNAMELEN]);

#define ETHERS_BUFFER_INIT ((struct ethers_buffer) { .buffer = NULL, .size = 0 })
void                 ethers_buffer_free(struct ethers_buffer[static const 1]);

struct ethers_writer ethers_writer_create(const struct ethers_file *_Nonnull const file, struct ethers_buffer buffer[const static 1]);
ssize_t              ethers_writer_write(struct ethers_writer writer[const static 1], const struct ether_addr[const static 1], const char name[const static 1]);
ssize_t              ethers_writer_flush(struct ethers_writer writer[const static 1]);
void                 ethers_writer_close(struct ethers_writer writer[const static 1]);

#pragma clang diagnostic pop
#endif /* ETHERS_FILE_H */
