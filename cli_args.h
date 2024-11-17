// vim: ft=c:ts=8 :

// Include system headers from subdirectories.
#include <net/ethernet.h>
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

// Define string constants as macros (constexpr is a C23 feature).
#define PROG_NAME   "ethers"
#define ETHERS_PATH "/etc/ethers"

struct cli_args {
	const char *_Nonnull const *_Nonnull names_start;
	const char *_Nonnull const *_Nonnull names_end;

	const char *_Nonnull  ethers_path;

	struct ether_addr     min_mac;
	struct ether_addr     max_mac;

	bool                  help;
	bool                  quiet;
	bool                  usage;
	bool                  verbose;
};


void __attribute__((noreturn)) print_usage_and_exit(void);
void __attribute__((noreturn)) print_help_and_exit(void);
void                           print_cli_args(const struct cli_args args[const static 1]);
struct cli_args                parse_cli_args(int argc, char *_Nullable *_Nonnull argv);
 
#pragma clang diagnostic pop
