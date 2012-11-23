#ifdef _WIN64
	#define WINDOWS
#elif _WIN32
	#define WINDOWS
#elif __linux
	#define LINUX
#elif __unix
	#define LINUX
#elif __posix
	#define LINUX
#endif