﻿#include "Asset.hpp"

usagi::Asset::Asset(Element *parent, std::string name, boost::uuids::uuid uuid)
    : Element { parent, std::move(name) }
    , mUuid { uuid }
{
}
