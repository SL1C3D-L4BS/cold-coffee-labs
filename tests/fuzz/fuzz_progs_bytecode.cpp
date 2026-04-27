// tests/fuzz/fuzz_progs_bytecode.cpp — LibFuzzer for `.gwprogs` v1 verifier + run.

#include "engine/runtime/progs_vm.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size) {
    if (data == nullptr) {
        return 0;
    }
    const gw::runtime::progs::VerifyResult vr =
        gw::runtime::progs::verify_bytecode({reinterpret_cast<const std::byte*>(data), size});
    if (vr == gw::runtime::progs::VerifyResult::Ok) {
        (void)gw::runtime::progs::run_bytecode({reinterpret_cast<const std::byte*>(data), size},
                                                 4096u);
    }
    return 0;
}
