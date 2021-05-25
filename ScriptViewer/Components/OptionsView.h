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

	virtual void RunImGui() override;
};