#include <rex/runtime/guest/context.h>
#include <rex/runtime/guest/function.h>
#include <rex/kernel/kernel_state.h>
#include <rex/logging.h>
#include <cstring>
#include <set>
#include "generated/band3_init.h"
#include "config.h"

static std::set<size_t> g_consumed_args;

void ControllerHook(PPCRegister& r11) {
    r11.u64 = band3::GetConfig().controller_type;
}

void UpdateArkHook(PPCRegister& r4) {
	//REXLOG_INFO("Patching update ark path to game dir");
    r4.u64 = 0x82089B50;
}

//replace calls to 822703D0 (bad) with 822703A8 (good)
extern "C" PPC_FUNC(App__Run)
{
	REXLOG_INFO("Patching debugger trap");
	RunFunc_AppRunWithoutDebugging(ctx, base);
}

// OptionBool(char* optionName, bool default) - check host cmd line aargs for boolean options
// these do not needa value, just being on the command line implies they are true
extern "C" PPC_FUNC(OptionBool)
{
	const char* optionName = reinterpret_cast<const char*>(
		base + static_cast<uint32_t>(ctx.r3.u64));
	bool defaultVal = ctx.r4.u64 != 0;

	const auto& args = band3::GetArgs();
	for (size_t i = 0; i < args.size(); i++) {
		if (g_consumed_args.count(i)) continue;
		if (args[i] == optionName) {
			REXLOG_INFO("OptionBool(\"{}\") = {} (found)", optionName, !defaultVal);
			g_consumed_args.insert(i);
			ctx.r3.u64 = !defaultVal;
			return;
		}
	}

	ctx.r3.u64 = defaultVal;
}

// OptionStr(char* option, char* default) - check host cmd line args for string options
extern "C" PPC_FUNC(OptionStr)
{
	const char* option = reinterpret_cast<const char*>(
		base + static_cast<uint32_t>(ctx.r3.u64));
	uint32_t defaultPtr = static_cast<uint32_t>(ctx.r4.u64);

	REXLOG_INFO("Checking for string arg {}", option);
	const auto& args = band3::GetArgs();
	for (size_t i = 0; i < args.size(); i++) {
		if (g_consumed_args.count(i)) continue;
		if (args[i] == option && i + 1 < args.size()) {
			const std::string& value = args[i + 1];
			auto* mem = rex::kernel::kernel_memory();
			uint32_t len = static_cast<uint32_t>(value.size() + 1);
			uint32_t str_guest = mem->SystemHeapAlloc(len, 1);
			std::memcpy(base + str_guest, value.c_str(), len);
			REXLOG_INFO("OptionStr(\"{}\") = \"{}\" (found)", option, value);
			g_consumed_args.insert(i);
			g_consumed_args.insert(i + 1);
			ctx.r3.u64 = str_guest;
			return;
		}
	}

	ctx.r3.u64 = defaultPtr;
}

// Rnd::PreInit - override vsync from ini
extern "C" void __imp__Rnd__PreInit(PPCContext& ctx, uint8_t* base);
extern "C" PPC_FUNC(Rnd__PreInit)
{
	uint32_t rnd_this = static_cast<uint32_t>(ctx.r3.u64);
	__imp__Rnd__PreInit(ctx, base);

	long sync = band3::GetConfig().sync;
	if (sync >= 0) {
		// TODO:
		// get a proper Rnd structure instead of this pointer math
		auto* ptr = reinterpret_cast<rex::be<uint32_t>*>(base + rnd_this + 0xf0);
		*ptr = static_cast<uint32_t>(sync);
		REXLOG_INFO("Rnd::PreInit: sync set to {}", sync);
	}
}

// file checksum patch, just return true always
extern "C" PPC_FUNC(StreamChecksum__ValidateChecksum)
{
	ctx.r3.u64 = 1;
}
// file checksum patch, just return
extern "C" PPC_FUNC(PlatformMgr__SetDiskError)
{
	return;
}

// nuke metamusic calls, if enabled
extern "C" void __imp__MetaMusic__Load(PPCContext& ctx, uint8_t* base);
extern "C" PPC_FUNC(MetaMusic__Load)
{
    if (band3::GetConfig().disable_metamusic) return;
    __imp__MetaMusic__Load(ctx, base);
}
extern "C" void __imp__MetaMusic__Poll(PPCContext& ctx, uint8_t* base);
extern "C" PPC_FUNC(MetaMusic__Poll)
{
    if (band3::GetConfig().disable_metamusic) return;
    __imp__MetaMusic__Poll(ctx, base);
}
extern "C" void __imp__MetaMusic__Start(PPCContext& ctx, uint8_t* base);
extern "C" PPC_FUNC(MetaMusic__Start)
{
    if (band3::GetConfig().disable_metamusic) return;
    __imp__MetaMusic__Start(ctx, base);
}
extern "C" void __imp__MetaMusic__Loaded(PPCContext& ctx, uint8_t* base);
extern "C" PPC_FUNC(MetaMusic__Loaded)
{
    if (band3::GetConfig().disable_metamusic) {
        ctx.r3.u64 = 1;
        return;
    }
    __imp__MetaMusic__Loaded(ctx, base);
}

//Set venue from ini hook, reloads the entire config when setvenue is called, sure
//also doesnt clear allocations for previous strings. Bad? Maybe!
extern "C" void __imp__MetaPerformer__SetVenue(PPCContext& ctx, uint8_t* base);
extern "C" PPC_FUNC(MetaPerformer__SetVenue)
{
    band3::LoadConfig();
    const std::string& forced_venue = band3::GetConfig().forced_venue;

    static std::string cached_venue;
    static uint32_t str_guest = 0;

    if (!forced_venue.empty() && forced_venue != "false") {
        if (cached_venue != forced_venue) {
            cached_venue = forced_venue;
            auto* mem = rex::kernel::kernel_memory();
            str_guest = mem->SystemHeapAlloc(static_cast<uint32_t>(cached_venue.size() + 1), 1);
            std::memcpy(base + str_guest, cached_venue.c_str(), cached_venue.size() + 1);
            REXLOG_INFO("Forcing venue to \"{}\"", cached_venue);
        }
        ctx.r4.u64 = str_guest;
    }

    __imp__MetaPerformer__SetVenue(ctx, base);
}