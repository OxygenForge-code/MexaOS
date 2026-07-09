/* ============================================================
 *  MexaOS Kernel Library Header
 * ============================================================ */

#ifndef MEXAOS_KERNLIB_H
#define MEXAOS_KERNLIB_H

#include <stdint.h>

/* Boot info parser */
void parse_boot_info(uint64_t addr);

/* Kernel panic - noreturn */
void __attribute__((noreturn)) kpanic_impl(const char *msg, const char *file, int line);

/* Convenience macro */
#define kpanic(msg) kpanic_impl(msg, __FILE__, __LINE__)

#endif
