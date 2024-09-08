#ifndef GPUFANCTL_SYMBOL_HPP_INCLUDED
#define GPUFANCTL_SYMBOL_HPP_INCLUDED

#include <dlfcn.h>
#include <stdexcept>

#define TRY_ATTACH_SYMBOL(target, name, lib)                                   \
    ::gfc::attach_symbol(target, name, lib)

namespace gfc
{
template <typename F>
auto attach_symbol(F** fn, char const* name, void* lib) -> void
{
    *fn = reinterpret_cast<F*>(dlsym(lib, name));
    if (!*fn)
        throw std::runtime_error { std::string { "Couldn't load symbol: " } +
                                   name };
}

} // namespace gfc

#endif // SHADOW_CAST_UTILS_SYMBOL_HPP_INCLUDED
