// vim: ft=c:ts=8 :

// Include system headers from subdirectories.
#include <net/ethernet.h>
#include <limits.h>

#include "slice.h"
#include "scan.h"

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

static struct maybe
scan_literal(struct valid valid, const char literal)
{
	if (is_empty(valid) || *valid.start != literal) {
		return none;
	} else {
		valid.start++;
		return valid.maybe;
	}
}

static struct maybe
scan_hexdigit(struct valid input, uint8_t *_Nonnull const restrict nibble)
{
	static const uint8_t allowed[UCHAR_MAX + 1] = {
		['0' ... '9'] = '0',
		['a' ... 'f'] = 'a' - 0x0a,
		['A' ... 'F'] = 'A' - 0x0a
	};
	if (input.start >= input.end) {
		return none;
	}
	const uint8_t digit     = *(const uint8_t *)input.start;
	const uint8_t substract = allowed[digit];
	if (substract == 0) {
		return none;
	}
	*nibble <<= 4;
	*nibble +=  digit - substract;
	input.start++;
	return input.maybe;
}

static struct maybe
scan_octet(struct valid input, uint8_t byte[const static 1])
{
	*byte = 0x00;
	struct maybe maybe = scan_hexdigit(input, byte);
	if (is_null(maybe)) {
		return maybe;
	} else {
		input = or_empty(maybe);
	}

	return scan_hexdigit(input, byte);
}

static struct maybe
scan_octet_colon(struct valid input, uint8_t byte[const static 1])
{
	struct maybe maybe = scan_octet(input, byte);
	if (is_null(maybe)) {
		return maybe;
	} else {
		input = or_empty(maybe);
	}

	maybe = scan_literal(input, ':');
	if (is_null(maybe)) {
		return maybe;
	} else {
		input = or_empty(maybe);
	}

	return input.maybe;
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

struct maybe
scan_addr(struct valid input, struct ether_addr addr[const static 1])
{
	input = trim_left_whitespace(input);
	for (size_t i = 0; i < 5; i++) {
		uint8_t *_Nonnull const octet = &addr->octet[i];
		const struct maybe maybe = scan_octet_colon(input, octet);
		if (is_null(maybe)) {
			return maybe;
		} else {
			input = or_empty(maybe);
		}
	}
	uint8_t *_Nonnull const octet = &addr->octet[5];
	return scan_octet(input, octet);
}

struct maybe
scan_name(struct valid input, struct maybe name[const static 1])
{
	static const bool allowed[UCHAR_MAX + 1] = {
		['-'        ] = true,
		['.'        ] = true,
		['0' ... '9'] = true,
		['A' ... 'Z'] = true,
		['a' ... 'z'] = true
	};

	input = trim_left_whitespace(input);
	if (is_empty(input)) {
		return (*name = none);
	}


	const unsigned char first = *(__typeof__(first)*)input.start;
	if (!allowed[first]) {
		return (*name = none);
	}
	const char *_Nonnull const start = input.start;

	for (input.start++; input.start != input.end; input.start++) {
		const unsigned char other = *(__typeof__(other)*)input.start;
		if (!allowed[other]) {
			break;
		}
	}
	*name = MAYBE(start, input.start);
	return input.maybe;
}

#pragma clang diagnostic pop
