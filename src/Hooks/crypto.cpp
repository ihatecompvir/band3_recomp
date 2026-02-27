#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/system/kernel_state.h>
#include <rex/logging.h>
#include <cstring>

// keyset
static const uint8_t NewKeyset[64] = {0x01, 0x22, 0x00, 0x38, 0xd2, 0x01, 0x78, 0x8b, 0xdd, 0xcd, 0xd0, 0xf0, 0xfe, 0x3e, 0x24, 0x7f, 0x51, 0x73, 0xad, 0xe5, 0xb3, 0x99, 0xb8, 0x61, 0x58, 0x1a, 0xf9, 0xb8, 0x1e, 0xa7, 0xbe, 0xbf, 0xc6, 0x22, 0x94, 0x30, 0xd8, 0x3c, 0x84, 0x14, 0x08, 0x73, 0x7c, 0xf2, 0x23, 0xf6, 0xeb, 0x5a, 0x02, 0x1a, 0x83, 0xf3, 0x97, 0xe9, 0xd4, 0xb8, 0x06, 0x74, 0x14, 0x6b, 0x30, 0x4c, 0x00, 0x91};

// guest addresses for crypto stuff
static uint32_t g_aes_state_guest = 0;
static uint32_t g_keyset_guest = 0;   
static uint32_t g_feed_guest = 0;     

extern "C" void __imp__XeCryptAesKey(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__XeCryptAesCbc(PPCContext& ctx, uint8_t* base);

static void EnsureCryptoBuffers(uint8_t* base) {
    if (g_aes_state_guest != 0) return;

	// allocate our heap memory inside the guest
    auto* mem = rex::system::kernel_memory();
    g_aes_state_guest = mem->SystemHeapAlloc(0x160, 16);
    g_keyset_guest = mem->SystemHeapAlloc(64, 16);
    g_feed_guest = mem->SystemHeapAlloc(0x10, 16);

    std::memset(base + g_aes_state_guest, 0, 0x160);
    std::memcpy(base + g_keyset_guest, NewKeyset, 64);
    std::memset(base + g_feed_guest, 0, 0x10);
}

// XeKeysSetKey(uint32_t key_index, void* key_buffer, uint32_t key_size)
extern "C" PPC_FUNC(XeKeysSetKey)
{
    EnsureCryptoBuffers(base);

    uint32_t key_index = static_cast<uint32_t>(ctx.r3.u64);
    uint32_t key_buffer_guest = static_cast<uint32_t>(ctx.r4.u64);

    uint32_t offset = key_buffer_guest - 0x82c76258;
    uint32_t our_key_guest = g_keyset_guest + offset;

    REXLOG_INFO("XeKeysSetKey: index=0x{:X}, key_buffer=0x{:08X}, offset=0x{:X}",
        key_index, key_buffer_guest, offset);

    ctx.r3.u64 = g_aes_state_guest;
    ctx.r4.u64 = our_key_guest;
    __imp__XeCryptAesKey(ctx, base);

    ctx.r3.u64 = 0;
}
extern "C" __attribute__((alias("XeKeysSetKey"))) void __imp__XeKeysSetKey(PPCContext& ctx, uint8_t* base);

// XeKeysAesCbc(uint32_t key_index, void* inp, uint32_t inp_size, void* out, void* feed, uint32_t encrypt)
extern "C" PPC_FUNC(XeKeysAesCbc)
{
    EnsureCryptoBuffers(base);

    uint32_t inp_guest = static_cast<uint32_t>(ctx.r4.u64);
    uint32_t inp_size = static_cast<uint32_t>(ctx.r5.u64);
    uint32_t out_guest = static_cast<uint32_t>(ctx.r6.u64);
    uint32_t feed_guest = static_cast<uint32_t>(ctx.r7.u64);
    uint32_t encrypt = static_cast<uint32_t>(ctx.r8.u64);

    if (feed_guest == 0) {
        std::memset(base + g_feed_guest, 0, 0x10);
        feed_guest = g_feed_guest;
    }

    REXLOG_INFO("XeKeysAesCbc: index=0x{:X}, size={}, encrypt={}, inp=0x{:08X}, out=0x{:08X}, feed=0x{:08X}",
        static_cast<uint32_t>(ctx.r3.u64), inp_size, encrypt, inp_guest, out_guest, feed_guest);

    ctx.r3.u64 = g_aes_state_guest;
    ctx.r4.u64 = inp_guest;
    ctx.r5.u64 = inp_size;
    ctx.r6.u64 = out_guest;
    ctx.r7.u64 = feed_guest;
    ctx.r8.u64 = encrypt;
    __imp__XeCryptAesCbc(ctx, base);

    ctx.r3.u64 = 0;
}
extern "C" __attribute__((alias("XeKeysAesCbc"))) void __imp__XeKeysAesCbc(PPCContext& ctx, uint8_t* base);
