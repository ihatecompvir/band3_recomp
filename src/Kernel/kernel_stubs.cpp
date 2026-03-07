#include <rex/ppc/function.h>
#include <rex/logging.h>
#define STUB_KERNEL(x) __attribute__((alias("__imp__" #x))) PPC_FUNC(x); PPC_FUNC_IMPL(__imp__##x) { REXLOG_INFO("{} stub!", __FUNCTION__); return; }

// Stubs for imported functions not provided by rex
// upd: rex ended up stubbing them, leaving one for reference
//STUB_KERNEL(ExAllocatePoolWithTag)