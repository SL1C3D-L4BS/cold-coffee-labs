#pragma once

#include <array>

namespace gw {
namespace core {

template <typename T, std::size_t N>
using Array = std::array<T, N>;

}  // namespace core
}  // namespace gw
