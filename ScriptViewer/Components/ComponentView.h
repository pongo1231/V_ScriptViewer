#pragma once

#include "Component.h"

#include <memory>
#include <vector>

class ComponentView : public Component
{
  private:
	struct ComponentEntry
	{
		ComponentEntry(const char *const szComponentName, std::unique_ptr<Component> &&pComponent)
		    : m_szComponentName(szComponentName), m_pComponent(std::move(pComponent))
		{
		}

		const char *const m_szComponentName;
		std::unique_ptr<Component> m_pComponent;
	};

	std::vector<ComponentEntry> m_rgComponents;

  public:
	ComponentView();

	virtual void RunImGui() override;
};

inline ComponentView g_componentView;