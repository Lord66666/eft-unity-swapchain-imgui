#include "VMTHook.h"

#include "CRT.hpp"
#include "xor.hpp"

#define Exit LI_FN(TerminateProcess)((HANDLE)-1, EXIT_SUCCESS)