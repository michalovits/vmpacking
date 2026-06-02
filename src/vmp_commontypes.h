#ifndef VMP_COMMONTYPES_H
#define VMP_COMMONTYPES_H

#include <iterator>
#include <memory>

namespace vmp
{

template <typename It, typename T>
concept UniquePtrIterator =
    std::input_iterator<It> && std::same_as<std::iter_value_t<It>, std::unique_ptr<T>>;

template <typename It, typename T>
concept ConstPtrIterator =
    std::input_iterator<It> && std::same_as<std::iter_value_t<It>, const T *>;

}  // namespace vmp

#endif  // VMP_COMMONTYPES_H
