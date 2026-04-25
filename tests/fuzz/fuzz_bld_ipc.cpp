// tests/fuzz/fuzz_bld_ipc.cpp — structured binary input via `.gwseq` parse (editor/BLD
// message surface shares the same sequencing / reflection stack).

#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size) {
    std::vector<std::uint8_t> v(data, data + size);
    gw::seq::GwseqReader     r;
    (void)r.open_bytes(std::move(v));
    return 0;
}
