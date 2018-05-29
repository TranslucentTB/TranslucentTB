#if defined(_CPICKER_DLL)
#define EXTERN __declspec(dllexport)
#else
#define EXTERN __declspec(dllimport)
#endif