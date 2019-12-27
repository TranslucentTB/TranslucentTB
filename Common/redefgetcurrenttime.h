#ifdef GET_CURRENT_TIME_UNDEFINED
# pragma pop_macro("GetCurrentTime")
# undef CURRENT_TIME_UNDEFINED
#else
# error "GetCurrentTime has not been undefined"
#endif
