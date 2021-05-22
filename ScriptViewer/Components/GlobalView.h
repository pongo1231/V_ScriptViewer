#pragma once

#include "Component.h"

class GlobalView : public Component
{
public:
	GlobalView() : Component()
	{

	}

	GlobalView(const GlobalView&) = delete;

	GlobalView& operator=(const GlobalView&) = delete;

	GlobalView(GlobalView&& globalView) noexcept : Component(std::move(globalView))
	{

	}

	GlobalView& operator=(GlobalView&&) = delete;

	virtual void RunImGui() override;

	virtual void RunScript() override;
};