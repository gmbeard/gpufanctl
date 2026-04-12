#pragma once

#include "schedule/get_completion_signatures.hpp"
#include "schedule/get_env.hpp"
namespace schedule
{

template <typename T>
concept sender = true;
//= requires(T) { schedule::get_completion_signatures<T, empty_env_t>(); };

} // namespace schedule
