// vim: ft=c:ts=8 :

#include "ethers_file.h"
#include "scan.h"

// Include library headers
#include <libxo/xo.h>

// Include system headers from subdirectories.
#include <net/ethernet.h>
#include <sys/mman.h>
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

static void
ethers_unmap(const struct valid map)
{
	if (!is_empty(map)) {
		void *_Nonnull const base = (void*)(uintptr_t)map.start;
		const size_t         size = (size_t)(map.end - map.start);
		if (munmap(base, size) != 0) {
			xo_err(EX_IOERR, "Failed to munmap() ethers file");
		}
	}
}

void
ethers_file_close(const struct ethers_file file)
{
	ethers_unmap(file.map);
	if (file.fd >= 0) {
		if (close(file.fd) != 0) {
			xo_err(EX_IOERR, "Failed to close() ethers file");
		}
	}
}

void
ethers_file_cleanup(const struct ethers_file file[const static 1])
{
	ethers_file_close(*file);
}

static struct valid
ethers_mmap(const int fd, const char path[static const 1])
{
	const size_t size = ({
		struct stat stat_buffer;
		if (fstat(fd, &stat_buffer) != 0) {
			xo_err(EX_IOERR, "Failed to fstat() ethers file '%s'", path);
		} else if (stat_buffer.st_size < 0) {
			xo_errx(EX_IOERR, "Fstat() reported negative size for ethers file '%s'", path);
		} else if (stat_buffer.st_size == 0) {
			return empty;
		}
		(size_t)stat_buffer.st_size;
	});
	const char *_Nonnull const start = ({
		const int perms = PROT_READ;
		const int flags = MAP_PRIVATE | MAP_PREFAULT_READ;
		void *_Nullable const addr = mmap(NULL, size, perms, flags, fd, 0);
		if (addr == MAP_FAILED) {
			xo_err(EX_IOERR, "Failed to mmap() ethers file '%s' for reading", path);
		}
		addr;
	});

	return VALID(start, &start[size]);
}

static int
ethers_create(const struct cli_args args[static 1])
{
	const char *_Nonnull const path = args->ethers_path;
	const int                  flags = O_RDWR | O_SHLOCK | O_DSYNC | O_CREAT | O_APPEND;
	const mode_t               perms = 0644;
	const size_t               size = strlen(path) + 1;
	char                       copy[PATH_MAX];

	const int valid_dir_fd = ({
		memcpy(copy, path, size);
		const char *_Nonnull const dir_path = dirname(copy);
		const int maybe_dir_fd = openat(AT_FDCWD, dir_path, O_DIRECTORY);
		if (maybe_dir_fd < 0) {
			xo_err(EX_CONFIG, "Failed to open directory supposed to contain the ethers file '%s'", path);
		}
		maybe_dir_fd;
	});

	const int valid_fd = ({
		memcpy(copy, path, size);
		const char *_Nonnull base_path = basename(copy);
		const int maybe_fd = openat(valid_dir_fd, base_path, flags, perms);
		if (maybe_fd < 0) {
			xo_err(EX_CONFIG, "Failed to open ethers file '%s' relative to parent directory", path);
		}
		maybe_fd;
		
	});

	if (fsync(valid_fd)) {
		xo_err(EX_IOERR, "Failed to fsync() created ethers file '%s'", path);
	} else if (fsync(valid_dir_fd)) {
		xo_err(EX_IOERR, "Failed to fsync() directory containing created ethers file '%s'", path);
	} else if (close(valid_dir_fd)) {
		xo_err(EX_IOERR, "Failed to close() directory containing created ethers file '%s'", path);
	}

	return valid_fd;
}

struct ethers_file
ethers_file_open(const struct cli_args args[const static 1])
{
	const char *_Nonnull const path  = args->ethers_path;
	const int                  flags = O_RDWR | O_SHLOCK | O_DSYNC | O_APPEND;

	const int valid_fd = ({
		const int maybe_fd = openat(AT_FDCWD, path, flags);

		if (maybe_fd < 0 && errno != ENOENT) {
			xo_warn("Failed to open ethers file '%s'", path);
			return (struct ethers_file) {
				.args     = args,
				.map      = empty,
				.fd       = -1,
				.reserved = 0
			};
		}

		maybe_fd >= 0 ? maybe_fd : ethers_create(args);
	});

	const struct valid map = ethers_mmap(valid_fd, args->ethers_path);

	return (struct ethers_file) {
		.args     = args,
		.map      = map,
		.fd       = valid_fd,
		.reserved = 0
	};
}

struct ethers_reader
ethers_reader_create(const struct ethers_file *_Nonnull const file)
{
	return (struct ethers_reader) {
		.file        = file,
		.input       = file->map,
		.line_number = 0
	};
}

struct ethers_writer
ethers_writer_create(const struct ethers_file file[static const 1], struct ethers_buffer buffer[static const 1])
{
	const struct ethers_writer writer = {
		.file   = file,
		.stream = open_memstream(&buffer->buffer, &buffer->size),
		.buffer = buffer
	};

	if (!writer.stream) {
		xo_err(EX_OSERR, "Failed open_memstream()");
	}

	return writer;
}

static struct valid
trim_left_whitespace(struct valid valid)
{
	for (; valid.start != valid.end; valid.start++) {
		const char byte = valid.start[0];
		if (byte != '\t' && byte != '\n' && byte != ' ') {
			return valid;
		}
	}
	return valid;
}

// Attempt to read the next line and the MAC address and hostname.
// Returns none on error and the remaining input on success.
// On success the MAC address and hostname are copied out to fixed size buffers.
//
// (It's a cleaner ether_line(3) reimplementation).
ssize_t
ethers_reader_read(struct ethers_reader reader[const static 1], struct ether_addr addr[const static 1], char name[const static MAXHOSTNAMELEN])
{
	// Dig through indirections for the input file path and line number to use in error messages.
	const struct ethers_file *_Nonnull const file        = reader->file;
	const struct cli_args    *_Nonnull const args        = file->args;
	const char               *_Nonnull const ethers_path = args->ethers_path;
	size_t                                   line_number;

retry:	line_number = ++(reader->line_number);
	if (is_empty(reader->input)) {
		return 0;
	}
	
	// Split the input on the first new line.
	// Capture the remaining lines to return them on success.
	const struct split input = split_line(reader->input);
	reader->input = or_empty(input.after);

	// Split input into a line of whitespace separated fields
	// followed by an optional comment.
	const struct split comment = split_comment(input.before);
	const struct valid line    = comment.before;

	// Skip over empty lines (no fields only whitespaces or comments).
	struct valid field = trim_left_whitespace(line);
	if (is_empty(field)) {
		goto retry;
	}

	// Extract MAC address out of the 1st field on the line.
	struct maybe space = scan_addr(line, addr);
	if (is_null(space)) {
		xo_warnx("Invalid MAC address in line %zu of ethers file '%s'.", line_number, ethers_path);
		return -1;
	}

	// The MAC address and hostname must be separated by at least one whitespace.
	field = trim_left_whitespace(or_empty(space));
	if (field.start == space.start) {
		xo_warnx("Missing whitespace between MAC address and name in line %zu of ethers file '%s'.", line_number, ethers_path);
		return -1;
	}

	// Locate the hostname (2nd field) on the line.
	struct maybe maybe_name = none;
	space = scan_name(field, &maybe_name);
	const size_t name_length = (size_t)(maybe_name.end - maybe_name.start);
	if (is_null(space)) {
		xo_warnx("Invalid name in line %zu of ethers file '%s'.", line_number, ethers_path);
		return -1;
	} else if (name_length >= MAXHOSTNAMELEN) {
		xo_warnx("The hostname in line %zu of ethers file '%s' is too long.", line_number, ethers_path);
		return -1;
	} else {
		memcpy(name, maybe_name.start, name_length);
		name[name_length] = '\0';
	}

	// Prohibit further fields on the line.
	field = trim_left_whitespace(or_empty(space));
	if (!is_empty(field)) {
		xo_warnx("Too many fields on line %zu of ethers file '%s'.", line_number, ethers_path);
		return -1;
	}

	return 1;
}

ssize_t
ethers_writer_write(struct ethers_writer writer[const static 1], const struct ether_addr addr[const static 1], const char name[const static 1])
{
	return (ssize_t)fprintf(writer->stream, "%02x:%02x:%02x:%02x:%02x:%02x %s\n",
		addr->octet[0], addr->octet[1], addr->octet[2], addr->octet[3], addr->octet[4], addr->octet[5], name
	);
}

void
ethers_buffer_free(struct ethers_buffer buffer[const static 1])
{
	free(buffer->buffer);
	buffer->buffer = NULL;
	buffer->size   = 0;
}

ssize_t
ethers_writer_flush(struct ethers_writer writer[const static 1])
{
	struct stat stat_buffer;
	FILE *const stream = writer->stream;
	const int fd = writer->file->fd;
	const char *_Nonnull const ethers_path = writer->file->args->ethers_path;
	if (fflush(stream) != 0) {
		xo_err(EX_OSERR, "Failed to flush memory stream");
	} else if (flock(fd, LOCK_EX) != 0) {
		xo_err(EX_IOERR, "Failed to lock ethers(5) file for writing: %s", ethers_path);
	} else if (fstat(fd, &stat_buffer) != 0) {
		xo_err(EX_IOERR, "Failed to fstat() ethers file '%s'", ethers_path);
	}

	const char *_Nullable const buffer  = writer->buffer->buffer;
	const size_t                size    = writer->buffer->size;
	const ssize_t               written = write(fd, buffer, size);
	if (written < 0) {
		return written;
	} else if (size != (size_t)written) {
		if (ftruncate(fd, stat_buffer.st_size) != 0) {
			xo_err(EX_IOERR, "Failed to ftruncate() away partial write to ethers(5) file: %s", ethers_path);
		}
		errno = EIO;
		return -1;
	} else if (flock(fd, LOCK_SH) != 0) {
		xo_err(EX_IOERR, "Failed to downgrade ethers(5) file lock: %s", ethers_path);
	}
	ethers_buffer_free(writer->buffer);
	return written;
}

void
ethers_writer_close(struct ethers_writer writer[const static 1])
{
	if (fclose(writer->stream) != 0) {
		xo_err(EX_OSERR, "Failed to close memory stream");
	}
	ethers_buffer_free(writer->buffer);
}


#pragma clang diagnostic pop

