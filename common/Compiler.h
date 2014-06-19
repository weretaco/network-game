#if defined _WIN64
   #define WINDOWS
#elif defined _WIN32
   #define WINDOWS
#elif defined __linux
   #define LINUX
#elif defined __unix
   #define LINUX
#elif defined __posix
   #define LINUX
#elif defined __APPLE__
   #define MAC
#endif
