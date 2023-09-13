#ifndef TopLevel_RTC_h
#define TopLevel_RTC_h

#include <stdio.h>

void RTC_Init();
void RTC_GetTime (char* Line, size_t MaxLength);
void RTC_GenDataFileName (const char* prefixStr, const char* kitNumStr, const size_t MaxLength, char* FName);

#endif  // TopLevel_RTC_h