﻿#pragma once

#include <boost/uuid/uuid.hpp>

#include <Usagi/Engine/Core/Element.hpp>

#include "Asset.hpp"

namespace usagi
{
class AssetPath;

class AssetRoot : public Element
{
	bool acceptChild(Element *child) override;

    Asset * findAsset(std::string locator) const;
    Asset * findAssetByUuid(const boost::uuids::uuid &uuid) const;
    Asset * findAssetByString(std::string string) const;

public:
    explicit AssetRoot(Element *parent);

    template <typename AssetType, typename DerivedType = AssetType>
    std::shared_ptr<DerivedType> find(const std::string &locator)
    {
        if constexpr(std::is_same_v<AssetType, DerivedType>)
            return findAsset(locator)->as<AssetType>();
		else
            return findAsset(locator)->as<AssetType, DerivedType>();
    }
};
}