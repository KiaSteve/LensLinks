 #include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include <iomanip>

#pragma comment(lib, "dbghelp.lib")

int main()
{
    const char* dll =
        R"(C:\Users\Administrator\lenslinks\native_build\Debug\LensLinksNative.dll)";

    const DWORD64 crashRva = 0xF2C1;

    HANDLE process = GetCurrentProcess();

    SymSetOptions(
        SYMOPT_LOAD_LINES |
        SYMOPT_UNDNAME |
        SYMOPT_DEFERRED_LOADS
    );

    if (!SymInitialize(
            process,
            R"(C:\Users\Administrator\lenslinks\native_build\Debug)",
            FALSE))
    {
        std::cerr << "SymInitialize failed: "
                  << GetLastError() << "\n";
        return 1;
    }

    DWORD64 base = SymLoadModuleEx(
        process,
        nullptr,
        dll,
        nullptr,
        0,
        0,
        nullptr,
        0
    );

    std::cout << "Loaded module base: 0x"
              << std::hex << base << "\n";

    if (!base)
    {
        std::cerr << "SymLoadModuleEx failed: "
                  << GetLastError() << "\n";

        SymCleanup(process);
        return 1;
    }

    DWORD64 address = base + crashRva;

    std::cout << "Crash RVA: 0x"
              << std::hex << crashRva << "\n";

    std::cout << "Calculated address: 0x"
              << std::hex << address << "\n";

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    ZeroMemory(buffer, sizeof(buffer));

    PSYMBOL_INFO symbol =
        reinterpret_cast<PSYMBOL_INFO>(buffer);

    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;

    if (SymFromAddr(
            process,
            address,
            &displacement,
            symbol))
    {
        std::cout << "\n=== SYMBOL ===\n";

        std::cout << "Name: "
                  << symbol->Name << "\n";

        std::cout << "Address: 0x"
                  << std::hex
                  << symbol->Address << "\n";

        std::cout << "Displacement: 0x"
                  << std::hex
                  << displacement << "\n";
    }
    else
    {
        std::cerr << "\nSymFromAddr failed: "
                  << GetLastError() << "\n";
    }

    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));

    line.SizeOfStruct = sizeof(line);

    DWORD lineDisplacement = 0;

    if (SymGetLineFromAddr64(
            process,
            address,
            &lineDisplacement,
            &line))
    {
        std::cout << "\n=== SOURCE ===\n";

        std::cout << "File: "
                  << line.FileName << "\n";

        std::cout << "Line: "
                  << std::dec
                  << line.LineNumber << "\n";

        std::cout << "Displacement: 0x"
                  << std::hex
                  << lineDisplacement << "\n";
    }
    else
    {
        std::cerr << "\nSymGetLineFromAddr64 failed: "
                  << GetLastError() << "\n";
    }

    SymUnloadModule64(process, base);
    SymCleanup(process);

    return 0;
}