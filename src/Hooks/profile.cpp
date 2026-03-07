#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/ppc/memory.h>
#include <rex/logging.h>
#include <cstring>
#include "src/config.h"

extern "C" void __imp__PlatformMgr__GetName(PPCContext& ctx, uint8_t* base);

extern "C" PPC_FUNC(PlatformMgr__GetName)
{
    __imp__PlatformMgr__GetName(ctx, base);

    auto& username = band3::GetConfig().username;
    if (!username.empty()) {
        char* buf = reinterpret_cast<char*>(base + ctx.r3.u32);
        size_t len = username.size();
        if (len > 32) len = 32;
        std::memcpy(buf, username.c_str(), len);
        buf[len] = '\0';
    }
}
