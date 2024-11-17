// vim: ft=c:ts=8 :

#include "allocator.h"

#include <libxo/xo.h>
#include <stdint.h>
#include <sysexits.h>

#if __STDC_VERSION__ >= 202311L
static_assert(true); // Workaround for https://github.com/clangd/clangd/issues/1167
#endif 

#pragma clang diagnostic push

// Set *very* aggressive diagnostics specific to clang (version 19.1.3).
// These diagnostics aren't stable between compiler versions.
// If used like this they're **EXPECTED** to break break the build,
// but they're also useful for catching errors.
// Use `make debug` to build with these diagnostics.
#ifdef DRACONIC
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

static inline uint64_t
addr_to_u64(const struct ether_addr addr)
{
	return  ((uint64_t)(addr.octet[0])) << 5*8 |
		((uint64_t)(addr.octet[1])) << 4*8 |
		((uint64_t)(addr.octet[2])) << 3*8 |
		((uint64_t)(addr.octet[3])) << 2*8 |
		((uint64_t)(addr.octet[4])) << 1*8 |
		((uint64_t)(addr.octet[5]));
}

static inline struct ether_addr
u64_to_addr(const uint64_t u64) {
	return (struct ether_addr) {
		.octet = {
			[0] = (uint8_t)(u64 >> 5*8),
			[1] = (uint8_t)(u64 >> 4*8),
			[2] = (uint8_t)(u64 >> 3*8),
			[3] = (uint8_t)(u64 >> 2*8),
			[4] = (uint8_t)(u64 >> 1*8),
			[5] = (uint8_t)(u64 >> 0*8)
		}
	};
}

struct allocator
allocator_create(const struct ether_addr min, const struct ether_addr max)
{
	const uint64_t offset = addr_to_u64(min);
	const uint64_t size   = addr_to_u64(max) - offset;
	if (size > SIZE_MAX) {
		xo_errx(EX_SOFTWARE, "Address space too small for %lu bit bitstring.", size);
	}
	bitstr_t *_Nullable const bitstring = bit_alloc((size_t)size);
	if (bitstring == NULL) {
		xo_err(EX_OSERR, "Failed to allocate %lu bit bitstring.", size);
	}
	return (struct allocator) {
		.bitstring = bitstring,
		.offset    = offset,
		.size      = size
	};
}

void
allocator_destroy(struct allocator allocator)
{
	free(allocator.bitstring);
}

void
allocator_cleanup(struct allocator allocator[static const 1])
{
	allocator_destroy(*allocator);
}

void
allocator_claim(const struct allocator allocator, const struct ether_addr addr[const static 1]) {
	uint64_t position = addr_to_u64(*addr) - allocator.offset;
	if (position < allocator.size) {
		bit_set(allocator.bitstring, position);
	}
}

bool
allocator_alloc(const struct allocator allocator, struct ether_addr addr[const static 1])
{
	ssize_t first = -1;
	bit_ffc(allocator.bitstring, allocator.size, &first);
	if (first < 0) {
		return false;
	} else {
		const size_t bit = (size_t)first;
		bit_set(allocator.bitstring, first);
		*addr = u64_to_addr(bit + allocator.offset);
		return true;
	}
}

#pragma clang diagnostic pop
