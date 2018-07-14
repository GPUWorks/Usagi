﻿#pragma once

#include "ChildElementEvent.hpp"

namespace usagi
{
class ChildElementRemovedEvent : ChildElementEvent
{
public:
	explicit ChildElementRemovedEvent()
		: ChildElementEvent { nullptr }
	{
	}
};
}
