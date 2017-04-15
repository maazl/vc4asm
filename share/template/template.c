#include "___HEADERNAME___"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
__declspec(align(8))
#elif defined(__GNUC__)
__attribute__((aligned(8)))
#endif
uint32_t ___SYMBOLNAME___[___INSTCOUNT2___] = {
___HEXDATA___};
#ifdef __HIGHC__
#pragma Align_to(8, ___SYMBOLNAME___)
#ifdef __cplusplus
}
#endif
#endif
