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

		ComponentEntry(const ComponentEntry&) = delete;

		ComponentEntry& operator=(const ComponentEntry&) = delete;

		ComponentEntry(ComponentEntry&& componentEntry) noexcept
			: ComponentEntry(componentEntry.m_szComponentName, std::forward<std::unique_ptr<Component>>(componentEntry.m_pComponent))
		{

		}

		ComponentEntry& operator=(ComponentEntry&&) = delete;

		const char* const m_szComponentName;
		std::unique_ptr<Component> m_pComponent;
	};

	std::vector<ComponentEntry> m_rgComponents;

public:
	ComponentView();

	ComponentView(ComponentView&) = delete;

	ComponentView& operator=(ComponentView&) = delete;

	ComponentView(ComponentView&& componentView) noexcept : Component(std::move(componentView))
	{

	}

	ComponentView& operator=(ComponentView&&) = delete;

	virtual void RunImGui() override;
};

inline ComponentView g_componentView;