#pragma once

#include "Engone/Utilities/ImageUtility.h"

#define GL_CHECK()  {int err = glGetError();if(err) {printf("OH NO %d\n",err); engone::log::out << engone::log::RED<<"GLError: "<<err<<" "<<(const char*)glewGetErrorString(err)<<"\n"; *(int*)0;}}
// #define GL_CHECK()  {int err = glGetError();if(err) {printf("OH NO %d\n",err); engone::log::out << engone::log::RED<<"GLError: "<<err<<" "<<(const char*)glewGetErrorString(err)<<"\n"; DebugBreak();}}

#include "glm/glm.hpp"

namespace engone {

	/*
		NOTE: You could go crazy about object oriented design for the objects and buffers but don't.
			Doing that will make the code confusing and that is the last thing the engine needs.
			If you want to write less code then it would be a good idea.
		NOTE: Error messages kind of only needed in draw calls. Because that's when you'll notice something's wrong.
	*/

	class GLObject {
	public:
		GLObject() = default;

		// careful with this
		//GLObject(const GLObject&) = delete;
		//GLObject& operator=(const GLObject&) = delete;

		virtual void cleanup() = 0;

		constexpr bool initialized() const { return m_id != 0; }

	protected:
		uint32_t m_id = 0;

		friend class VertexArray;
	};
	// Everything that has to do with memory is in bytes. sizeof(char)
	class GLBuffer : public GLObject {
	public:
		GLBuffer() = default;

		// Will allocate/reallocate buffer if buffer doesn't exist or size is bigger than previous size.
		// Will replace data in buffer with new data if size is equal or less then previous size.
		// size in bytes, offset in bytes
		virtual void setData(uint32_t size, void* data, uint32_t offset = 0) = 0;
		// Will fill data with data from opengl
		// size in bytes, offset in bytes
		virtual void getData(uint32_t size, void* data, uint32_t offset = 0) = 0;

		// size in bytes
		uint32_t getSize() const { return m_size; }

	protected:
		uint32_t m_size = 0;
	};

	class VertexBuffer : public GLBuffer {
	public:
		VertexBuffer() = default;
		~VertexBuffer() { cleanup(); };

		// These make it possible to cause bad behaviour
		//VertexBuffer(const VertexBuffer& ib) = delete;
		//VertexBuffer& operator=(const VertexBuffer&) = delete;

		void bind() const;
		void unbind() const;
		void cleanup() override;
		
		// size in bytes
		void setData(uint32_t size, void* data, uint32_t offset = 0) override;
		// size in bytes
		void getData(uint32_t size, void* data, uint32_t offset = 0) override;

	};
	class IndexBuffer : public GLBuffer {
	public:
		IndexBuffer() = default;
		~IndexBuffer() { cleanup(); };

		// These make it possible to cause bad behaviour
		//IndexBuffer(const IndexBuffer& ib) = delete;
		//IndexBuffer& operator=(const IndexBuffer&) = delete;
		void bind() const;
		void unbind() const;
		void cleanup() override;

		void setData(uint32_t size, void* data, uint32_t offset = 0) override;
		void getData(uint32_t size, void* data, uint32_t offset = 0) override;


	private:

		//friend class VertexArray;
	};
	// Shader Storage Buffer Object
	class ShaderBuffer : public GLBuffer {
	public:
		ShaderBuffer() = default;
		~ShaderBuffer() { cleanup(); };

		// These make it possible to cause bad behaviour
		//ShaderBuffer(const ShaderBuffer&) = delete;
		//ShaderBuffer& operator=(const ShaderBuffer&) = delete;

		void bind() const;
		void unbind() const;
		void cleanup() override;

		void setData(uint32_t size, void* data, uint32_t offset = 0) override;
		void getData(uint32_t size, void* data, uint32_t offset = 0) override;

		void bindBase(int index) const;

		void drawPoints(int count, int bindingIndex);

	private:
		uint32_t m_vaId = 0;
	};
	class VertexArray : public GLObject {
	public:
		VertexArray() = default;
		~VertexArray() { cleanup(); };

		// These make it possible to cause bad behaviour
		//VertexArray(const VertexArray& ib) = delete;
		//VertexArray& operator=(const VertexArray&) = delete;

		void bind() const;
		void unbind() const;
		void cleanup() override;

		// reset attributes, not sure if it works
		void clear();

		void addAttribute(u8 floatSize);
		void addAttribute(u8 floatSize, VertexBuffer* buffer);
		void addAttribute(u8 floatSize, u8 divisor);
		void addAttribute(u8 floatSize, u8 divisor, VertexBuffer* buffer);

		// If you are using instanced bufferes
		void selectBuffer(u8 location, VertexBuffer* buffer);

		void drawPoints(int count);
		void drawLines(IndexBuffer* indexBuffer);
		void draw(IndexBuffer* indexBuffer);
		void draw(IndexBuffer* indexBuffer, uint32_t instanceAmount);
		// NOTE THAT THE ARGUMENT IS VERTEX COUNT NOT TRIANGLE COUNT
		void drawTriangleArray(int vertexCount);
		// NOTE THAT THE ARGUMENT IS VERTEX COUNT NOT LINE COUNT
		void drawLineArray(int vertexCount);
		
		static const int MAX_LOCATIONS =  8;
		static const int MAX_BUFFERS =  2;
	private:

		u8 totalLocation = 0, location = 0, bufferSection = 0;
		// max locations
		u8 locationSizes[MAX_LOCATIONS]{};
		// Buffer section
		u8 strides[MAX_BUFFERS]{};
		u8 startLocations[MAX_BUFFERS]{};
	};

	class Texture : public GLObject {
	public:
		Texture() = default;
		~Texture() { cleanup(); }

		void bind(unsigned int slot = 0);
		void unbind();
		void cleanup() override;

		//void init(const char* path);

		// image cannot be nullptr
		void init(RawImage* image);
		// void init(int width, int height, char* data);
		// void init(char* data, int width, int height, int x = 0, int y = 0);
		//void init(const void* inBuffer, uint32_t size);
		//Texture(const std::string& path) : Asset(TYPE) { load(path); };
		//Texture(const char* inBuffer, uint32_t size, Assets* assets = nullptr) : Asset(TYPE, "") { load(inBuffer, size, assets); };
		//void load(const void* inBuffer, uint32_t size);
		//void load(const std::string& path) override;

		//void init(int w, int h, void* data);
		
		// function is not complete. Initialization of the image can only be done once.
		void setData(char* data, int width, int height, int x = 0, int y=0);

		int getWidth() const;
		int getHeight() const;

	private:
		unsigned char* buffer = nullptr;
		int m_width = 0, m_height = 0, BPP = 0;
	};

	// Note that the framebuffer doesn't inherit GLBuffer since you link other buffers to it.
	class FrameBuffer : public GLObject {
	public:
		FrameBuffer() = default;
		~FrameBuffer() { cleanup(); }

		void init();
		void initBlur(int width, int height);

		void bind() const;
		void unbind() const;
		void cleanup() override;

		// don't forget view port when drawing
		//void bind();
		//void unbind();
		// will not resize if the size is the same
		void resize(int width, int height);

		void bindTexture();
		void bindRenderbuffer();

		// will bind and unbind appropriate buffers and copy depth to default framebuffer.
		// May not work if this and the default frame buffer have different depth and stencil formats
		// (uses GL_DEPTH24_STENCIL8)
		// use texture instead of renderbuffer for depth if this is the case.
		// width, height of the destination frame buffer.
		void blitDepth(int width, int height);

		int getWidth()const { return m_width; }
		int getHeight()const { return m_height; }
		int m_width = 1024, m_height = 1024;

		unsigned int m_textureId = 0;
		uint32_t m_renderBufferId = 0;
	};
	class Shader : public GLObject {
	public:
		Shader() = default;
		Shader(const char* source) { init(source); }
		~Shader() { cleanup(); }

		void bind();
		void unbind();
		void cleanup() override;

		//void load(const std::string& path, Assets* assets) override;
		void init(const char* source);

		struct ShaderSource {
			const char* start = nullptr; // this is not null terminated
			int length = 0;
			int line = 0;// which line on the original

			bool operator==(ShaderSource src) {
				return start == src.start && length == src.length;
			}
		};
		struct GenRes{
			ShaderSource vs;
			ShaderSource fs;
		};
		static GenRes GenerateSources(const char* source);

		ShaderSource vs, fs;
		const char* original = nullptr;

		void setInt(const std::string& name, int i);
		void setFloat(const std::string& name, float f);
		void setVec2(const std::string& name, glm::vec2);
		void setIVec2(const std::string& name, glm::ivec2);
		void setVec3(const std::string& name, glm::vec3);
		void setIVec3(const std::string& name, glm::ivec3);
		void setVec4(const std::string& name, glm::vec4 v);
		void setMat4(const std::string& name, glm::mat4 v);

		//static TrackerId trackerId;
	private:
		uint32_t createShader(ShaderSource vertexSrc, ShaderSource fragmentSrc);
		uint32_t compileShader(uint32_t type, ShaderSource source);

		unsigned int getUniformLocation(const std::string& name);
		std::unordered_map<std::string, unsigned int> uniLocations;
	};
}