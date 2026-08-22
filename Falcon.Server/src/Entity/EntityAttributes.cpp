#include "Entity/EntityAttributes.h"

#include <limits>

EntityAttributes::EntityAttributes() = default;

void EntityAttributes::_add(const std::string &name, float minimum, float maximum, float value) {
    AttributeData attribute;
    attribute.mName = name;
    attribute.mMinimum = minimum;
    attribute.mMaximum = maximum;
    attribute.mValue = value;
    attribute.mDefaultMinimum = minimum;
    attribute.mDefaultMaximum = maximum;
    attribute.mDefaultValue = value;
    mAttributes.push_back(attribute);
}

EntityAttributes EntityAttributes::createPlayerDefaults() {
    const float unbounded = std::numeric_limits<float>::max();

    EntityAttributes attributes;
    attributes._add("minecraft:health", 0.0f, 20.0f, 20.0f);
    attributes._add("minecraft:player.hunger", 0.0f, 20.0f, 20.0f);
    attributes._add("minecraft:player.saturation", 0.0f, 20.0f, 20.0f);
    attributes._add("minecraft:player.exhaustion", 0.0f, 5.0f, 0.0f);
    attributes._add("minecraft:player.level", 0.0f, 24791.0f, 0.0f);
    attributes._add("minecraft:player.experience", 0.0f, 1.0f, 0.0f);
    attributes._add("minecraft:movement", 0.0f, unbounded, 0.1f);
    attributes._add("minecraft:underwater_movement", 0.0f, unbounded, 0.02f);
    attributes._add("minecraft:lava_movement", 0.0f, unbounded, 0.02f);
    attributes._add("minecraft:knockback_resistance", 0.0f, 1.0f, 0.0f);
    attributes._add("minecraft:absorption", 0.0f, unbounded, 0.0f);
    return attributes;
}

void EntityAttributes::set(const std::string &name, float value) {
    for (AttributeData &attribute: mAttributes) {
        if (attribute.mName == name) {
            attribute.mValue = value;
            return;
        }
    }
}

float EntityAttributes::get(const std::string &name) const {
    for (const AttributeData &attribute: mAttributes) {
        if (attribute.mName == name)
            return attribute.mValue;
    }
    return 0.0f;
}

float EntityAttributes::getMinimum(const std::string &name) const {
    for (const AttributeData &attribute: mAttributes) {
        if (attribute.mName == name)
            return attribute.mMinimum;
    }
    return 0.0f;
}

float EntityAttributes::getMaximum(const std::string &name) const {
    for (const AttributeData &attribute: mAttributes) {
        if (attribute.mName == name)
            return attribute.mMaximum;
    }
    return 0.0f;
}

void EntityAttributes::setClamped(const std::string &name, float value) {
    for (AttributeData &attribute: mAttributes) {
        if (attribute.mName == name) {
            if (value < attribute.mMinimum)
                value = attribute.mMinimum;
            if (value > attribute.mMaximum)
                value = attribute.mMaximum;

            attribute.mValue = value;
            return;
        }
    }
}
