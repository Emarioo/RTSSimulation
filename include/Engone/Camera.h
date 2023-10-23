#pragma once

// You cannot access position or rotation directly because the view matrix needs to be
// updated whenever the camera changes.
// ISSUE: consider moving definitions to a .cpp file.
namespace engone {
	class Camera {
	public:
		Camera() { refreshViewMatrix(); };

		void setPosition(float x, float y, float z) { position.x = x; position.y = y; position.z = z; refreshViewMatrix(); }
		void setPosition(glm::vec3 v) { position = v; refreshViewMatrix(); }
		void setX(float x) { position.x = x; refreshViewMatrix(); }
		void setY(float y) { position.y = y; refreshViewMatrix(); }
		void setZ(float z) { position.z = z; refreshViewMatrix(); }

		void setRotation(float x, float y, float z) { rotation.x = x; rotation.y = y; rotation.z = z; refreshViewMatrix(); }
		void setRotation(glm::vec3 v) { rotation = v; refreshViewMatrix(); }
		void setRX(float x) { rotation.x = x; refreshViewMatrix(); }
		void setRY(float y) { rotation.y = y; refreshViewMatrix(); }
		void setRZ(float z) { rotation.z = z; refreshViewMatrix(); }

		const glm::vec3& getPosition() const { return position; }
		const glm::vec3& getRotation() const { return rotation; }
		const glm::mat4& getViewMatrix() const { return viewMatrix; }

		glm::vec3 getLookVector() {
			glm::vec3 look = {};
			look.x = -sin(rotation.y)*cos(rotation.x);
			look.z = -cos(rotation.y)*cos(rotation.x);
			look.y = sin(rotation.x);
			return look;
		}
		glm::vec3 getFlatLookVector() {
			glm::vec3 look = {};
			look.x = -sin(rotation.y);
			look.z = -cos(rotation.y);
			return look;
		}
		glm::vec3 getUpVector() {
			glm::vec3 look = {};
			look.x = sin(rotation.y)*sin(rotation.x);
			look.z = cos(rotation.y)*sin(rotation.x);
			look.y = cos(rotation.x);
			return look;
		}
		glm::vec3 getRightVector() {
			glm::vec3 look = {};
			look.x = cos(rotation.y);
			look.z = -sin(rotation.y);
			return look;
		}
		// returns vec3{ screenX, screenY, zoom/depth }
		glm::vec3 worldPosToScreenPos(const glm::vec3& pos, const glm::mat4& perspective, int winWidth, int winHeight, bool& visible) {
			glm::vec4 coord = (perspective * viewMatrix) * glm::vec4(pos,1.f);
			coord.x /= coord.w;
			coord.y /= coord.w;
			// coord.z /= coord.w;
			if(coord.x < -1.f || coord.x > 1.f || coord.y < -1.f || coord.y > 1.f || coord.z < 0.f) {
				visible = false;
				return {0,0,1};
			}
			visible = true;
			return {(1.f + coord.x) / 2.f * winWidth, (1.f - coord.y) / 2.f *winHeight, coord.z };
		}
        
        glm::vec3 directionFromMouse(int mouseX, int mouseY, int winWidth, int winHeight, const glm::mat4& perspectiveMatrix) {
            // https://gamedev.stackexchange.com/questions/200114/constructing-a-world-ray-from-mouse-coordinates
            // screen/window space
            float screen_x = (float)mouseX / winWidth * 2.f - 1.f;
            float screen_y = 1.f - (float)mouseY / winHeight * 2.f;
            glm::vec4 screen_point = glm::vec4(screen_x, screen_y, 1.0f, 1.0f);

            glm::mat4 viewMatrix_without_position = glm::inverse(
				glm::rotate(rotation.y, glm::vec3(0, 1, 0)) *
				glm::rotate(rotation.x, glm::vec3(1, 0, 0))
			);
            glm::vec3 ray_dir = glm::inverse(viewMatrix_without_position) * (glm::inverse(perspectiveMatrix) * screen_point);
            ray_dir = glm::normalize(ray_dir);
            return ray_dir;
        }

	private:
		glm::vec3 position{};
		glm::vec3 rotation{};
		glm::mat4 viewMatrix{};

		void refreshViewMatrix() {
			viewMatrix = glm::inverse(
				glm::translate(glm::mat4(1.0f), position) *
				glm::rotate(rotation.y, glm::vec3(0, 1, 0)) *
				glm::rotate(rotation.x, glm::vec3(1, 0, 0))
			);
		}

	};
}