#include "Engone/InputModule.h"

// #include "Engone/Utilities/Utilities.h"
#include "Engone/Logger.h"

namespace engone {

	static std::unordered_map<GLFWwindow*, InputModule*> moduleMapping; // Todo: doesn't work with dlls
	static InputModule* GetMappedModule(GLFWwindow* window) {
		auto win = moduleMapping.find(window);
		if (win != moduleMapping.end())
			return win->second;
		return nullptr;
	}
	void CharCallback(GLFWwindow* window, u32 chr) {
		auto input = GetMappedModule(window);
		if (!input) {
			log::out << "InputModule isn't mapped\n";
			return;
		}
		if (input->m_charIn == input->m_charOut && !input->m_emptyCharArray) {
			log::out << log::RED << "CharCallback : m_charArray["<<InputModule::CHAR_ARRAY_SIZE<<"] of InputModule is full, skipping "<<chr<<" (increase size to prevent this).\n";
			return; // list is full
		}

		input->m_charArray[input->m_charIn] = chr;
		input->m_charIn = (input->m_charIn+1)% InputModule::CHAR_ARRAY_SIZE;
		input->m_emptyCharArray = false;
	}
	void DropCallback(GLFWwindow* window, int count, const char** paths) {
		auto input = GetMappedModule(window);
		if (!input) {
			log::out << "InputModule isn't mapped\n";
			return;
		}
		for (int i = 0; i < count; ++i) {
			input->m_pathDrops.push_back(paths[i]);
		}
	}
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto input = GetMappedModule(window);
		if (!input) {
			log::out << "InputModule isn't mapped\n";
			return;
		}
		// Event e{ EventKey };
		// e.key = key;
		// e.scancode = scancode;
		// e.action = action;
		// e.window = win;
		// win->addEvent(e);

		input->setInput(key, action != 0); // if not 0 means if pressed or repeating

		if (action != 0 && (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE || key == GLFW_KEY_ENTER || key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT)) {
			CharCallback(window, key);
		}
		//win->runListeners();
	}
	void MouseCallback(GLFWwindow* window, int button, int action, int mods) {
		auto input = GetMappedModule(window);
		if (!input) {
			log::out << "InputModule isn't mapped\n";
			return;
		}
		// Position needs updating
		double mx, my;
		glfwGetCursorPos(window, &mx, &my);
		input->setMouseX((float)mx);
		input->setMouseY((float)my);
		input->setInput(button, action != 0);

		//log::out << "click " << button << " " <<action<< "\n";

		// Event e{ EventClick };
		// e.button = button;
		// e.action = action;
		// e.mx = (float)mx;
		// e.my = (float)my;
		// e.window = win;
		// win->addEvent(e);
		//win->runListeners();
	}
	void CursorPosCallback(GLFWwindow* window, double mx, double my) {
		auto input = GetMappedModule(window);
		if (!input) {
			log::out << "InputModule isn't mapped\n";
			return;
		}
		input->setMouseX((float)mx);
		input->setMouseY((float)my);
		// Event e{ EventMove };
		// e.mx = (float)mx;
		// e.my = (float)my;
		// e.window = win;
		// win->addEvent(e);
		//win->runListeners();
	}
	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
		auto input = GetMappedModule(window);
		if (!input) {
			log::out << "InputModule isn't mapped\n";
			return;
		}
		input->setScrollX((float)xoffset);
		input->setScrollY((float)yoffset);

		// printf("Callback %llf\n",yoffset);

		// Event e{ EventScroll };
		// e.scrollX = (float)xoffset;
		// e.scrollY = (float)yoffset;
		// e.mx = win->getMouseX();
		// e.my = win->getMouseY();
		// e.window = win;
		// win->addEvent(e);
		//win->runListeners();
	}
	// void FocusCallback(GLFWwindow* window, int focused) {
	// 	auto input = GetMappedModule(window);
	// 	if (!win) return;
		
	// 	win->m_focus = focused;
		
	// }
	// void ResizeCallback(GLFWwindow* window, int width, int height) {
	// 	auto input = GetMappedModule(window);
	// 	if (!win){
	// 		log::out << "InputModule isn't mapped\n";
	// 		return;
	// 	}

	// 	Event e{ EventResize };
	// 	e.width = (float)width;
	// 	e.height = (float)height;
	// 	e.window = win;
	// 	win->addEvent(e);
	// 	//win->runListeners();

	// 	win->w = (float)width;
	// 	win->h = (float)height;
		
	// 	// if(win->getParent()->getEngine()->getFlags()&Engone::EngoneRenderOnResize)
	// 	// 	win->getParent()->getEngine()->renderApps();
	// 	if(win->getParent()->getFlags()&Engone::EngoneRenderOnResize)
	// 		win->getParent()->renderApps();
	// }
	// void CloseCallback(GLFWwindow* window) {
	// 	InputModule* win = GetMappedWindow(window);
	// 	if (!win) return;
	// 	// DebugBreak();
	// 	Event e{ EventClose };
	// 	e.window = win;
	// 	win->addEvent(e);
	// 	// if (win->m_parent) {
	// 	// 	win->m_parent->onClose(win);
	// 	// }
	// 	//if (mainWindow == win)
	// 	//	mainWindow = nullptr;
	// }
	// void PosCallback(GLFWwindow* window, int x, int y) {
	// 	InputModule* win = GetMappedWindow(window);
	// 	if (!win) return;
		
	// 	win->x = (float)x;
	// 	win->y = (float)y;
	// 	// if(win->getParent()->getEngine()->getFlags()&Engone::EngoneRenderOnResize)
	// 	// 	win->getParent()->getEngine()->renderApps();
	// }
	// static float cameraSensitivity = 0.2f;
	// EventType FirstPerson(Event& e) {
	// 	if (e.window->m_lastMouseX != -1) {
	// 		CommonRenderer* renderer = e.window->getCommonRenderer();
	// 		Camera* camera = nullptr;
	// 		if (renderer)
	// 			camera = renderer->getCamera();
	// 		if (e.window->isCursorLocked() && camera != nullptr) {
	// 			float rawX = -(e.mx - e.window->m_lastMouseX) * (glm::pi<float>() / 360.0f) * cameraSensitivity;
	// 			float rawY = -(e.my - e.window->m_lastMouseY) * (glm::pi<float>() / 360.0f) * cameraSensitivity;

	// 			e.window->m_tickRawMouseX += rawX;
	// 			e.window->m_frameRawMouseX += rawX;
	// 			e.window->m_tickRawMouseY += rawY;
	// 			e.window->m_frameRawMouseY += rawY;

	// 			if (e.window->m_enabledFirstPerson) {
	// 				glm::vec3 rot = camera->getRotation(); // get a copy of rotation
	// 				rot.y += rawX;
	// 				rot.x += rawY;
	// 				// clamp up and down directions.
	// 				if (rot.x > glm::pi<float>() / 2) {
	// 					rot.x = glm::pi<float>() / 2;
	// 				}
	// 				if (rot.x < -glm::pi<float>() / 2) {
	// 					rot.x = -glm::pi<float>() / 2;
	// 				}
	// 				rot.x = remainder(rot.x, glm::pi<float>() * 2);
	// 				rot.y = remainder(rot.y, glm::pi<float>() * 2);
	// 				camera->setRotation(rot); // set the new rotation
	// 				//log::out << "FIRST PERSON\n";
	// 			}
	// 		}
	// 	}
	// 	e.window->m_lastMouseX = e.mx;
	// 	e.window->m_lastMouseY = e.my;
	// 	return EventNone;
	// }
	// EventType FirstPersonProc(Event* _e) {
	// 	Event& e = *_e;
	// 	if (e.window->m_lastMouseX != -1) {
	// 		CommonRenderer* renderer = e.window->getCommonRenderer();
	// 		Camera* camera = nullptr;
	// 		if (renderer)
	// 			camera = renderer->getCamera();
	// 		if (e.window->isCursorLocked() && camera != nullptr) {
	// 			float rawX = -(e.mx - e.window->m_lastMouseX) * (glm::pi<float>() / 360.0f) * cameraSensitivity;
	// 			float rawY = -(e.my - e.window->m_lastMouseY) * (glm::pi<float>() / 360.0f) * cameraSensitivity;

	// 			e.window->m_tickRawMouseX += rawX;
	// 			e.window->m_frameRawMouseX += rawX;
	// 			e.window->m_tickRawMouseY += rawY;
	// 			e.window->m_frameRawMouseY += rawY;

	// 			if (e.window->m_enabledFirstPerson) {
	// 				glm::vec3 rot = camera->getRotation(); // get a copy of rotation
	// 				rot.y += rawX;
	// 				rot.x += rawY;
	// 				// clamp up and down directions.
	// 				if (rot.x > glm::pi<float>() / 2) {
	// 					rot.x = glm::pi<float>() / 2;
	// 				}
	// 				if (rot.x < -glm::pi<float>() / 2) {
	// 					rot.x = -glm::pi<float>() / 2;
	// 				}
	// 				rot.x = remainder(rot.x, glm::pi<float>() * 2);
	// 				rot.y = remainder(rot.y, glm::pi<float>() * 2);
	// 				camera->setRotation(rot); // set the new rotation
	// 				//log::out << "FIRST PERSON\n";
	// 			}
	// 		}
	// 	}
	// 	e.window->m_lastMouseX = e.mx;
	// 	e.window->m_lastMouseY = e.my;
	// 	return EventNone;
	// }
    void SetCallbacks(GLFWwindow* window);
    void InputModule::init(GLFWwindow* window) {
        m_glfwWindow = window;
        moduleMapping[window] = this;

        SetCallbacks(window);
    }
    InputModule::~InputModule() {
        moduleMapping.erase(m_glfwWindow);
    }

	void InputModule::setInput(int code, bool down) {
		int index=-1;
		for (u32 i = 0; i < m_inputs.size(); ++i) {
			if (m_inputs[i].code == code) {
				index = i;
				break;
			}
		}
		if(index==-1){
			index = m_inputs.size();
			m_inputs.push_back({});
			m_inputs[index].code=code;
		}
		Input& input = m_inputs[index];
		if(down){
			if (!input.down) {
				// printf("PRESSED %c (%d)\n",(char)code,code);
				// log::out << "PRESSED "<<(char)code << " ("<<code << ")\n";
				input.down = true;
				input.tickPressed++;
				input.framePressed++;
			}
		}else{
			// log::out << "RELEASED "<<(char)code << " ("<<code << ")\n";
			// printf("RELEASED %c (%d)\n",(char)code,code);
			input.down = false;
			input.tickReleased++;
			input.frameReleased++;
		}
	}
	// float InputModule::getRawMouseX() const {
	// 	// if (m_parent->isRenderingWindow()) {
	// 	// 	return m_frameRawMouseX;
	// 	// } else {
	// 		return m_tickRawMouseX;
	// 	// }
	// }
	// float InputModule::getRawMouseY() const {
	// 	// if (m_parent->isRenderingWindow()) {
	// 	// 	return m_frameRawMouseY;
	// 	// } else {
	// 		return m_tickRawMouseY;
	// 	// }
	// }
	float InputModule::getScrollX() const {
		// if (m_parent->isRenderingWindow()) {
		// 	return m_frameScrollX;
		// } else {
			return m_tickScrollX;
		// }
	}
	float InputModule::getScrollY() const {
		// if (m_parent->isRenderingWindow()) {
		// 	return m_frameScrollY;
		// } else {
			return m_tickScrollY;
		// }
	}
	std::string InputModule::pollPathDrop() {
		if (m_pathDrops.size() == 0) return "";
		std::string path = m_pathDrops.back();
		m_pathDrops.pop_back();
		return path;
	}
    void InputModule::resetPollChar(){
        m_emptyCharArray = true;
        m_charOut = m_charIn;
    }
	u32 InputModule::pollChar() {
		if (m_charIn == m_charOut)
			return 0;
		
		u32 chr = m_charArray[m_charOut];
		m_charOut = (m_charOut + 1) % CHAR_ARRAY_SIZE;
		if (m_charIn == m_charOut)
			m_emptyCharArray = true;
		return chr;
	}
	bool InputModule::isKeyDown(int code) {
		for (u32 i = 0; i < m_inputs.size(); ++i) {
			if (m_inputs[i].code == code)
				return m_inputs[i].down;
		}
		return false;
	}
	bool InputModule::isKeyPressed(int code) {
		for (u32 i = 0; i < m_inputs.size(); ++i) {
			if (m_inputs[i].code == code) {
				// log::out << "IS "<<(char)code<<"\n";
				// if (m_parent->isRenderingWindow()) {
				// 	if (m_inputs[i].framePressed > 0) {
				// 		return true;
				// 	}
				// } else {
					if (m_inputs[i].tickPressed > 0) {
						// log::out << " Done\n";
						return true;
					}
				// }
				return false;
			}
		}
		return false;
	}
	bool InputModule::isKeyReleased(int code) {
		for (u32 i = 0; i < m_inputs.size(); ++i) {
			if (m_inputs[i].code == code) {
				// if (m_parent->isRenderingWindow()) {
				// 	if (m_inputs[i].frameReleased > 0) {
				// 		return true;
				// 	}
				// }
				// else {
					if (m_inputs[i].tickReleased > 0) {
						return true;
					}
				// }
				return false;
			}
		}
		return false;
	}
	void InputModule::resetEvents(bool resetFrameInput) {
		resetFrameInput = false;
        if (resetFrameInput) {
            m_frameScrollX = 0;
            m_frameScrollY = 0;
            // m_frameRawMouseX = 0;
            // m_frameRawMouseY = 0;
            for (int i = 0; i < (int)m_inputs.size(); ++i) {
                m_inputs[i].framePressed = 0;
                m_inputs[i].frameReleased = 0;
            }
        } else {
            m_tickScrollX = 0;
            m_tickScrollY = 0;
            // m_tickRawMouseX = 0;
            // m_tickRawMouseY = 0;
            for (int i = 0; i < (int)m_inputs.size(); ++i) {
                if(m_inputs[i].tickPressed!=0){
                    int code = m_inputs[i].code;
                    // log::out << "RESET "<<(char)code << " ("<<code << ")\n";
                    // printf("RESET %c (%d)\n",(char)code,code);
                }
                m_inputs[i].tickPressed = 0;
                m_inputs[i].tickReleased = 0;
            }
        }
	}
	// void InputModule::runListeners() {
	// 	for (u32 j = 0; j < m_events.size(); j++) {
	// 		EventTypes breaker = EventNone; // if a flag is simular it will break
	// 		// Todo: remove m_listeners
	// 		// for (u32 i = 0; i < m_listeners.size(); i++) {
	// 		// 	if (m_listeners[i]->eventTypes & breaker)// continue if an event has been checked/used/disabled
	// 		// 		continue;

	// 		// 	if (m_listeners[i]->eventTypes & m_events[j].eventType) {
	// 		// 		EventTypes types = m_listeners[i]->run(m_events[j]);
	// 		// 		if (types != EventNone)
	// 		// 			breaker = (breaker | m_listeners[i]->eventTypes);
	// 		// 	}
	// 		// }
	// 		for (u32 i = 0; i < listenerProcs.size(); i++) {
	// 			ListenerProc& proc = listenerProcs[i];
	// 			if (proc.types & breaker)// continue if an event has been checked/used/disabled
	// 				continue;

	// 			if (proc.types & m_events[j].eventType) {
	// 				EventTypes types = proc.proc(&m_events[j]);
	// 				if (types != EventNone)
	// 					breaker = (breaker | m_listeners[i]->eventTypes);
	// 			}
	// 		}
	// 	}
	// 	m_events.clear();
	// }
	// void InputModule::enableFirstPerson(bool yes) {
	// 	m_enabledFirstPerson = yes;
	// }
	static void SetCallbacks(GLFWwindow* window) {
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetMouseButtonCallback(window, MouseCallback);
		glfwSetCursorPosCallback(window, CursorPosCallback);
		glfwSetScrollCallback(window, ScrollCallback);
		// glfwSetWindowFocusCallback(window, FocusCallback);
		// glfwSetWindowSizeCallback(window, ResizeCallback);
		// glfwSetWindowCloseCallback(window, CloseCallback);
		// glfwSetWindowPosCallback(window, PosCallback);
		glfwSetCharCallback(window, CharCallback);
		// glfwSetDropCallback(window, DropCallback);
	}
	void InputModule::setCursorVisible(bool visible) {
		m_cursorVisible = visible;
		if (visible) {
			// NOTE: Calling this after locking the cursor will unintentionally
			//   unlock the cursor.
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		} else {
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
	}
	void InputModule::lockCursor(bool locked) {
		m_cursorLocked = locked;
		if (locked) {
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (glfwRawMouseMotionSupported()) {
				glfwSetInputMode(m_glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
			} else {
				log::out << "raw mouse motion not supported\n";
			}
		} else {
			setCursorVisible(isCursorVisible());
			// if (glfwRawMouseMotionSupported())
			glfwSetInputMode(m_glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
		}
	}
}