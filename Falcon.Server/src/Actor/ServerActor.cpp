#include "Actor/ServerActor.h"

#include "Actor/DynamicPropertyStore.h"

namespace {
    Tag floatList3(float x, float y, float z) {
        Tag list = Tag::ofList(Tag::Type::Float);
        list.addToList(Tag::ofFloat(x));
        list.addToList(Tag::ofFloat(y));
        list.addToList(Tag::ofFloat(z));
        return list;
    }

    float listValue(const Tag &data, const char *key, size_t index, float fallback) {
        const Tag *list = data.get(key);
        if (list == nullptr || !list->isList())
            return fallback;

        const std::vector<Tag> &values = list->getList();
        if (index >= values.size())
            return fallback;

        return values[index].asFloat();
    }
}

ServerActor::ServerActor(uint64_t runtimeId, const std::string &identifier)
        : Actor(runtimeId), mIdentifier(identifier) {}

int32_t ServerActor::getIntProperty(const std::string &name, int32_t fallback) const {
    const auto it = mIntProperties.find(name);
    return it == mIntProperties.end() ? fallback : it->second;
}

float ServerActor::getFloatProperty(const std::string &name, float fallback) const {
    const auto it = mFloatProperties.find(name);
    return it == mFloatProperties.end() ? fallback : it->second;
}

Tag ServerActor::saveNbt() const {
    Tag data = Tag::ofCompound();

    data.putString("identifier", mIdentifier);
    data.put("Pos", floatList3(mPosition.x, mPosition.y, mPosition.z));
    data.put("Rotation", floatList3(mRotation.x, mRotation.y, mRotation.z));
    data.put("Motion", floatList3(mMotion.x, mMotion.y, mMotion.z));

    data.putFloat("Health", mHealth);
    data.putFloat("MaxHealth", mMaxHealth);
    data.putString("NameTag", mNameTag);
    data.putLong("OwnerUniqueId", mOwnerUniqueId);

    Tag tags = Tag::ofList(Tag::Type::String);
    for (const std::string &tag: mTags)
        tags.addToList(Tag::ofString(tag));
    data.put("Tags", tags);

    Tag intProperties = Tag::ofCompound();
    for (const auto &entry: mIntProperties)
        intProperties.putInt(entry.first, entry.second);
    data.put("IntProperties", intProperties);

    Tag floatProperties = Tag::ofCompound();
    for (const auto &entry: mFloatProperties)
        floatProperties.putFloat(entry.first, entry.second);
    data.put("FloatProperties", floatProperties);

    data.put("DynamicProperties", serializeDynamicProperties(mDynamicProperties));

    return data;
}

void ServerActor::loadNbt(const Tag &data) {
    if (!data.isCompound())
        return;

    mPosition = Vector3f(listValue(data, "Pos", 0, mPosition.x),
                         listValue(data, "Pos", 1, mPosition.y),
                         listValue(data, "Pos", 2, mPosition.z));

    mRotation = Vector3f(listValue(data, "Rotation", 0, 0.0f),
                         listValue(data, "Rotation", 1, 0.0f),
                         listValue(data, "Rotation", 2, 0.0f));

    mMotion = Vector3f(listValue(data, "Motion", 0, 0.0f),
                       listValue(data, "Motion", 1, 0.0f),
                       listValue(data, "Motion", 2, 0.0f));

    mHealth = data.getFloat("Health", mHealth);
    mMaxHealth = data.getFloat("MaxHealth", mMaxHealth);
    mNameTag = data.getString("NameTag", mNameTag);
    mOwnerUniqueId = data.getLong("OwnerUniqueId", mOwnerUniqueId);

    const Tag *tags = data.get("Tags");
    if (tags != nullptr && tags->isList()) {
        for (const Tag &entry: tags->getList())
            mTags.insert(entry.asString());
    }

    const Tag *intProperties = data.get("IntProperties");
    if (intProperties != nullptr && intProperties->isCompound()) {
        for (const std::string &name: intProperties->getKeys())
            mIntProperties[name] = intProperties->getInt(name, 0);
    }

    const Tag *floatProperties = data.get("FloatProperties");
    if (floatProperties != nullptr && floatProperties->isCompound()) {
        for (const std::string &name: floatProperties->getKeys())
            mFloatProperties[name] = floatProperties->getFloat(name, 0.0f);
    }

    const Tag *dynamicProperties = data.get("DynamicProperties");
    if (dynamicProperties != nullptr)
        deserializeDynamicProperties(*dynamicProperties, mDynamicProperties);
}
