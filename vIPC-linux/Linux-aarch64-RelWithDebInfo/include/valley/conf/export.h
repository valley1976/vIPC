#ifndef LIBVALLEY_CONF_DETAILS_EXPORT_HXX
#define LIBVALLEY_CONF_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_CONF_STATIC)         // Using static.
#  define LIBVALLEY_CONF_EXPORT
#elif defined(LIBVALLEY_CONF_STATIC_BUILD) // Building static.
#  define LIBVALLEY_CONF_EXPORT
#elif defined(LIBVALLEY_CONF_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_CONF_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_CONF_EXPORT
#  endif
#elif defined(LIBVALLEY_CONF_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_CONF_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_CONF_EXPORT
#  endif
#else
#  define LIBVALLEY_CONF_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_CONF_DETAILS_EXPORT_HXX
