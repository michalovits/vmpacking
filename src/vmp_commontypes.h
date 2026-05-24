#ifndef VMP_ITERATORS_H
#define VMP_ITERATORS_H

#include <vmp_treeinstance.h>

#include <vmp_clustertreeinstance.h>
#include <vmp_generalinstance.h>

#include <memory>

namespace vmp
{

template <typename It, typename T>
concept SharedPtrIterator =
    std::input_iterator<It> && std::same_as<std::iter_value_t<It>, std::shared_ptr<T>>;

template <typename T>
concept Instance = std::same_as<T, GeneralInstance> || std::same_as<T, ClusterTreeInstance> ||
                   std::same_as<T, TreeInstance>;

}  // namespace vmp

#endif  // VMP_ITERATORS_H
