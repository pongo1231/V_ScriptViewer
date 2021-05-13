#pragma once

#include "Component.h"

#include <vector>
#include <memory>

class ComponentView : public Component
{
private:
	struct ComponentEntry
	{
		ComponentEntry(const char* const szComponentName, std::unique_ptr<Component>&& pComponent)
			: m_szComponentName(szComponentName), m_pComponent(std::move(pComponent))
		{

		}

		ComponentEntry(ComponentEntry&) = delete;

		Component& operator=(Component&) = delete;

		ComponentEntry(ComponentEntry&& componentEntry) noexcept
			: ComponentEntry(componentEntry.m_szComponentName, std::forward<std::unique_ptr<Component>>(componentEntry.m_pComponent))
		{

		}

		Component& operator=(Component&&) = delete;

		const char* const m_szComponentName;
		std::unique_ptr<Component> m_pComponent;
	};

	std::vector<ComponentEntry> m_rgComponents;

public:
	ComponentView();

	virtual void RunImGui() override;
};

inline ComponentView g_componentView;