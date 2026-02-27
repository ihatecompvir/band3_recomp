#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/system/kernel_state.h>
#include <rex/logging.h>
#include <cstring>
#include <vector>
#include <string_view>
#include <set>
#include "generated/band3_init.h"
#include "config.h"
#include <random>
#include <cstdio>

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
			auto* mem = rex::system::kernel_memory();
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
    const std::string& forced = band3::GetConfig().forced_venue;

    if (forced.empty() || forced == "false") {
        __imp__MetaPerformer__SetVenue(ctx, base);
        return;
    }

    static const char* small_club[] = { "01","02","03","04","05","06","10","11","13","14","15" };
    static const char* big_club[]   = { "01","02","04","15","17" };
    static const char* arena[]      = { "01","04","06","07","10","11","12" };
    static const char* festival[]   = { "01","02" };
    static const char* video[]      = { "01","02","03","04","05","06","07" };

    static std::mt19937 rng{ std::random_device{}() };

    auto pick = [](const char* const* list, size_t n) -> const char* {
        std::uniform_int_distribution<size_t> dist(0, n - 1);
        return list[dist(rng)];
    };

    auto trim = [](std::string_view s) {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
        while (!s.empty() && (s.back()  == ' ' || s.back()  == '\t')) s.remove_suffix(1);
        return s;
    };

    std::string choice;
    {
        std::vector<std::string_view> items;
        std::string_view sv(forced);

        while (!sv.empty()) {
            size_t comma = sv.find(',');
            std::string_view part = (comma == std::string_view::npos) ? sv : sv.substr(0, comma);
            part = trim(part);
            if (!part.empty() && part != "false")
                items.push_back(part);
            if (comma == std::string_view::npos) break;
            sv.remove_prefix(comma + 1);
        }

        if (items.empty()) {
            __imp__MetaPerformer__SetVenue(ctx, base);
            return;
        }

        std::uniform_int_distribution<size_t> dist(0, items.size() - 1);
        choice.assign(items[dist(rng)]);
    }

    std::string resolved = choice;

    struct VenueGroup { const char* name; const char* const* list; size_t count; };
    static const VenueGroup groups[] = {
        { "small_club", small_club, std::size(small_club) },
        { "big_club",   big_club,   std::size(big_club)   },
        { "arena",      arena,      std::size(arena)      },
        { "festival",   festival,   std::size(festival)   },
        { "video",      video,      std::size(video)      },
    };

    for (const auto& g : groups) {
        if (choice == g.name) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%s_%s", g.name, pick(g.list, g.count));
            resolved = buf;
            break;
        }
    }

    static std::string cached;
    static uint32_t str_guest = 0;

    if (cached != resolved) {
        cached = resolved;
        auto* mem = rex::kernel::kernel_memory();
        str_guest = mem->SystemHeapAlloc(static_cast<uint32_t>(cached.size() + 1), 1);
        std::memcpy(base + str_guest, cached.c_str(), cached.size() + 1);
        REXLOG_INFO("Forcing venue to \"{}\"", cached);
    }

    ctx.r4.u64 = str_guest;
    __imp__MetaPerformer__SetVenue(ctx, base);
}