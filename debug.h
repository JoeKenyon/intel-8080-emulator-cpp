#pragma once
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>

#define EMU_DEBUG_ENABLE

// ============================================================
// debug.h - optional tracing, zero cost when disabled
//
// To turn on: compile with -DEMU_DEBUG_ENABLE
// To log to a file: call DBG_TO_FILE("trace.log") at startup
// To filter noise: call DBG_SET_LEVEL(INFO) to skip TRACE lines
//
// When EMU_DEBUG_ENABLE is NOT defined, every macro below
// becomes literally nothing - the compiler strips them out
// completely. No performance cost at all.
// ============================================================

class DebugLog
{
public:
    // TRACE = every single instruction (very spammy)
    // INFO  = major events only (resets, interrupts, etc.)
    enum class Level { TRACE, INFO };

    // Singleton - one global log, grab it with DebugLog::get()
    static DebugLog& get()
    {
        static DebugLog instance;
        return instance;
    }

    void setLevel(Level l) { min_level = l; }

    void toFile(const std::string& path)
    {
        file.open(path);
        use_file = true;
    }

    // Dumps a full CPU state line - one per instruction at TRACE level
    // Looks like: PC=0104 OP=3E [MVI A    ] A=00 B=00 ... F=[Z0 S0 P0 C0 AC0]
    void trace(uint16_t pc, uint8_t opcode, const char* mnemonic,
               uint8_t a, uint8_t b, uint8_t c,
               uint8_t d, uint8_t e, uint8_t h, uint8_t l,
               uint16_t sp,
               uint8_t fZ, uint8_t fS, uint8_t fP, uint8_t fC, uint8_t fAC)
    {
        if (min_level > Level::TRACE) return;

        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0')
           << "PC=" << std::setw(4) << pc
           << " OP=" << std::setw(2) << (int)opcode
           << " [" << std::left << std::setw(10) << mnemonic << "]"
           << std::right
           << " A="  << std::setw(2) << (int)a
           << " B="  << std::setw(2) << (int)b
           << " C="  << std::setw(2) << (int)c
           << " D="  << std::setw(2) << (int)d
           << " E="  << std::setw(2) << (int)e
           << " H="  << std::setw(2) << (int)h
           << " L="  << std::setw(2) << (int)l
           << " SP=" << std::setw(4) << sp
           << " F=[Z"  << (int)fZ
           <<   " S"   << (int)fS
           <<   " P"   << (int)fP
           <<   " C"   << (int)fC
           <<   " AC"  << (int)fAC << "]";

        write(ss.str());
    }

    // For logging events that aren't per-instruction spam
    void info(const std::string& msg) { if (min_level <= Level::INFO) write("[INFO] " + msg); }

private:
    DebugLog() : min_level(Level::INFO), use_file(false) {}

    void write(const std::string& line)
    {
        if (use_file && file.is_open()) file << line << "\n";
        else                            std::cout << line << "\n";
    }

    Level         min_level;
    bool          use_file;
    std::ofstream file;
};

// ============================================================
// The macros - these are what you actually use in the code
// ============================================================

#ifdef EMU_DEBUG_ENABLE

    // Drop a full register dump for the current instruction
    #define DBG_TRACE(pc, op, mn, a,b,c,d,e,h,l,sp, fZ,fS,fP,fC,fAC) \
        DebugLog::get().trace(pc, op, mn, a,b,c,d,e,h,l,sp, fZ,fS,fP,fC,fAC)

    // Log a plain text event (interrupts, resets, etc.)
    #define DBG_INFO(msg)  DebugLog::get().info(msg)

    // Change the minimum log level (TRACE or INFO)
    #define DBG_SET_LEVEL(lvl) DebugLog::get().setLevel(DebugLog::Level::lvl)

    // Redirect output to a file
    #define DBG_TO_FILE(path) DebugLog::get().toFile(path)

#else
    // Not in debug mode - everything becomes a no-op
    #define DBG_TRACE(...)
    #define DBG_INFO(msg)
    #define DBG_SET_LEVEL(lvl)
    #define DBG_TO_FILE(path)
#endif