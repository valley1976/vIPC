#ifndef LIBVALLEY_BASE_DETAILS_EXPORT_HXX
#define LIBVALLEY_BASE_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_BASE_STATIC)         // Using static.
#  define LIBVALLEY_BASE_EXPORT
#elif defined(LIBVALLEY_BASE_STATIC_BUILD) // Building static.
#  define LIBVALLEY_BASE_EXPORT
#elif defined(LIBVALLEY_BASE_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_BASE_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_BASE_EXPORT
#  endif
#elif defined(LIBVALLEY_BASE_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_BASE_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_BASE_EXPORT
#  endif
#else
#  define LIBVALLEY_BASE_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_BASE_DETAILS_EXPORT_HXX
