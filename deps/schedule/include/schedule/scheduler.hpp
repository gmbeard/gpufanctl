#pragma once

namespace schedule
{

template <typename T>
concept scheduler = requires(T& t) { t.schedule(); };

} // namespace schedule
