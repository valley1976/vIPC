#ifndef LIBVALLEY_FAILURE_DETAILS_EXPORT_HXX
#define LIBVALLEY_FAILURE_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_FAILURE_STATIC)         // Using static.
#  define LIBVALLEY_FAILURE_EXPORT
#elif defined(LIBVALLEY_FAILURE_STATIC_BUILD) // Building static.
#  define LIBVALLEY_FAILURE_EXPORT
#elif defined(LIBVALLEY_FAILURE_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_FAILURE_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_FAILURE_EXPORT
#  endif
#elif defined(LIBVALLEY_FAILURE_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_FAILURE_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_FAILURE_EXPORT
#  endif
#else
#  define LIBVALLEY_FAILURE_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_FAILURE_DETAILS_EXPORT_HXX
