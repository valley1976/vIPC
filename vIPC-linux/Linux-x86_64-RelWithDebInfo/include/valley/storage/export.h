#ifndef LIBVALLEY_STORAGE_DETAILS_EXPORT_HXX
#define LIBVALLEY_STORAGE_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_STORAGE_STATIC)         // Using static.
#  define LIBVALLEY_STORAGE_EXPORT
#elif defined(LIBVALLEY_STORAGE_STATIC_BUILD) // Building static.
#  define LIBVALLEY_STORAGE_EXPORT
#elif defined(LIBVALLEY_STORAGE_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_STORAGE_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_STORAGE_EXPORT
#  endif
#elif defined(LIBVALLEY_STORAGE_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_STORAGE_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_STORAGE_EXPORT
#  endif
#else
#  define LIBVALLEY_STORAGE_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_STORAGE_DETAILS_EXPORT_HXX
