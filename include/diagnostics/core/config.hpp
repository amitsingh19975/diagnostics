#ifndef AMT_DARK_DIAGNOSTICS_CORE_CONFIG_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_CONFIG_HPP

#if !defined(DARK_OS_ANDROID) && !defined(DARK_OS_IOS) && !defined(DARK_OS_WIN) && \
    !defined(DARK_OS_UNIX) && !defined(DARK_OS_MAC) && !defined(DARK_EMPSCRIPTEN)

    #ifdef __APPLE__
        #include <TargetConditionals.h>
    #endif

    #if defined(_WIN32) || defined(__SYMBIAN32__)
        #define DARK_OS_WIN
    #elif defined(ANDROID) || defined(__ANDROID__)
        #define DARK_OS_ANDROID
        #define DARK_OS_UNIX
    #elif defined(linux) || defined(__linux)
        #define DARK_OS_LINUX
        #define DARK_OS_UNIX
    #elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
          defined(__DragonFly__)
        #define DARK_OS_BSD
        #define DARK_OS_UNIX
    #elif defined(__Fuchsia__) || defined(__GLIBC__) || defined(__GNU__) || defined(__unix__)
        #define DARK_OS_UNIX
    #elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define DARK_OS_IOS
        #define DARK_OS_UNIX
    #else
        #define DARK_OS_MAC
        #define DARK_OS_UNIX
    #endif
#endif

#if defined(__MSC_VER)
    #define DARK_COMPILER_MSVC
#elif defined(__clang__)
    #define DARK_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
    #define DARK_COMPILER_GCC
#endif

#ifdef DARK_COMPILER_MSVC
    #define DARK_ALWAYS_INLINE __forceinline inline
#else
    #define DARK_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

#if defined(DARK_COMPILER_CLANG)
    #define DARK_ASSUME(expr) __builtin_assume(expr)
#elif defined(DARK_COMPILER_GCC)
    #define DARK_ASSUME(expr) if (expr) {} else { __builtin_unreachable(); }
#elif defined(DARK_COMPILER_MSVC) || defined(__ICC)
    #define DARK_ASSUME(expr) __assume(expr)
#endif

#if !defined(DARK_RESTRICT)
    #ifdef DARK_COMPILER_MSVC
        #define DARK_RESTRICT __restrict
    #else
        #define DARK_RESTRICT __restrict__
    #endif
#endif


namespace dark {
#ifndef DARK_DIAGNOSTICS_SIZE_TYPE
    using dsize_t = unsigned;
#else
    using usize_t = DARK_DIAGNOSTICS_SIZE_TYPE;
#endif
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CORE_CONFIG_HPP
