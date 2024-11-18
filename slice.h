// vim: ft=c:ts=8 :

#ifndef SLICE_H
#define SLICE_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

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

struct valid {
	union {
		struct {
			const char *_Nonnull start;
			const char *_Nonnull end;
		};
		struct maybe {
			const char *_Nullable start;
			const char *_Nullable end;
		} maybe;
	};
};

struct split {
	struct valid before;
	struct maybe after;
};

#define VALID(_start, _end) ((struct valid) { .start = (_start), .end = (_end) })
#define MAYBE(_start, _end) ((struct maybe) { .start = (_start), .end = (_end) })

static const char         empty_string[] = "";
static const struct valid empty          = VALID(empty_string, empty_string);
static const struct maybe none           = MAYBE(NULL, NULL);

static inline size_t
valid_length(const struct valid valid)
{
	return (size_t)(valid.end - valid.start);
}

static inline bool
is_valid(const struct maybe maybe)
{
	return maybe.start != NULL && maybe.end != NULL && maybe.end >= maybe.start;
}

static inline bool
is_null(const struct maybe maybe)
{
	return maybe.start == NULL || maybe.end == NULL || maybe.end < maybe.start;
}

static inline bool
is_empty(const struct valid valid) {
	return valid.start == valid.end;
}

static inline struct valid
or_empty(const struct maybe maybe)
{
	return is_valid(maybe) ? VALID((const char *_Nonnull)maybe.start, (const char *_Nonnull)maybe.end) : empty;
}

static inline struct valid
valid_string(const char string[const static 1])
{
	const size_t length = strlen(string);
	return (struct valid) { .start = string, .end = &string[length] };
}

static struct split
split(struct valid valid, const char separator)
{
	const size_t length = valid_length(valid);
	const char *_Nonnull const found = memchr(valid.start, separator, length);
	if (found == NULL) {
		return (struct split) { .before = valid, .after = none };
	} else {
		return (struct split) {
			.before = { .start = valid.start, .end = found     },
			.after  = { .start = found + 1  , .end = valid.end }
		};
	}
}

static inline struct split
split_line(struct valid valid)
{
	return split(valid, '\n');
}

static inline struct split
split_comment(struct valid valid)
{
	return split(valid, '#');
}

#pragma clang diagnostic pop
#endif /* SLICE_H */
