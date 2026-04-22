// engine/telemetry/dsar_exporter.cpp

#include "engine/telemetry/dsar_exporter.hpp"
#include "engine/persist/local_store.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace gw::telemetry {

bool dsar_export_to_dir(const std::filesystem::path& dir,
                        gw::persist::ILocalStore&   store,
                        std::string_view            user_id_hash_hex) {
    try {
        std::filesystem::create_directories(dir);
        std::vector<gw::persist::SlotRow> slots;
        (void)store.list_slots(slots);
        std::ostringstream json;
        json << "{\"schema_version\":1,\"user_id_hash\":\"" << user_id_hash_hex << "\",\"slots\":[";
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if (i) json << ',';
            json << "{\"slot_id\":" << slots[i].slot_id << ",\"path\":\"" << slots[i].path << "\"}";
        }
        json << "],\"telemetry_queue_rows\":" << store.telemetry_pending_count() << "}";
        std::ofstream f(dir / "dsar_manifest.json", std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << json.str();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace gw::telemetry
