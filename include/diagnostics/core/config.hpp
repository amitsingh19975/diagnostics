#ifndef DARK_DIAGNOSTICS_CORE_CONFIG_HPP
#define DARK_DIAGNOSTICS_CORE_CONFIG_HPP

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    #define DARK_UNIX
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    #define DARK_WINDOWS
#endif

#if defined(_MSC_VER)
    #define DARK_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
    #define DARK_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#endif // DARK_DIAGNOSTICS_CORE_CONFIG_HPP
