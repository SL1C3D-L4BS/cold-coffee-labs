#pragma once

#include <span>

namespace gw::core {

template <typename T, std::size_t Extent = std::dynamic_extent>
using Span = std::span<T, Extent>;

}  // namespace gw::core
