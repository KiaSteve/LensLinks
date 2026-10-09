#pragma once

#include <windows.h>

class LensLinksMediaStream;

class LensLinksTestAccess
{
public:
    virtual LensLinksMediaStream* GetLensLinksStream() = 0;

protected:
    virtual ~LensLinksTestAccess() = default;
};
