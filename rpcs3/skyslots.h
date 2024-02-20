#pragma once
#include <optional>
#include "util/types.hpp"
#include <iostream>

constexpr auto UI_SKY_NUM = 8;
static std::optional<std::tuple<u8, u16, u16>> sky_slots[UI_SKY_NUM] = {};