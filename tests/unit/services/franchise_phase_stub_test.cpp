// tests/unit/services/franchise_phase_stub_test.cpp — Phase 26–29 seams (plan Track J).

#include <doctest/doctest.h>

#include "engine/ai_runtime/cook_ai_weights_path.hpp"
#include "engine/ai_runtime/inference_runtime_contract.hpp"
#include "engine/franchise/second_ip_preprod.hpp"
#include "engine/services/franchise/phase27_catalog.hpp"

TEST_CASE("franchise phase stubs — catalog + AI contract") {
    CHECK(static_cast<int>(gw::services::franchise::Phase27ServiceId::Count) == 7);
    gw::ai_runtime::InferenceRuntimeContract c{};
    CHECK(c.schema_version == 1u);
    CHECK(gw::ai_runtime::kCookAiWeightsRelPath[0] == 'a');
    CHECK_FALSE(gw::franchise::kSecondIpPreprodEnabled);
}
