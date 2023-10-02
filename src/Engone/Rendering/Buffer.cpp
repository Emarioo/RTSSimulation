#include "Engone/Rendering/Buffer.h"
#include "Engone/Logger.h"

#include <GL/glew.h>

// #include "Engone/Utilities/Alloc.h"

#include "stb/stb_image.h"

// #define GL_CHECK()  {int err = glGetError();if(err) {engone::log::out << engone::log::RED<<"GLError: "<<err<<" "<<(const char*)glewGetErrorString(err)<<"\n";DebugBreak();}}
// unknown error can happen if vertex array isn't bound when a vertex buffer is bound
namespace engone {

	//static void GL_CHECK() {
	//	{
	//		int err = glGetError();
	//		if (err)
	//			log::out << log::RED << "GLError: " << err << " " << (const char*)glewGetErrorString(err) << "\n";
	//	}
	//}


	void VertexBuffer::bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, m_id);
	}
	void VertexBuffer::unbind() const {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void VertexBuffer::cleanup() {
		if (m_id != 0)
			glDeleteBuffers(1, &m_id);
	}
	void VertexBuffer::setData(uint32_t size, void* data, uint32_t offset) {
		if (m_id == 0)
			glGenBuffers(1, &m_id);

		bind();
		if (size > m_size) {
			glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
			m_size = size;
		}
		else if (size <= m_size) {
			if (data)
				glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
		}
		GL_CHECK();
		unbind();
	}
	void VertexBuffer::getData(uint32_t size, void* outData, uint32_t offset) {
		if (size == 0) return;
		bind();
		glGetBufferSubData(GL_ARRAY_BUFFER, offset, size, outData);
		GL_CHECK();
		unbind();
	}
	void IndexBuffer::bind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
	}
	void IndexBuffer::unbind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	void IndexBuffer::cleanup() {
		if (m_id != 0)
			glDeleteBuffers(1, &m_id);
	}
	void IndexBuffer::setData(uint32_t size, void* data, uint32_t offset) {
		if (m_id == 0)
			glGenBuffers(1, &m_id);

		bind();
		if (size > m_size) {
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
			m_size = size;
		}
		else if (size <= m_size) {
			if (data)
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
		}
		GL_CHECK();
		unbind();
	}
	void IndexBuffer::getData(uint32_t size, void* outData, uint32_t offset) {
		if (size == 0) return;
		bind();
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, outData);
		unbind();
	}
	void ShaderBuffer::bind() const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
	}
	void ShaderBuffer::unbind() const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	void ShaderBuffer::cleanup() {
		if (m_id != 0) {
			glDeleteBuffers(1, &m_id);
			glDeleteVertexArrays(1, &m_vaId);
		}
	}
	void ShaderBuffer::setData(uint32_t size, void* data, uint32_t offset) {
		if (m_id == 0) {
			glGenBuffers(1, &m_id);
			glGenVertexArrays(1, &m_vaId);
		}

		bind();
		if (size > m_size) {
			glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
			m_size = size;
		}
		else if (size <= m_size) {
			if (data)
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
		}
		GL_CHECK();
		unbind();
	}
	void ShaderBuffer::getData(uint32_t size, void* outData, uint32_t offset) {
		if (size == 0) return;
		if (!initialized()) {
			log::out << log::RED << "ShaderBuffer::getData - object is uninitialized!\n";
			return;
		}
		bind();
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, outData);
		GL_CHECK();
		unbind();
	}
	void ShaderBuffer::bindBase(int index) const {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_id);
	}
	void ShaderBuffer::drawPoints(int count, int bindingIndex) {
		bind();
		bindBase(bindingIndex);
		glBindVertexArray(m_vaId);
		GL_CHECK();
		glDrawArrays(GL_POINTS, 0, count);
		GL_CHECK();
		unbind();
		glBindVertexArray(0);
	}
	void VertexArray::clear() {
		while (bufferSection > 0) {
			bufferSection--;
			strides[bufferSection] = 0;
			while (location > 0) {
				location--;
				locationSizes[location] = 0;
				glDisableVertexAttribArray(location);
			}
		}
		totalLocation = 0;
	}
	void VertexArray::addAttribute(u8 floatSize, u8 divisor) {
		if (!initialized())
			glGenVertexArrays(1, &m_id);

		if (location == 8) {
			log::out << log::RED << "VertexArray::addAttribute Limit of 8 locations!\n";
			return;
		}

		locationSizes[totalLocation++] = floatSize + (divisor << 4);
		strides[bufferSection] += floatSize;
	}
	void VertexArray::addAttribute(u8 floatSize, u8 divisor, VertexBuffer* buffer) {
		if (!buffer) {
			// this is fine incase you use instancing
			//log::out << log::RED << "VertexArray::addAttribute - buffer was nullptr\n";
			//return;
		}
		addAttribute(floatSize, divisor);
		if (location == 8)
			return;

		if (buffer) buffer->bind();
		bind();

		int offset = 0;
		startLocations[bufferSection] = location;
		while (totalLocation > location) {
			glEnableVertexAttribArray(location);

			u8 size = locationSizes[location] << 4;
			size = size >> 4;
			u8 div = locationSizes[location] >> 4;

			//log::out << location << " " << size << " " << div << " " << strides[bufferSection] << " " << offset << "\n";

			glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, strides[bufferSection] * sizeof(float), (void*)(offset * sizeof(float)));
			if (div != 0)
				glVertexAttribDivisor(location, div);

			offset += locationSizes[location];
			location++;
		}
		bufferSection++;
		GL_CHECK();
		if (buffer) buffer->bind();
		unbind();
	}
	void VertexArray::addAttribute(u8 floatSize) {
		addAttribute(floatSize, (u8)0);
	}
	void VertexArray::addAttribute(u8 floatSize, VertexBuffer* buffer) {
		addAttribute(floatSize, 0u, buffer);
	}
	void VertexArray::selectBuffer(u8 location, VertexBuffer* buffer) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::selectBuffer - object is uninitialized!\n";
			return;
		}
		if (!buffer) {
			log::out << log::RED << "VertexArray::selectBuffer - cannot select nullptr\n";
			return;
		}
		buffer->bind();
		bind();
		GL_CHECK();
		uint32_t offset = 0;
		uint32_t section = 0;
		while (section < MAX_BUFFERS) {
			if (startLocations[section] >= location)
				break;
			section++;
		}
		u8 index = startLocations[(int)section];
		while (offset == strides[(int)section] - locationSizes[startLocations[section]]) {
			offset += locationSizes[index];
			index++;
		}
		while (offset < strides[section]) {
			u8 size = locationSizes[location] << 4;
			size = size >> 4;
			u8 div = locationSizes[location] >> 4;

			//log::out << location << " " << size << " " << strides[section] << " " << offset << "\n";
			glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, strides[section] * sizeof(float), (void*)(offset * sizeof(float)));
			offset += size;
			location++;
		}
		GL_CHECK();
		buffer->bind(); // buffer is probably an instance buffer which needs to be bound before drawing. I did bind it a few lines up so may not need to bind again.
		unbind();
	}
	void VertexArray::bind() const {
		glBindVertexArray(m_id);
	}
	void VertexArray::unbind() const {
		glBindVertexArray(0);
	}
	
	void VertexArray::cleanup() {
		if (m_id != 0)
			glDeleteVertexArrays(1, &m_id);
	}
	void VertexArray::drawLines(IndexBuffer* indexBuffer) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::drawLines - object is uninitialized!\n";
			return;
		}
		bind();

		if (bufferSection == 0) {
			log::out << log::RED << "VertexArray::drawLines - You forgot VBO in addAttribute!\n";
		}

		if (indexBuffer != nullptr) {
			if (indexBuffer->initialized()) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->m_id);

				glDrawElements(GL_LINES, indexBuffer->getSize() / sizeof(uint32_t), GL_UNSIGNED_INT, nullptr);
			}
			else {
				log::out << log::RED << "VertexArray::drawLines - buffer is uninitialized!\n";
			}
		}
		else {
			log::out << log::RED << "VertexArray::drawLines - Must have indexBuffer when drawing!\n";
		}
		GL_CHECK();
		unbind();
	}
	void VertexArray::drawPoints(int count) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::drawLines - object is uninitialized!\n";
			return;
		}
		bind();

		if (bufferSection == 0) {
			log::out << log::RED << "VertexArray::drawPoints - You forgot VBO in addAttribute!\n";
		}
		//if (vertexBuffer != nullptr) {
		//	if (vertexBuffer->id != 0) {
		//		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->id);

		glDrawArrays(GL_POINTS, 0, count);
		//	}
		//}
		//else {
		//	log::out << log::RED << "VertexArray::drawPoints Must have vertexBuffer when drawing!\n";
		//}
		GL_CHECK();
		unbind();
	}
	void VertexArray::draw(IndexBuffer* indexBuffer) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::draw - object is uninitialized!\n";
			return;
		}
		bind();

		if (bufferSection == 0) {
			log::out << log::RED << "VertexArray::draw You forgot VBO in addAttribute!\n";
		}

		if (indexBuffer != nullptr) {
			if (indexBuffer->initialized()) {
				indexBuffer->bind();
				//glDrawElements(GL_TRIANGLES, indexBuffer->getSize(), GL_UNSIGNED_INT, nullptr);
				glDrawElements(GL_TRIANGLES, indexBuffer->getSize()/sizeof(uint32_t), GL_UNSIGNED_INT, nullptr);
			} else {
				log::out << log::RED << "VertexArray::draw - buffer is uninitialized!\n";
			}
		}
		else {
			log::out << log::RED << "VertexArray::draw Must have indexBuffer when drawing!\n";
		}
		GL_CHECK();
		unbind();
	}
	void VertexArray::draw(IndexBuffer* indexBuffer, uint32_t instanceAmount) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::draw - object is uninitialized!\n";
			return;
		}
		bind();

		if (indexBuffer != nullptr) {
			if (indexBuffer->initialized()) {
				indexBuffer->bind();
				glDrawElementsInstanced(GL_TRIANGLES, indexBuffer->getSize() / sizeof(uint32_t), GL_UNSIGNED_INT, nullptr, instanceAmount);
			}
			else {
				log::out << log::RED << "VertexArray::draw - buffer is uninitialized!\n";
			}
		}
		else {
			log::out << log::RED << "VertexArray::draw indexBuffer required when drawing instances!\n";
		}
		GL_CHECK();
		unbind();
	}
	void VertexArray::drawTriangleArray(int vertexCount) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::drawTriangleArray - object is uninitialized!\n";
			return;
		}
		bind();

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		GL_CHECK();
		unbind();
	}
	void VertexArray::drawLineArray(int vertexCount) {
		if (!initialized()) {
			log::out << log::RED << "VertexArray::drawLineArray - object is uninitialized!\n";
			return;
		}
		bind();

		glDrawArrays(GL_LINES, 0, vertexCount);

		int err = glGetError();
		if(err) {
			engone::log::out << engone::log::RED<<"GLError: "<<err<<" "<<(const char*)glewGetErrorString(err)<<"\n";DebugBreak();
		}

		GL_CHECK();
		unbind();
	}
	void Texture::cleanup() {
		if (m_id != 0)
			glDeleteTextures(1, &m_id);
	}
	void Texture::init(RawImage* image) {
		if (!image) {
			log::out << "Texture::init - image was null\n";
			return;
		}
		setData(image->data(),image->width, image->height);
	}
	// void Texture::init(char* data, int width, int height) {
	// 	if (!data) {
	// 		log::out << "Texture::init - data was null\n";
	// 		return;
	// 	}
	// 	m_width = width;
	// 	m_height = height;

	// 	glGenTextures(1, &m_id);
	// 	glBindTexture(GL_TEXTURE_2D, m_id);

	// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	// 	glBindTexture(GL_TEXTURE_2D, 0);
	// }
	void Texture::setData(char* data,int width, int height, int x, int y) {
		if (!initialized()) {
			if (width == 0 || height == 0) {
				log::out <<log::RED<< "Texture::setData - width or height cannot be 0\n";
				return;
			}
			m_width = width;
			m_height = height;

			glGenTextures(1, &m_id);
			bind();
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			unbind();
		} else {
			bind();
			glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			unbind();
		}
	}
	//void Texture::init(const char* path) {
	//	stbi_set_flip_vertically_on_load(1);
	//	buffer = stbi_load(path, &width, &height, &BPP, 4);

	//	glGenTextures(1, &m_id);
	//	glBindTexture(GL_TEXTURE_2D, m_id);

	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	//	glBindTexture(GL_TEXTURE_2D, 0);

	//	if (buffer)
	//		stbi_image_free(buffer);

	//}
	//void Texture::init(const void* inBuffer, uint32_t size) {
	//	stbi_set_flip_vertically_on_load(1);

	//	buffer = stbi_load_from_memory((const stbi_uc*)inBuffer, size, &width, &height, &BPP, 4);

	//	glGenTextures(1, &m_id);
	//	glBindTexture(GL_TEXTURE_2D, m_id);

	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	//	glBindTexture(GL_TEXTURE_2D, 0);

	//	if (buffer)
	//		stbi_image_free(buffer);
	//}
	//void Texture::init(int w, int h, void* data)
	//{
	//	width = w;
	//	height = h;
	//	glGenTextures(1, &id);
	//	glBindTexture(GL_TEXTURE_2D, id);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//}
	//void Texture::subData(int x, int y, int w, int h, void* data) {
	//	glBindTexture(GL_TEXTURE_2D, m_id);
	//	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//	glBindTexture(GL_TEXTURE_2D, 0);
	//}
	void Texture::bind(unsigned int slot) {
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_id);
	}
	void Texture::unbind() {
		//glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	int Texture::getWidth() const {
		return m_width;
	}
	int Texture::getHeight()const  {
		return m_height;
	}
	void FrameBuffer::init() {
		log::out << "FrameBuffer::init - code disabled\n";
		//glGenFramebuffers(1, &m_id);

		//glGenTextures(1, &m_textureId);
		//glBindTexture(GL_TEXTURE_2D, m_textureId);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		//	1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		//float color[] = { 1,1,1,1 };
		//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

		//glBindFramebuffer(GL_FRAMEBUFFER, m_id);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textureId, 0);
		//glDrawBuffer(GL_NONE);
		//glReadBuffer(GL_NONE);
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void FrameBuffer::initBlur(int width, int height) {
		glGenFramebuffers(1, &m_id);

		m_width = width;
		m_height = height;
		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//float color[] = { 1,1,1,1 };
		//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
		glGenRenderbuffers(1, &m_renderBufferId);
		glBindRenderbuffer(GL_RENDERBUFFER, m_renderBufferId);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureId, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_renderBufferId);

		// NOTE: applying a renderbuffer without creating and setting buffer will cause wierd things do happen with the buffer.
		//		Probably because it expects a depth stencil when it doens't exist.

		int err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (err != GL_FRAMEBUFFER_COMPLETE) {
			log::out << log::RED << "Error with framebuffer\n";
		}
		unbind();
	}
	void FrameBuffer::bind() const {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
	}
	void FrameBuffer::unbind() const {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	void FrameBuffer::cleanup() {
		if (m_textureId != 0)
			glDeleteTextures(1, &m_textureId);
		if (m_renderBufferId != 0)
			glDeleteRenderbuffers(1, &m_renderBufferId);
		if (m_id != 0)
			glDeleteFramebuffers(1, &m_id);
		//log::out << log::RED << "FrameBuffer:cleanup - not complete!\n"; // probably completed
	}
	void FrameBuffer::resize(int width, int height) {
		log::out << log::RED << "FrameBuffer::resize - not implemented\n";
		//bindTexture();
		//m_width = width;
		//m_height = height;
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		// needs to resize render buffer as well

	}
	void FrameBuffer::blitDepth(int width, int height) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_id);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, m_width, m_height,
			0, 0, width, height,
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		//GL_CHECK();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	void FrameBuffer::bindTexture() {
		glBindTexture(GL_TEXTURE_2D, m_textureId);
	}
	void FrameBuffer::bindRenderbuffer() {
		glBindRenderbuffer(GL_RENDERBUFFER, m_renderBufferId);
	}
	void Shader::bind() {
		//glLinkProgram(id);
		//glValidateProgram(id);
		glUseProgram(m_id);

	}
	void Shader::unbind() {
		glUseProgram(0);
	}
	void Shader::cleanup() {
		if(m_id!=0)
			glDeleteProgram(m_id);
	}
	void Shader::init(const char* source) {
		//std::string& vertex=vs, &fragment = fs;
		uint32_t sourceSize = strlen(source);

		const char* vTag = "#shader vertex\n";
		uint32_t vTagLen = strlen(vTag);
		uint32_t vIndex = 0;

		const char* fTag = "#shader fragment\n";
		uint32_t fTagLen = strlen(fTag);
		uint32_t fIndex = 0;

		enum State {
			ReadingNone,
			ReadingVertex,
			ReadingFragment,
		};
		State state = ReadingNone;

		vs = {};
		fs = {};

		int currentLine = 0;
		for (int i = 0; i < sourceSize; i++) {
			char chr = source[i];
			if (chr == '\n') currentLine++;
			if (chr == vTag[vIndex]) {
				vIndex++;
				if (vIndex == vTagLen) {
					vs.start = source + i + 1;
					vs.line = currentLine;
					state = ReadingVertex;
					if (state == ReadingFragment) {
						fs.length -= vTagLen;
					}
					continue;
				}
			}
			else {
				vIndex = 0;
			}
			if (chr == fTag[fIndex]) {
				fIndex++;
				if (fIndex == fTagLen) {
					fs.start = source + i + 1;
					fs.line = currentLine;
					if (state == ReadingVertex) {
						vs.length -= fTagLen;
					}
					state = ReadingFragment;
				}
			}
			else {
				fIndex = 0;
			}
			if (state == ReadingVertex) {
				vs.length++;
			}
			else if (state == ReadingFragment) {
				fs.length++;
			}
		}

		if (vs.length == 0 || fs.length == 0) {
			log::out << "Is this shader source correct?:\n" << source << "\n";
		}

		m_id = createShader(vs, fs);
	}
	unsigned int Shader::createShader(ShaderSource vertexSrc, ShaderSource fragmentSrc) {
		unsigned int program = glCreateProgram();
		unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
		unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

		if (vs == 0 || fs == 0) {
			// failed
			if(vs)
				glDeleteShader(vs);
			if(fs)
				glDeleteShader(fs);

			glDeleteProgram(m_id);
			return 0;
		}

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glValidateProgram(program);

		//int eh=0;
		//glGetProgramiv(program, GL_VALIDATE_STATUS, &eh);
		//log::out << "Error? " << eh<<"\n";

		glDeleteShader(vs);
		glDeleteShader(fs);

		return program;
	}
	unsigned int Shader::compileShader(unsigned int type, ShaderSource source)
	{
		unsigned int id = glCreateShader(type);
		glShaderSource(id, 1, &source.start, &source.length);
		glCompileShader(id);

		int result;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE) {
			//error = 2;
			int msgSize; // null terminate is included
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &msgSize);

			char* msg = (char*)Allocate(msgSize);// a bit extra for when memory is moved to fit in actual error line
			memset(msg, 0, msgSize);

			int length=0;
			glGetShaderInfoLog(id, msgSize, &length, msg); // length is alter and does no longer include null term char

			//std::cout << "Compile error with " << filePath << " (" << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << ")\n";

			// log::out << msg << "\n";

			// find where number starts
			int numStart = 0;
			while (numStart < length) {
				numStart++;
				if (msg[numStart - 1] == '(')
					break;
			}
			// set the null term character
			int index = numStart;
			while (index < length) {
				index++;
				if (msg[index - 1] == ')') {
					msg[index - 1] = 0;
					break;
				}
			}

			// This is broken
			int line = atoi(msg + numStart) + source.line;

			if (type == GL_VERTEX_SHADER)
				log::out << "Vertex Line:";
			if (type == GL_FRAGMENT_SHADER)
				log::out << "Fragment Line:";
			log::out << line << (msg + index);
			if (msg[length - 1] != '\n')
				log::out << "\n";

			glDeleteShader(id);

			Free(msg, msgSize);

			return 0;
		}

		return id;
	}
	void Shader::setFloat(const std::string& name, float f)
	{
		glUniform1f(getUniformLocation(name), f);
		GL_CHECK();
	}
	void Shader::setVec2(const std::string& name, glm::vec2 v)
	{
		// WARN IF NOT BOUND?
		//int num = glGetError();
		//const GLubyte* okay = glewGetErrorString(num);
		//std::cout << okay << "\n";
		glUniform2f(getUniformLocation(name), v.x, v.y);
		GL_CHECK();
	}
	void Shader::setIVec2(const std::string& name, glm::ivec2 v)
	{
		glUniform2i(getUniformLocation(name), v.x, v.y);
		GL_CHECK();
	}
	void Shader::setVec3(const std::string& name, glm::vec3 v)
	{
		glUniform3f(getUniformLocation(name), v.x, v.y, v.z);
		GL_CHECK();
	}
	void Shader::setIVec3(const std::string& name, glm::ivec3 v)
	{
		glUniform3i(getUniformLocation(name), v.x, v.y, v.z);
		GL_CHECK();
	}
	void Shader::setVec4(const std::string& name, glm::vec4 v)
	{
		glUniform4f(getUniformLocation(name), v.x, v.y, v.z, v.w);
		GL_CHECK();
	}
	void Shader::setInt(const std::string& name, int v)
	{
		glUniform1i(getUniformLocation(name), v);
		GL_CHECK();
	}
	void Shader::setMat4(const std::string& name, glm::mat4 mat)
	{
		glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
		GL_CHECK();
	}
	unsigned int Shader::getUniformLocation(const std::string& name)
	{
		if (uniLocations.find(name) != uniLocations.end()) {
			return uniLocations[name];
		}
		//int num = glGetError();
		//const GLubyte* okay = glewGetErrorString(num);
		//std::cout << num << "\n";
		unsigned int loc = glGetUniformLocation(m_id, name.c_str());
		// int num = glGetError();
		// const GLubyte* okay = glewGetErrorString(num);
		// log::out << num << " "<<(const char*)okay<<"\n";
		uniLocations[name] = loc;
		return loc;
	}
}