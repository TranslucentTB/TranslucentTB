#ifndef GET_CURRENT_TIME_UNDEFINED
# ifdef GetCurrentTime
#  pragma push_macro("GetCurrentTime")
#  undef GetCurrentTime
#  define GET_CURRENT_TIME_UNDEFINED
# else
#  error "GetCurrentTime is not defined"
# endif
#else
# error "GetCurrentTime has already been undefined"
#endif
