// ll_logging.cpp
//
// Copies everything written to std::cout / std::cerr inside this DLL into
// C:\ProgramData\LensLinks\log.txt, prefixed with time, process id and
// thread id. The original console output is preserved, so test programs
// still print to their console.
//
// If log.txt is larger than kMaxLogBytes when the DLL loads, it is moved to
// log.old.txt (replacing any older copy) before a fresh log is started.

#include <windows.h>
#include <share.h>

#include <cstdio>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

namespace
{
    constexpr wchar_t kLogDir[] = L"C:\\ProgramData\\LensLinks";
    constexpr wchar_t kLogPath[] = L"C:\\ProgramData\\LensLinks\\log.txt";
    constexpr wchar_t kOldLogPath[] = L"C:\\ProgramData\\LensLinks\\log.old.txt";

    // Rotate when the log is bigger than this.
    constexpr ULONGLONG kMaxLogBytes = 2ull * 1024ull * 1024ull;

    // Moves an oversized log aside. If another process has the file open the
    // move fails, and that is fine: we simply keep appending to it.
    void rotateLogIfNeeded()
    {
        WIN32_FILE_ATTRIBUTE_DATA info;

        if (!GetFileAttributesExW(kLogPath, GetFileExInfoStandard, &info))
        {
            return; // no log yet
        }

        const ULONGLONG size =
            (static_cast<ULONGLONG>(info.nFileSizeHigh) << 32) |
            info.nFileSizeLow;

        if (size <= kMaxLogBytes)
        {
            return;
        }

        MoveFileExW(kLogPath, kOldLogPath, MOVEFILE_REPLACE_EXISTING);
    }

    std::mutex& logMutex()
    {
        static std::mutex m;
        return m;
    }

    class TeeBuf : public std::streambuf
    {
    public:
        TeeBuf(std::streambuf* original, FILE* file)
            : original_(original), file_(file)
        {
        }

    protected:
        int_type overflow(int_type ch) override
        {
            if (traits_type::eq_int_type(ch, traits_type::eof()))
            {
                return traits_type::not_eof(ch);
            }

            const char c = traits_type::to_char_type(ch);

            // Keep the normal console output (ignore failures: a service
            // has no console and that must not break file logging).
            if (original_ != nullptr)
            {
                original_->sputc(c);
            }

            std::lock_guard<std::mutex> lock(logMutex());

            if (c != '\n')
            {
                line_.push_back(c);
                return ch;
            }

            SYSTEMTIME t;
            GetLocalTime(&t);

            char prefix[96];
            sprintf_s(
                prefix,
                "%02d:%02d:%02d.%03d pid=%lu tid=%lu ",
                t.wHour,
                t.wMinute,
                t.wSecond,
                t.wMilliseconds,
                GetCurrentProcessId(),
                GetCurrentThreadId());

            // One write per line so lines from different processes
            // are less likely to interleave.
            std::string out = prefix;
            out += line_;
            out += '\n';

            fwrite(out.data(), 1, out.size(), file_);
            fflush(file_);

            line_.clear();

            return ch;
        }

        int sync() override
        {
            if (original_ != nullptr)
            {
                original_->pubsync();
            }

            return 0;
        }

    private:
        std::streambuf* original_ = nullptr;
        FILE* file_ = nullptr;
        std::string line_;
    };

    struct LogRedirect
    {
        FILE* file = nullptr;
        TeeBuf* out = nullptr;
        TeeBuf* err = nullptr;
        std::streambuf* oldOut = nullptr;
        std::streambuf* oldErr = nullptr;

        LogRedirect()
        {
            CreateDirectoryW(kLogDir, nullptr);

            rotateLogIfNeeded();

            // _SH_DENYNO lets several processes (your test app and the
            // Frame Server svchost) append to the same file.
            file = _fsopen(
                "C:\\ProgramData\\LensLinks\\log.txt",
                "a",
                _SH_DENYNO);

            if (file == nullptr)
            {
                return;
            }

            out = new TeeBuf(std::cout.rdbuf(), file);
            err = new TeeBuf(std::cerr.rdbuf(), file);

            oldOut = std::cout.rdbuf(out);
            oldErr = std::cerr.rdbuf(err);

            std::cout << "[LensLinksLog] logging started" << std::endl;
        }

        ~LogRedirect()
        {
            if (file == nullptr)
            {
                return;
            }

            std::cout.rdbuf(oldOut);
            std::cerr.rdbuf(oldErr);

            delete out;
            delete err;

            fclose(file);
        }
    };

    // Constructed when the DLL is loaded, destroyed when it is unloaded.
    LogRedirect g_logRedirect;
}
