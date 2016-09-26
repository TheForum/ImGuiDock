#include "Editor.h"

namespace ImGuiDock
{
	Dockspace::Dockspace(GuiWindowList *windowListPtr, GuiWindow *owner) : m_windowListPtr(windowListPtr), m_owner(owner)
	{
	};

	Dockspace::~Dockspace()
	{
		clear();
	};

	bool Dockspace::dock(Dock *dock, DockSlot dockSlot, float size, bool active)
	{
		return dockWith(dock, nullptr, dockSlot, size, active);
	};

	bool Dockspace::dockWith(Dock *dock, Dock *dockTo, DockSlot dockSlot, float size, bool active)
	{
		if (dock == nullptr)
			return false;

		Container *currentContainer = &m_container;

		if (dockTo != nullptr)
		{
			if (dockSlot == ImGuiDock::DockSlot::Tab)
			{
				dockTo->container->activeDock = active ? dock : currentContainer->splits[0]->activeDock ? currentContainer->splits[0]->activeDock : dock;
				dockTo->container->docks.push_back(dock);
				dock->container = dockTo->container;
				return true;
			}
			else
			{
				m_containers.push_back(new Container{});
				auto newContainer = m_containers[m_containers.size() - 1];
				newContainer->parent = dockTo->container->parent;
				newContainer->splits[0] = dockTo->container;
				newContainer->size = dockTo->container->size;
				//if (size)
				//	newContainer->alwaysAutoResize = false;
				dockTo->container->size = 0;
				if (dockTo->container->parent->splits[0] == dockTo->container)
					dockTo->container->parent->splits[0] = newContainer;
				else dockTo->container->parent->splits[1] = newContainer;
				//dockTo->container->parent = newContainer;
				dockTo->container = newContainer->splits[0];
				dockTo->container->parent = newContainer;
				currentContainer = newContainer;
			}
		}

		Container *childContainer = nullptr;
		if (currentContainer->splits[0] == nullptr || currentContainer->splits[1] == nullptr)
		{
			m_containers.push_back(new Container{});
			childContainer = m_containers[m_containers.size() - 1];
		};

		if (currentContainer->splits[0] == nullptr)
		{
			currentContainer->splits[0] = childContainer;
			currentContainer->splits[0]->activeDock = active ? dock : currentContainer->splits[0]->activeDock ? currentContainer->splits[0]->activeDock : dock;
			currentContainer->splits[0]->docks.push_back(dock);
			currentContainer->splits[0]->parent = currentContainer;
			currentContainer->splits[0]->size = size < 0 ? size * -1 : size;
			dock->container = currentContainer->splits[0];
			dock->container->parent = currentContainer;
		}
		else if (currentContainer->splits[1] == nullptr)
		{
			currentContainer->splits[1] = childContainer;
			Container *otherSplit = currentContainer->splits[0];
			if (size > 0)
			{
				currentContainer->splits[0]->alwaysAutoResize = true;
				currentContainer->splits[0]->size = 0;
				currentContainer->splits[1]->size = size;
				currentContainer->splits[1]->alwaysAutoResize = false;
			}
			else if (size == 0) {}
			else
			{
				currentContainer->splits[0]->alwaysAutoResize = false;
				currentContainer->splits[0]->size = size * -1;
				currentContainer->splits[1]->size = 0;
				currentContainer->splits[1]->alwaysAutoResize = true;
			}
			switch (dockSlot)
			{
			case ImGuiDock::DockSlot::Left:
				currentContainer->splits[1] = currentContainer->splits[0];
				currentContainer->splits[0] = childContainer;
				currentContainer->verticalSplit = true;
				break;
			case ImGuiDock::DockSlot::Right:
				currentContainer->verticalSplit = true;
				break;
			case ImGuiDock::DockSlot::Top:
				currentContainer->splits[1] = currentContainer->splits[0];
				currentContainer->splits[0] = childContainer;
				currentContainer->verticalSplit = false;
				break;
			case ImGuiDock::DockSlot::Bottom:
				currentContainer->verticalSplit = false;
				break;
			case ImGuiDock::DockSlot::Tab:
				currentContainer->verticalSplit = false;
				break;
			case ImGuiDock::DockSlot::None:
				break;
			default:
				break;
			}
			childContainer->activeDock = active ? dock : childContainer->activeDock ? childContainer->activeDock : dock;
			childContainer->docks.push_back(dock);
			childContainer->parent = currentContainer;

			//	if (childContainer->parent != nullptr && currentContainer->verticalSplit != childContainer->parent->verticalSplit)
			//		currentContainer->size = otherSplit->size ? otherSplit->size + otherSplit->size : otherSplit->size;

			dock->container = childContainer;
		}
		else
		{
			return false;
		}

		return true;
	};

	bool Dockspace::undock(Dock *dock)
	{
		if (dock != nullptr)
		{
			if (dock->container->docks.size() > 1)
			{
				for (int i = 0; i < dock->container->docks.size(); i++)
				{
					if (dock->container->docks[i] == dock)
					{
						dock->lastSize = dock->container->activeDock->lastSize;
						dock->container->docks.erase(dock->container->docks.begin() + i);
						if (i != dock->container->docks.size())
							dock->container->activeDock = dock->container->docks[i];
						else dock->container->activeDock = dock->container->docks[i - 1];
					}
				}
			}
			else
			{
				Container *toDelete = nullptr, *parentToDelete = nullptr;
				if (dock->container->parent == &m_container)
				{
					if (m_container.splits[0] == dock->container)
					{
						if (m_container.splits[1])
						{
							toDelete = m_container.splits[0];
							if(m_container.splits[1]->splits[0]){
								parentToDelete = m_container.splits[1];
								m_container.splits[0] = m_container.splits[1]->splits[0];
								m_container.splits[0]->parent = &m_container;
								m_container.splits[0]->verticalSplit = false;
								m_container.splits[1] = m_container.splits[1]->splits[1];
								m_container.splits[1]->parent = &m_container;
								m_container.splits[1]->parent->verticalSplit = m_container.splits[1]->verticalSplit;
								m_container.splits[1]->verticalSplit = false;
							}
							else 
							{ 
								m_container.splits[0] = m_container.splits[1]; 
								m_container.splits[1] = nullptr;
								m_container.splits[0]->size = 0;
								m_container.splits[0]->verticalSplit = false;
								m_container.splits[0]->parent->verticalSplit = false;
							}
						}
						else return false;
					}
					else
					{
						toDelete = m_container.splits[1];
						m_container.splits[1] = nullptr;
					}
				}
				else
				{
					parentToDelete = dock->container->parent;
					if (dock->container->parent->splits[0] == dock->container)
					{
						toDelete = dock->container->parent->splits[0];
						Container *parent = dock->container->parent->parent;
						Container *working = nullptr;
						if (dock->container->parent->parent->splits[0] == dock->container->parent)
							working = dock->container->parent->parent->splits[0] = dock->container->parent->splits[1];
						else working = dock->container->parent->parent->splits[1] = dock->container->parent->splits[1];
						working->parent = parent;
						working->size =  dock->container->parent->size;
					}
					else
					{
						toDelete = dock->container->parent->splits[1];
						Container *parent = dock->container->parent->parent;
						Container *working = nullptr;
						if (dock->container->parent->parent->splits[0] == dock->container->parent)
							working = dock->container->parent->parent->splits[0] = dock->container->parent->splits[0];
						else working = dock->container->parent->parent->splits[1] = dock->container->parent->splits[0];
						working->parent = parent;
						working->size = dock->container->parent->size;
					}
				}
				for (int i = 0; i < m_containers.size(); i++)
				{
					if (toDelete == m_containers[i]) { delete m_containers[i]; m_containers.erase(m_containers.begin() + i); }
					if(m_containers.size() > 1 && parentToDelete == m_containers[i]) { delete m_containers[i]; m_containers.erase(m_containers.begin() + i); }
					if (m_containers.size() > 1 && toDelete == m_containers[i]) { delete m_containers[i]; m_containers.erase(m_containers.begin() + i); }
				}
			}
			return true;
		}
		return false;
	}

	void Dockspace::updateAndDraw(ImVec2 dockspaceSize)
	{
		//m_container.size = size;

		uint32_t idgen = 0;

		float tabbarHeight = 20;

		std::function<void(Container*, ImVec2, ImVec2)> renderContainer = [&](Container *container, ImVec2 size, ImVec2 cursorPos) {
			ImVec2 calculatedSize = size;
			ImVec2 calculatedCursorPos = cursorPos;

			idgen++;

			std::string idname = "Dock##";
			idname += idgen;

			calculatedSize.y -= tabbarHeight;

			float splitterButtonWidth = 4;
			float splitterButtonWidthHalf = splitterButtonWidth / 2;

			if (container->splits[0] == nullptr && container != &m_container)
			{
				_renderTabBar(container, calculatedSize, cursorPos);
				cursorPos.y += tabbarHeight;
				
				ImGui::SetCursorPos(cursorPos);
				ImVec2 screenCursorPos = ImGui::GetCursorScreenPos();
				screenCursorPos.y -= tabbarHeight;

				ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(.25, .25, .25, 1));
				ImGui::BeginChild(idname.c_str(), calculatedSize, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
				container->activeDock->drawFunction(calculatedSize);
				container->activeDock->lastSize = calculatedSize;
				ImGui::EndChild();

				GuiWindow *draggedWindow = _isAnyWindowDragging();
				if (draggedWindow != nullptr && draggedWindow->getDockspace()->m_container.splits[0]->activeDock != container->activeDock)
				{
					int mousex, mousey,wposx,wposy;
					m_owner->getMouseScreenPosition(mousex, mousey);
					m_owner->getWindowPosition(wposx, wposy);
					mousex -= wposx;
					mousey -= wposy;
					if ((mousex > screenCursorPos.x && mousex < (screenCursorPos.x+size.x)) &&
						(mousey > screenCursorPos.y && mousey < (screenCursorPos.y + size.y)))
					{
						ImGui::BeginChild("##dockSlotPreview");
						ImGui::PushClipRect(ImVec2(),ImGui::GetIO().DisplaySize, false);
						DockSlot dockSlot = _renderDockSlotPreview(mousex, mousey, screenCursorPos, size);
						ImGui::PopClipRect();
						ImGui::EndChild();
						if (dockSlot != DockSlot::None)
						{
							Dock *draggedWindowDock = draggedWindow->getDockspace()->m_container.splits[0]->activeDock;
							draggedWindowDock->redockFrom = this;
							draggedWindowDock->redockTo = container->activeDock;
							draggedWindowDock->redockSlot = dockSlot;
							draggedWindowDock->redockFromWindow = m_owner;
						}
						else draggedWindow->getDockspace()->m_container.splits[0]->activeDock->redockTo = nullptr;
					}
				}

				ImGui::PopStyleColor(1);
			}
			else
			{
				ImVec2 calculatedSize0 = size, calculatedSize1;

				if (container->splits[1])
				{
					float acontsizeX = container->splits[0]->size ? container->splits[0]->size :
						container->splits[1]->size ? size.x - container->splits[1]->size - splitterButtonWidth : size.x / 2 - splitterButtonWidthHalf;
					float acontsizeY = container->splits[0]->size ? container->splits[0]->size :
						container->splits[1]->size ? size.y - container->splits[1]->size - splitterButtonWidth : size.y / 2 - splitterButtonWidthHalf;

					float bcontsizeX = container->splits[0]->size ? size.x - container->splits[0]->size - splitterButtonWidth :
						container->splits[1]->size ? container->splits[1]->size : size.x / 2 - splitterButtonWidthHalf;
					float bcontsizeY = container->splits[0]->size ? size.y - container->splits[0]->size - splitterButtonWidth :
						container->splits[1]->size ? container->splits[1]->size : size.y / 2 - splitterButtonWidthHalf;

					calculatedSize0 = ImVec2(container->verticalSplit ? acontsizeX : size.x, !container->verticalSplit ? acontsizeY : size.y);
					calculatedSize1 = ImVec2(container->verticalSplit ? bcontsizeX : size.x, !container->verticalSplit ? bcontsizeY : size.y);
				}
				if (container->splits[0])
				{
					if (container->splits[0] == nullptr)
						size.x = 1;
					renderContainer(container->splits[0], calculatedSize0, calculatedCursorPos);
					if (container->verticalSplit)
						calculatedCursorPos.x = calculatedSize0.x + calculatedCursorPos.x + splitterButtonWidth;
					else
					{
						calculatedCursorPos.y = calculatedSize0.y + calculatedCursorPos.y + splitterButtonWidth;
					}
				}
				Container *thisContainer = container->splits[1];
				if (container->splits[1])
				{
					ImGui::SetCursorPosX(calculatedCursorPos.x - splitterButtonWidth);
					ImGui::SetCursorPosY(calculatedCursorPos.y - splitterButtonWidth);
					std::string idnamesb = "##SplitterButton";
					idnamesb += idgen++;
					ImGui::InvisibleButton(idnamesb.c_str(), ImVec2(
						container->verticalSplit ? splitterButtonWidth : size.x + splitterButtonWidth,
						!container->verticalSplit ? splitterButtonWidth : size.y + splitterButtonWidth));

					ImGui::SetItemAllowOverlap(); // This is to allow having other buttons OVER our splitter. 

					if (ImGui::IsItemActive())
					{
						float mouse_delta = !container->verticalSplit ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;

						if (container->splits[0]->alwaysAutoResize != true)
						{
							ImVec2 minSize;
							_getMinSize(container->splits[0], &minSize);
							if (container->splits[0]->size == 0)
								container->splits[0]->size = container->verticalSplit ? calculatedSize1.x : calculatedSize1.y;
							if (container->splits[0]->size + mouse_delta >= (container->verticalSplit ? minSize.x : minSize.y))
								container->splits[0]->size += mouse_delta;
						}
						else
						{
							ImVec2 minSize;
							_getMinSize(container->splits[1], &minSize);
							if (container->splits[1]->size == 0)
								container->splits[1]->size = container->verticalSplit ? calculatedSize1.x : calculatedSize1.y;
							if (container->splits[1]->size - mouse_delta >= (container->verticalSplit ? minSize.x : minSize.y))
								container->splits[1]->size -= mouse_delta;
						}
					}

					if (ImGui::IsItemHovered() || ImGui::IsItemActive())
						//ImGui::SetMouseCursor(container->verticalSplit ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
						SetCursor(LoadCursor(NULL, container->verticalSplit ? IDC_SIZEWE : IDC_SIZENS));

					renderContainer(container->splits[1], calculatedSize1, calculatedCursorPos);
				}
			}
		};

		ImVec2 backup_pos = ImGui::GetCursorPos();
		renderContainer(&m_container, dockspaceSize, backup_pos);
		ImGui::SetCursorPos(backup_pos);

		if (m_currentDockTo)
		{
			if (m_currentDockToAction == eUndock)
			{
				if (undock(m_currentDockTo))
				{
					ImGuiContext *imguiContext = ImGui::GetCurrentContext();
					m_currentDockTo->container = nullptr;
					m_currentDockTo->draging = true;
					GuiWindow *guiWindow = new GuiWindow(m_windowListPtr, m_currentDockTo->title, 0, 0, m_currentDockTo->lastSize.x, m_currentDockTo->lastSize.y /*+ 32*/, GuiWindow::fShow);
					guiWindow->dockspace.dock(m_currentDockTo, DockSlot::Tab, 0, true);
					guiWindow->setMinMaxSize(m_currentDockTo->minSize.x, m_currentDockTo->minSize.y, 0, 0);
					auto &io = m_owner->m_imGuiContext->IO;
					guiWindow->setPosition((int)(io.MousePos.x) + m_owner->m_posx-20, (int)io.MousePos.y + m_owner->m_posy- tabbarHeight/2+2);
				//	SetActiveWindow(guiWindow->getWindowHandle());
					//SetFocus(guiWindow->getWindowHandle());

					m_windowListPtr->push_back(guiWindow);
					ImGui::SetCurrentContext(imguiContext);
				}
			}
			else if (m_currentDockToAction == eDrag)
			{
				if (!m_currentDockTo->draging)
				{
					if (m_owner)
					{
						m_owner->setPosition(m_owner->m_posx, m_owner->m_posy + 32-4);
						m_owner->setSize(m_owner->m_width, m_owner->m_height - 32);
					}

				}
				m_currentDockTo->draging = true;
			}
			else if(m_currentDockToAction == eClose)
			{
				if (m_currentDockTo->onCloseFunction)
				{
					if (m_currentDockTo->onCloseFunction()) undock(m_currentDockTo);
				}
				else undock(m_currentDockTo);
			}
			m_currentDockTo = nullptr;
			m_currentDockToAction = eNull;
		}
	};

	void Dockspace::clear()
	{
		for (auto container : m_containers)
		{
			delete container;
		}
		m_containers.clear();

		m_container = {};
	};

	bool Dockspace::_getMinSize(Container *container, ImVec2 *min)
	{
		int begin = 0;
		if (container->splits[0] == nullptr)
		{
			if (min->x < container->activeDock->minSize.x)
				min->x = container->activeDock->minSize.x;
			if (min->y < container->activeDock->minSize.y)
				min->y = container->activeDock->minSize.y;
			return true;
		}
		else
		{
			if (_getMinSize(container->splits[0], min))
			{
				if (container->splits[1])
				{
					if (_getMinSize(container->splits[1], min))
					{
						return true;
					}
				}
			};
		}

		return false;
	};

	GuiWindow *Dockspace::_isAnyWindowDragging()
	{
		for (auto windows : *m_windowListPtr)
			if (windows->getDockspace()->m_container.splits[0]->activeDock->draging)
				return windows;

		return nullptr;
	};

	DockSlot Dockspace::_renderDockSlotPreview(int mx, int my, ImVec2 cPos, ImVec2 cSize)
	{
		float halfposx = (cSize.x / 2) + cPos.x;
		float halfposy = (cSize.y / 2) + cPos.y;
		float halfScreenSizeY = ImGui::GetIO().DisplaySize.y / 2;
		float halfScreenSizeX = ImGui::GetIO().DisplaySize.x / 2;
		float buttonRectHalfSize = 20;
		float buttonRectOffset = 7;

		DockSlot dockSlot = DockSlot::None;
		//center
		if ((mx > (halfposx - 25) && mx < (halfposx + 25)) && (my > (halfposy - 25) && my < (halfposy + 25))) dockSlot = DockSlot::Tab;
		if ((mx >(halfposx - 85) && mx < (halfposx - 35)) && (my >(halfposy - 25) && my < (halfposy + 25))) dockSlot = DockSlot::Left;
		if ((mx <(halfposx + 85) && mx >(halfposx + 35)) && (my >(halfposy - 25) && my < (halfposy + 25))) dockSlot = DockSlot::Right;
		if ((mx >(halfposx - 25) && mx < (halfposx + 25)) && (my >(halfposy + 35) && my < (halfposy + 85))) dockSlot = DockSlot::Bottom;
		if ((mx >(halfposx - 25) && mx < (halfposx + 25)) && (my < (halfposy - 35) && my >(halfposy - 85))) dockSlot = DockSlot::Top;
		switch (dockSlot)
		{
		case ImGuiDock::DockSlot::Left:
			ImGui::GetWindowDrawList()->AddRectFilled(cPos, ImVec2(cPos.x + (cSize.x / 2), cPos.y + cSize.y), ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .5)));//tab
			break;
		case ImGuiDock::DockSlot::Right:
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cPos.x + (cSize.x / 2), cPos.y), ImVec2(cPos.x + cSize.x, cPos.y + cSize.y), ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .5)));//tab
			break;
		case ImGuiDock::DockSlot::Top:
			ImGui::GetWindowDrawList()->AddRectFilled(cPos, ImVec2(cPos.x + cSize.x, cPos.y + (cSize.y / 2)), ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .5)));//tab
			break;
		case ImGuiDock::DockSlot::Bottom:
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cPos.x, cPos.y + (cSize.y/2)), ImVec2(cPos.x + cSize.x, cPos.y + cSize.y), ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .5)));//tab
			break;
		case ImGuiDock::DockSlot::Tab:
			ImGui::GetWindowDrawList()->AddRectFilled(cPos, ImVec2(cPos.x + cSize.x, cPos.y + cSize.y), ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .5)));//tab
			break;
		}

	/*	ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(buttonRectOffset, halfScreenSizeY - buttonRectHalfSize),
			ImVec2(2 * buttonRectHalfSize + buttonRectOffset, halfScreenSizeY + buttonRectHalfSize),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//left
		
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(2 * halfScreenSizeX - (2 * buttonRectHalfSize + buttonRectOffset), halfScreenSizeY - buttonRectHalfSize),
			ImVec2(2 * halfScreenSizeX - buttonRectOffset, halfScreenSizeY + buttonRectHalfSize),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//right*/

		//--------------------------------------------------------------------
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(halfposx - buttonRectHalfSize, halfposy - buttonRectHalfSize), 
			ImVec2(halfposx + buttonRectHalfSize, halfposy + buttonRectHalfSize),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//tab

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(halfposx - (buttonRectHalfSize * 3 + buttonRectOffset), halfposy - buttonRectHalfSize), 
			ImVec2(halfposx - (buttonRectOffset + buttonRectHalfSize), halfposy + buttonRectHalfSize),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//left

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(halfposx + (3 * buttonRectHalfSize + buttonRectOffset), halfposy - buttonRectHalfSize),
			ImVec2(halfposx + buttonRectOffset + buttonRectHalfSize, halfposy + buttonRectHalfSize),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//right

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(halfposx - buttonRectHalfSize, halfposy + buttonRectHalfSize + buttonRectOffset),
			ImVec2(halfposx + buttonRectHalfSize, halfposy + (3 * buttonRectHalfSize + buttonRectOffset)),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//bot rev
		
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(halfposx - buttonRectHalfSize, halfposy - (buttonRectHalfSize + buttonRectOffset)),
			ImVec2(halfposx + buttonRectHalfSize, halfposy - (3 * buttonRectHalfSize + buttonRectOffset)),
			ImGui::ColorConvertFloat4ToU32(ImVec4(.4, .4, .4, .8)));//top rev
		
		return dockSlot;
	}

	void Dockspace::_renderTabBar(Container *container, ImVec2 size, ImVec2 cursorPos)
	{
		ImGui::SetCursorPos(cursorPos);

		ImGui::PushStyleColor(ImGuiCol_CloseButton, ImVec4(.31, .31, .31, 1));
		ImGui::PushStyleColor(ImGuiCol_CloseButtonHovered, ImVec4(.3, .3, .3, 1));
		ImGui::PushStyleColor(ImGuiCol_CloseButtonActive, ImVec4(.2, .2, .2, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 3));
		for (auto dock : container->docks)
		{
			std::string dockTitle = dock->title;
			if (dock->closeButton == true)
				dockTitle += "  ";

			if (dock == container->activeDock)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.25, .25, .25, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.25, .25, .25, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.25, .25, .25, 1));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.21, .21, .21, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.35, .35, .35, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.4, .4, .4, 1));
			}
			if (ImGui::Button(dockTitle.c_str(), ImVec2(0, 20)))
			{
				container->activeDock = dock;
			}
			if (ImGui::IsItemActive())
			{
				float delta = ImGui::GetCursorScreenPos().y - ImGui::GetIO().MousePos.y;
				if (dock->container->parent == &m_container && m_container.splits[1] == nullptr)
				{
					if (delta < -2 || delta > 2)
					{
						m_currentDockToAction = eDrag;
						m_currentDockTo = dock;
					}
				}
				else
				{
					if (delta < -4 || delta > 27)
					{
						if (dock->undockable == false)
						{
							m_currentDockToAction = eUndock;
							m_currentDockTo = dock;
						}
					}
				}
			}

			ImGui::SameLine();
			
			/*if (dock->closeButton == true)
			{
				ImVec2 backupCursorPos = ImGui::GetCursorScreenPos();
				ImGui::SetCursorPosX(backupCursorPos.x - 26);
				ImGui::SetCursorPosY(backupCursorPos.y +10);
				ImGui::SetItemAllowOverlap();
				if(ImGui::CloseButton(5, ImVec2(backupCursorPos.x - 18, backupCursorPos.y + 10), 7))
				{
					m_currentDockToAction = eClose;
					m_currentDockTo = dock;
				}
				ImGui::SetCursorPos(backupCursorPos);
			}*/
			ImGui::PopStyleColor(3);
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
	};
}