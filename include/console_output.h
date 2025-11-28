/**
 * console_output.h - Conditional Console Output for iTidy
 * 
 * This header provides macros for console output that can be conditionally
 * compiled. When ENABLE_CONSOLE is defined, printf statements are active
 * and a console window will open. When not defined, all console output
 * is disabled and no console window appears.
 * 
 * Usage:
 *   #include "console_output.h"
 *   
 *   CONSOLE_PRINTF("Processing: %s\n", path);
 *   CONSOLE_ERROR("Failed to open file\n");
 * 
 * Build:
 *   make CONSOLE=1          # Enable console output
 *   make                    # Release build (no console)
 * 
 * Author: Kerry Thompson
 * Date: November 2025
 */

#ifndef CONSOLE_OUTPUT_H
#define CONSOLE_OUTPUT_H

#include <stdio.h>

/*===========================================================================*/
/* Console Output Macros                                                     */
/*===========================================================================*/

#ifdef ENABLE_CONSOLE

    /*-----------------------------------------------------------------------*/
    /* Console output ENABLED - printf statements compile in                 */
    /*-----------------------------------------------------------------------*/
    
    /**
     * @brief General console output (status messages, info)
     * Use for non-critical informational output
     */
    #define CONSOLE_PRINTF(...) printf(__VA_ARGS__)
    
    /**
     * @brief Error output to console
     * Use for error messages that should be visible to user
     */
    #define CONSOLE_ERROR(...) printf(__VA_ARGS__)
    
    /**
     * @brief Warning output to console
     * Use for non-fatal warnings
     */
    #define CONSOLE_WARNING(...) printf(__VA_ARGS__)
    
    /**
     * @brief Status output to console (processing progress)
     * Use for operation progress messages
     */
    #define CONSOLE_STATUS(...) printf(__VA_ARGS__)
    
    /**
     * @brief Debug output to console (development only)
     * Only active when both ENABLE_CONSOLE and DEBUG are defined
     */
    #ifdef DEBUG
        #define CONSOLE_DEBUG(...) printf(__VA_ARGS__)
    #else
        #define CONSOLE_DEBUG(...) ((void)0)
    #endif
    
    /**
     * @brief Startup banner - only shown once at program start
     */
    #define CONSOLE_BANNER(...) printf(__VA_ARGS__)
    
    /**
     * @brief Separator line for visual output grouping
     */
    #define CONSOLE_SEPARATOR() printf("===============================================\n")

#else

    /*-----------------------------------------------------------------------*/
    /* Console output DISABLED - all macros become no-ops                    */
    /* This ensures no console window opens on Amiga                         */
    /*-----------------------------------------------------------------------*/
    
    #define CONSOLE_PRINTF(...)   ((void)0)
    #define CONSOLE_ERROR(...)    ((void)0)
    #define CONSOLE_WARNING(...)  ((void)0)
    #define CONSOLE_STATUS(...)   ((void)0)
    #define CONSOLE_DEBUG(...)    ((void)0)
    #define CONSOLE_BANNER(...)   ((void)0)
    #define CONSOLE_SEPARATOR()   ((void)0)

#endif /* ENABLE_CONSOLE */

/*===========================================================================*/
/* Conditional newline macro (useful for formatting)                         */
/*===========================================================================*/

#ifdef ENABLE_CONSOLE
    #define CONSOLE_NEWLINE() printf("\n")
#else
    #define CONSOLE_NEWLINE() ((void)0)
#endif

#endif /* CONSOLE_OUTPUT_H */
