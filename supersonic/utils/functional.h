#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>

template <typename... V, template<typename ...> class C, typename Func>
auto map_container(const C<V...>& iterable, Func&& func)
{
    // Some convenience type definitions
    using value_type = decltype(func(*std::begin(iterable)));
    C<value_type> res;

    // Let std::transform apply `func` to all elements
    // (use perfect forwarding for the function object)
    for (auto& v: iterable) {
        res.emplace_back(func(v));
    }

    return res;
}
