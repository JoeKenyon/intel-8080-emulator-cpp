#pragma once

// runs the standard 8080 diagnostic rom suite (tst8080, cputest, 8080pre,
// 8080exm) against the cpu core. pops open a custom sdl window to print
// the terminal output instead of dumping it to stdout.
// expects the roms to live under ./roms/cpu_tests/.
void runCpuDiagnostics();
