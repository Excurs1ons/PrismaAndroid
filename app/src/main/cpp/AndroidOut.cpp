#if defined(__ANDROID__) || defined(ANDROID)
#include "AndroidOut.h"

AndroidOut androidOut("AO");
std::ostream aout(&androidOut);
#endif