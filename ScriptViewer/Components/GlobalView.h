#pragma once

#include "Component.h"

class GlobalView : public Component
{
  public:
	GlobalView() : Component()
	{
	}

	virtual void RunImGui() override;

	virtual void RunScript() override;
};