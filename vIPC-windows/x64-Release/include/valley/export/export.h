#ifndef LIBVALLEY_DETAILS_EXPORT_HXX
#define LIBVALLEY_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_STATIC)         // Using static.
#  define LIBVALLEY_EXPORT
#elif defined(LIBVALLEY_STATIC_BUILD) // Building static.
#  define LIBVALLEY_EXPORT
#elif defined(LIBVALLEY_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_EXPORT
#  endif
#elif defined(LIBVALLEY_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_EXPORT
#  endif
#else
#  define LIBVALLEY_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_DETAILS_EXPORT_HXX
