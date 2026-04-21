// engine/net/desync_detector.cpp — Phase 14 Wave 14G (ADR-0051).

#include "engine/net/desync_detector.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace gw::net {

bool DesyncDetector::record(Tick tick, PeerId peer, std::uint64_t server_hash, std::uint64_t peer_hash) {
    DesyncSample s{};
    s.tick        = tick;
    s.peer        = peer;
    s.server_hash = server_hash;
    s.peer_hash   = peer_hash;
    s.divergent   = (server_hash != peer_hash);
    ring_.push_back(s);
    while (ring_.size() > window_) ring_.pop_front();
    if (s.divergent) {
        divergence_count_ += 1;
        if (auto_dump_) (void)dump(tick);
    }
    return s.divergent;
}

std::size_t DesyncDetector::dump(Tick tick) const {
    std::error_code ec;
    std::filesystem::create_directories(dump_dir_, ec);
    char name[64];
    std::snprintf(name, sizeof(name), "/desync_%010u.gwdesync", static_cast<unsigned>(tick));
    const std::string path = dump_dir_ + name;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return 0;
    // 'GDSY' magic + version.
    static constexpr char kMagic[4] = {'G', 'D', 'S', 'Y'};
    f.write(kMagic, 4);
    const std::uint32_t version = 1;
    f.write(reinterpret_cast<const char*>(&version), sizeof(version));
    const std::uint32_t count = static_cast<std::uint32_t>(ring_.size());
    f.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& s : ring_) {
        f.write(reinterpret_cast<const char*>(&s.tick), sizeof(s.tick));
        f.write(reinterpret_cast<const char*>(&s.peer.id), sizeof(s.peer.id));
        f.write(reinterpret_cast<const char*>(&s.server_hash), sizeof(s.server_hash));
        f.write(reinterpret_cast<const char*>(&s.peer_hash), sizeof(s.peer_hash));
        const std::uint8_t div = s.divergent ? 1 : 0;
        f.write(reinterpret_cast<const char*>(&div), sizeof(div));
    }
    f.flush();
    return static_cast<std::size_t>(f.tellp());
}

} // namespace gw::net
