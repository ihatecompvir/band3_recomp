#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/ppc/memory.h>
#include <rex/logging.h>
#include <cstdint>
#include <cstring>
#include "src/config.h"
#include "src/Game/DataNode.h"
#include "src/Game/DataArray.h"

extern "C" void __imp__AddHeap(PPCContext& ctx, uint8_t* base);

extern "C" PPC_FUNC(AddHeap)
{
    uint32_t arr_addr = ctx.r5.u32;

	// look at the DataArray in r5 and determine which heap we are adding to override the size
	// TODO: this does not handle pools from mem.dta, so big_hunk is not changeable, would be nice if we could also adjust this in the future
    if (arr_addr) {
        auto* arr = reinterpret_cast<const band3::DataArray*>(PPC_RAW_ADDR(arr_addr));
        uint32_t nodes_addr = arr->mNodes;
        auto* first = reinterpret_cast<const band3::DataNode*>(PPC_RAW_ADDR(nodes_addr));
        if (first->type == band3::kDataSymbol) {
            const char* name = reinterpret_cast<const char*>(PPC_RAW_ADDR(first->value));
            auto& cfg = band3::GetConfig();
            if (cfg.main_heap_size > 0 && strcmp(name, "main") == 0) {
                REXLOG_INFO("Overriding main heap size: {:#x} -> {:#x}", ctx.r4.u32, cfg.main_heap_size);
                ctx.r4.u32 = static_cast<uint32_t>(cfg.main_heap_size);
            } else if (cfg.char_heap_size > 0 && strcmp(name, "char") == 0) {
                REXLOG_INFO("Overriding char heap size: {:#x} -> {:#x}", ctx.r4.u32, cfg.char_heap_size);
                ctx.r4.u32 = static_cast<uint32_t>(cfg.char_heap_size);
            }
        }
    }

    __imp__AddHeap(ctx, base);
}