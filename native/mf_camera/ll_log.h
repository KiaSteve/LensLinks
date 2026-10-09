#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>

// Appends a timestamped line to C:\ProgramData\LensLinks\log.txt.
// Includes pid/tid so you can tell Frame Server (svchost) lines
// apart from your test process lines.
inline void LL_Log(const char* fmt, ...)
{
    CreateDirectoryW(L"C:\\ProgramData\\LensLinks", nullptr);

    FILE* f = nullptr;
    if (fopen_s(&f, "C:\\ProgramData\\LensLinks\\log.txt", "a") != 0 || !f)
    {
        return;
    }

    SYSTEMTIME t;
    GetLocalTime(&t);

    fprintf(f, "%02d:%02d:%02d.%03d pid=%lu tid=%lu ",
            t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
            GetCurrentProcessId(), GetCurrentThreadId());

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fputc('\n', f);
    fclose(f);
}