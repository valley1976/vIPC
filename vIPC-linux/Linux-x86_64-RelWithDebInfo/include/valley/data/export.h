#ifndef LIBVALLEY_DATA_DETAILS_EXPORT_HXX
#define LIBVALLEY_DATA_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_DATA_STATIC)         // Using static.
#  define LIBVALLEY_DATA_EXPORT
#elif defined(LIBVALLEY_DATA_STATIC_BUILD) // Building static.
#  define LIBVALLEY_DATA_EXPORT
#elif defined(LIBVALLEY_DATA_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_DATA_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_DATA_EXPORT
#  endif
#elif defined(LIBVALLEY_DATA_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_DATA_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_DATA_EXPORT
#  endif
#else
#  define LIBVALLEY_DATA_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_DATA_DETAILS_EXPORT_HXX
