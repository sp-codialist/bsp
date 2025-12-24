#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef UNITY_UNIT_TESTS
    #ifndef FORCE_STATIC
        #define FORCE_STATIC
    #endif
#else
    #ifndef FORCE_STATIC
        #define FORCE_STATIC static
    #endif
#endif
#ifdef __cplusplus
}
#endif
