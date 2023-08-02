#pragma once

#include "Component.h"

#include <memory>
#include <vector>

class OptionsView : public Component
{
  public:
	OptionsView() : Component()
	{
	}

	virtual void RunImGui() override;
};