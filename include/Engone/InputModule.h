#pragma once

#include "Engone/PlatformLayer.h"

namespace engone {
	struct InputModule {
        void init(GLFWwindow* window);
        ~InputModule();

		inline GLFWwindow* glfw() const { return m_glfwWindow; }

		// True if cursor is visible.
		bool isCursorVisible() const { return m_cursorVisible; }

		float getMouseX() const { return m_mouseX; }
		float getMouseY() const { return m_mouseY; }
		// raw input from first person
		// float getRawMouseX() const;
		// float getRawMouseY() const;
		void setMouseX(float x) { m_mouseX=x; }
		void setMouseY(float y) { m_mouseY=y; }
		float getScrollX() const;
		float getScrollY() const;
		void setScrollX(float x) { m_frameScrollX += x; m_tickScrollX += x;}
		void setScrollY(float y) { m_frameScrollY += y; m_tickScrollY += y; }

		void setInput(int code, bool down);

		std::string pollPathDrop();
		u32 pollChar();
        void resetPollChar();

		bool isKeyDown(int code);
		bool isKeyPressed(int code);
		bool isKeyReleased(int code);
		
		// void enableFirstPerson(bool yes);
		// bool isFirstPerson() const { return m_enabledFirstPerson; }

		// If true, the cursor will be made visible.
		void setCursorVisible(bool visible);
		// Whether the cursor is locked to the window.
		bool isCursorLocked() const { return m_cursorLocked;}
		// If true, the cursor will be made invisible and locked to the window. Use this when you want the player to lock around.
		void lockCursor(bool locked);

		void resetEvents(bool resetFrameInput);

		GLFWwindow* m_glfwWindow=nullptr;
		
		struct Input {
			int code=0;
			bool down = false;
			uint8_t tickPressed = 0;
			uint8_t framePressed = 0;
			uint8_t tickReleased = 0;
			uint8_t frameReleased = 0;
		};
		
		std::vector<Input> m_inputs;
		std::vector<std::string> m_pathDrops;
		
		u32 m_charIn = 0;
		u32 m_charOut = 0;
		bool m_emptyCharArray = true;
		static const u32 CHAR_ARRAY_SIZE = 20;
		u32 m_charArray[CHAR_ARRAY_SIZE];

		// bool m_enabledFirstPerson = false;

		float m_mouseX = 0, m_mouseY = 0;
		float m_lastMouseX = -1, m_lastMouseY=-1;
		// float m_tickRawMouseX = 0, m_tickRawMouseY = 0, m_frameRawMouseX = 0, m_frameRawMouseY = 0;
		float m_tickScrollX=0, m_tickScrollY = 0;
		float m_frameScrollX = 0, m_frameScrollY = 0;

		bool m_cursorVisible = true, m_cursorLocked = false, m_focus = true;
	};
}