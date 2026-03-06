#ifndef ITIDY_VERSION_INFO_H
#define ITIDY_VERSION_INFO_H

/* Centralized runtime version string for all binaries */
/* v2.0+ = ReAction GUI (WB 3.2+), v1.x = GadTools GUI (WB 3.0/3.1) */
#define ITIDY_VERSION "2.0"

/* ---------------------------------------------------------------
 * Compile-time CPU and FPU target strings (VBCC m68k macros)
 * These reflect the -cpu= and -fpu= flags passed to the compiler.
 * --------------------------------------------------------------- */
#if defined(__M68060)
#  define ITIDY_CPU_STR "68060"
#elif defined(__M68040)
#  define ITIDY_CPU_STR "68040"
#elif defined(__M68030)
#  define ITIDY_CPU_STR "68030"
#elif defined(__M68020)
#  define ITIDY_CPU_STR "68020"
#else
#  define ITIDY_CPU_STR "68000"
#endif

#if defined(__M68882) || defined(__M68881)
#  define ITIDY_FPU_STR " + FPU"
#else
#  define ITIDY_FPU_STR ""
#endif

/* Combined target string, e.g. "68000" or "68020 + FPU" */
#define ITIDY_BUILD_TARGET ITIDY_CPU_STR ITIDY_FPU_STR

#endif /* ITIDY_VERSION_INFO_H */
