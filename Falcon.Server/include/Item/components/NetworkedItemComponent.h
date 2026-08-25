#pragma once

#include "item/components/ItemComponent.h"

template <typename T>
class NetworkedItemComponent : public ItemComponent {
public:
    const char *getName() const final { return T::componentName(); }

    std::unique_ptr<ItemComponent> clone() const final {
        return std::make_unique<T>(static_cast<const T &>(*this));
    }
};
