// vim: ft=c:ts=8 :

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <libxo/xo.h>

#include <net/ethernet.h>
#include <bitstring.h>
#include <stdbool.h>

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

struct allocator {
	bitstr_t *_Nonnull const bitstring;
	const uint64_t           offset;
	const uint64_t           size;
};

struct allocator allocator_create(const struct ether_addr min, const struct ether_addr max);
void             allocator_destroy(struct allocator allocator);
void             allocator_cleanup(struct allocator allocator[static const 1]);
void             allocator_claim(struct allocator allocator, const struct ether_addr addr[const static 1]);
bool             allocator_alloc(struct allocator allocator, struct ether_addr addr[const static 1]);

#pragma clang diagnostic pop

#endif /* ALLOCATOR_H */
