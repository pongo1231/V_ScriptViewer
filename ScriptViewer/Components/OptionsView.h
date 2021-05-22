#pragma once

#include "Component.h"

#include <vector>
#include <memory>

class OptionsView : public Component
{
public:
	OptionsView() : Component()
	{

	}

	OptionsView(OptionsView&) = delete;

	OptionsView& operator=(OptionsView&) = delete;

	OptionsView(OptionsView&& optionsView) noexcept : Component(std::move(optionsView))
	{

	}

	OptionsView& operator=(OptionsView&&) = delete;

	virtual void RunImGui() override;
};