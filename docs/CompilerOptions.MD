VBCC Compiler Options for AmigaOS (68k) Projects

When cross-compiling Amiga Workbench applications with VBCC, you can use various compiler flags in your Makefile to optimize performance or reduce the size of the generated 68000 binaries. Below is a breakdown of useful VBCC compiler options and how to apply them for smaller, optimized executables.

CPU and FPU Target Options

Target CPU (-cpu=) – Specify the 68k CPU model to optimize for a particular processor. For example, use -cpu=68000 for maximum compatibility (the default is 68000)
cdn.hackaday.io
. You can experiment with higher CPUs like -cpu=68020 or -cpu=68040 if you have a faster Amiga or emulator, as VBCC will then use instructions available on those CPUs for potentially faster code. Keep in mind that using a higher CPU target will produce code that won’t run on a plain 68000.

Floating Point Unit (-fpu=) – If your program uses floating-point math, you can indicate the FPU type. For example, -fpu=68881 targets the 68881/68882 math co-processor (or the 68040/060’s built-in FPU)
cdn.hackaday.io
. If you don’t have an FPU (typical for 68000/68020 machines) or want to avoid FPU instructions, you can use the default (which is essentially no hardware FPU, i.e. software emulation). This ensures the compiler avoids generating FPU instructions that would make the program crash on systems without one.

Optimization Flags for Speed and Size

Optimization Levels (-O) – VBCC supports different optimization levels similar to GCC. Using these can both speed up your code and often reduce its size by eliminating inefficient code patterns. Key levels include:

-O0 – No optimizations (useful for debugging, but results in larger, slower code).

-O (or -O1) – Enables a basic set of optimizations. This often yields the smallest code size while improving speed slightly
cdn.hackaday.io
.

-O2 – Activates most optimizations (a more aggressive level)
cdn.hackaday.io
. This can make your program faster, though it may sometimes increase code size by unrolling loops or other transformations.

-O3 – Activates all optimizations
cdn.hackaday.io
. VBCC’s -O3 includes more aggressive inlining and loop optimizations, which can increase performance but might also bloat the code. Use this if you need maximum speed and don’t mind a larger binary.

Tip: In practice, moderate optimization often gives the smallest binaries. For instance, many developers find -O1 or -O2 produces smaller 68k code than -O3 because the highest level may unroll loops or inline functions extensively (increasing code size). It’s worth trying multiple levels to see the trade-offs. VBCC even provides a level -O4 for link-time whole-program optimization, but that is more advanced and not usually needed for everyday use
cdn.hackaday.io
.

Optimize for Speed or Size (-speed / -size) – By default, VBCC balances speed and size, but you can explicitly tune this balance. Using -speed will let the optimizer favor faster code even if it becomes larger
cdn.hackaday.io
. Conversely, -size will prioritize a smaller code footprint, even if that means some loss of execution speed
cdn.hackaday.io
. For example, -size can prevent certain loop unrolling or large inlining that -O2/-O3 might do, thereby keeping the binary smaller
cdn.hackaday.io
. Use one of these flags (not both) if you have a clear goal to emphasize speed or compact size. If you want both better speed and reasonable size, a good approach is -O2 -size (which enables most optimizations but avoids bloating code unnecessarily).

Eliminate Unused Code (-final) – This option is useful at link time when you compile multiple source files. -final tells VBCC that this is the final build/link stage and it can remove any functions or data that are never referenced by the program
cdn.hackaday.io
. This is a form of dead code elimination across modules. It can shrink the binary by stripping out unused library code or unused functions in your project. To use it, add -final when linking your objects into the final executable (it has an effect only with higher optimization levels and during the link stage). For example: vc -O2 -size -final *.c -o MyProg.

Debug Symbols and Stripping for Smaller Binaries

Debug Info (-g) – Compile without debug information for smaller binaries. Unless you need to run a debugger on your program, make sure you do not use the -g flag in your release build. The -g option tells VBCC to include debug symbols (source line info) in the output
cdn.hackaday.io
, which can significantly increase the executable size. By default, if you omit -g, VBCC will not include debug information. In fact, some VBCC setups automatically strip out any debug data when you build without -g
ada.untergrund.net
. So for the smallest binary, use a "Release" configuration (no -g and with optimizations enabled).

Stripping Symbols – Even without -g, the binary might still contain symbol tables or other metadata. After linking your program, you can run a strip tool to remove any remaining symbols and relocation data not needed for execution. This can further shrink the binary. For example, the Amiga OS 4 development example below shows the effect of using the strip command on an executable – the file size drops from about 5504 bytes to 5360 bytes after stripping
os4coding.net
. On classic Amiga (Hunk format) you would typically use the VBCC linker’s options or an Amiga-specific strip utility. If you’re using VBCC’s built-in linker (vlink), you can add the -s option at link time to strip all symbols from the output (or -S to strip just debug symbols) as it creates the final executable. This ensures the smallest possible Workbench binary without any extraneous info. (Note: Stripping symbols will make debugging impossible on that binary, so do this only for the final release build.)

Example Makefile Flags

Below is a sample of how you might set up your Makefile variables with these options:

# Compiler and target settings
VBCC = vc +amiga                # 'vc' is VBCC's compile driver for Amiga target
CFLAGS = -O2 -size -cpu=68000 -quiet  # optimize for size on 68000, silence banner

# Use -g for debug builds only; omit for releases
# CFLAGS += -g                  # (uncomment this for debugging builds)

LDFLAGS = -final                # eliminate unused code at link time
# (VBCC links against its runtime and AmigaOS libs by default, e.g., -lamiga etc.)

# Target rule
myprog: main.o gui.o util.o
    $(VBCC) $(CFLAGS) $^ $(LDFLAGS) -o $@ 

# (Optional) Post-link strip step for extra size reduction
    vlink -s -o $@ $@       # or use an AmigaOS tool to strip symbols


In this example, -O2 -size is used to optimize most parts of the code while keeping it compact, targeting the base 68000 CPU. The -final flag (on the link line) will strip out any unused functions across the object files. The commented -g can be enabled when you need a debug build. Finally, a manual call to vlink -s (if available in your setup) or another strip tool can be used to remove symbols from the binary. Adjust these flags as needed for your project (for instance, use -cpu=68020 if you plan to run on an A1200/68020 or higher and want to take advantage of those CPU features).

By fine-tuning these VBCC compiler options in your makefile, you should be able to significantly shrink your binary size (by removing debug info and unused code) and optimize performance (by selecting appropriate CPU target and optimization level) for your Amiga Workbench GUI application.
cdn.hackaday.io
cdn.hackaday.io

Sources: VBCC Manual and Amiga VBCC documentation
cdn.hackaday.io
cdn.hackaday.io
cdn.hackaday.io
ada.untergrund.net
 (see VBCC compiler manual and change-log notes for optimization and stripping features).