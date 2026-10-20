#ifndef LIBVALLEY_SERVE_DETAILS_EXPORT_HXX
#define LIBVALLEY_SERVE_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_SERVE_STATIC)         // Using static.
#  define LIBVALLEY_SERVE_EXPORT
#elif defined(LIBVALLEY_SERVE_STATIC_BUILD) // Building static.
#  define LIBVALLEY_SERVE_EXPORT
#elif defined(LIBVALLEY_SERVE_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_SERVE_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_SERVE_EXPORT
#  endif
#elif defined(LIBVALLEY_SERVE_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_SERVE_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_SERVE_EXPORT
#  endif
#else
#  define LIBVALLEY_SERVE_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_SERVE_DETAILS_EXPORT_HXX
