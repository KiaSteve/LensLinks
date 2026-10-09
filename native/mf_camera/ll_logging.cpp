// ll_logging.cpp
// Copies std::cout / std::cerr into C:\ProgramData\LensLinks\log.txt with
// time, pid and tid, while keeping the normal console output.

#include <windows.h>
#include <share.h>

#include <cstdio>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

namespace
{
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
            CreateDirectoryW(L"C:\\ProgramData\\LensLinks", nullptr);

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

    LogRedirect g_logRedirect;
}
