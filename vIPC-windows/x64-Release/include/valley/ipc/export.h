#ifndef LIBVALLEY_IPC_DETAILS_EXPORT_HXX
#define LIBVALLEY_IPC_DETAILS_EXPORT_HXX

#if defined(LIBVALLEY_IPC_STATIC)         // Using static.
#  define LIBVALLEY_IPC_EXPORT
#elif defined(LIBVALLEY_IPC_STATIC_BUILD) // Building static.
#  define LIBVALLEY_IPC_EXPORT
#elif defined(LIBVALLEY_IPC_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBVALLEY_IPC_EXPORT __declspec(dllimport)
#  else
#    define LIBVALLEY_IPC_EXPORT
#  endif
#elif defined(LIBVALLEY_IPC_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBVALLEY_IPC_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_IPC_EXPORT
#  endif
#else
#  define LIBVALLEY_IPC_EXPORT            // Using static or shared.
#endif

#endif // LIBVALLEY_IPC_DETAILS_EXPORT_HXX
