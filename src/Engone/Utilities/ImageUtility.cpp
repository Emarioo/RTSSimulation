#include "Engone/Utilities/ImageUtility.h"

// #include "Engone/Utilities/Alloc.h"
#include "Engone/Logger.h"
#include "stb/stb_image.h"

#include "Engone/PlatformLayer.h"

#include "Engone/util/Tracker.h"

namespace engone {
	template<>
	ICO* Image::Convert<PNG,ICO>(PNG* img) {
		log::out << "Image::Convert - PNG to ICO is not implemented\n";
		return nullptr;
	}
	template<>
	RawImage* Image::Convert < BMP,RawImage > (BMP* img) {
		if (!img) return nullptr;
		if (!img->m_data) return nullptr;

		RawImage* out = TRACK_ALLOC(RawImage);

		char* imgData = img->m_data + sizeof(BITMAPINFOHEADER);

		out->m_data = (char*)Allocate(img->header()->biSizeImage);
		if (!img->m_data) {
			log::out << log::RED << "BMP::toRawImage - failed allocation\n";
			return {};
		}
		out->m_size = img->header()->biSizeImage;
		out->width = img->header()->biWidth;
		out->height = img->header()->biHeight / 2; // bmp has double size for and mask
		out->channels = img->header()->biBitCount / 8;
		for (int i = 0; i < out->m_size / 4; i++) {
			int p = out->m_size / 4 - 1 - i;
			//bgra -> rgba
			out->m_data[p * 4 + 2] = imgData[i * 4 + 0];
			out->m_data[p * 4 + 1] = imgData[i * 4 + 1];
			out->m_data[p * 4 + 0] = imgData[i * 4 + 2];
			out->m_data[p * 4 + 3] = imgData[i * 4 + 3];
		}

		out->m_flags = OwnerSelf;
		return out;
	}
	template<>
	BMP* Image::Convert<RawImage,BMP>(RawImage* img) {
		if (!img) return nullptr;
		if (!img->m_data) return nullptr;

		uint32_t andMaskSize = img->width * img->height / 8;
		//uint32_t andMaskSize = 0;
		uint32_t total = sizeof(BITMAPINFOHEADER) + img->m_size + andMaskSize;
		char* newData = (char*)Allocate(total);
		if (!newData) {
			log::out << log::RED << "BMP image failed allocating memory\n";
			return nullptr;
		}
		BMP* out = TRACK_ALLOC(BMP);
		new(out)BMP();
		out->m_data = newData;
		out->m_flags = OwnerSelf;
		out->m_size = total;
		memset(out->m_data, 0, sizeof(BITMAPINFOHEADER));
		if (andMaskSize != 0)
			//memset(out->data + total - andMaskSize, 255, andMaskSize);
			memset(out->m_data + total - andMaskSize, 0, andMaskSize);

		out->header()->biSize = sizeof(BITMAPINFOHEADER);
		out->header()->biSizeImage = img->m_size;
		out->header()->biClrUsed = 0;
		out->header()->biClrImportant = 0;
		out->header()->biCompression = 0;
		out->header()->biPlanes = 1;
		out->header()->biXPelsPerMeter = 0;
		out->header()->biYPelsPerMeter = 0;

		out->header()->biWidth = img->width;
		out->header()->biHeight = img->height * 2; // double because of and mask
		out->header()->biBitCount = img->channels * 8;

		//auto a = out->header(); // debug

		//memcpy_s(out->m_data+sizeof(BITMAPINFOHEADER), img->m_size, img->data(), img->m_size);
		char* dstData = out->m_data + sizeof(BITMAPINFOHEADER);

		for (int i = 0; i < img->m_size / 4; i++) {
			//int p = img->m_size / 4 - 1 - i; // whether you want image flipped
			int p = i;
			//rgba -> bgra
			dstData[p * 4 + 2] = img->data()[i * 4 + 0];
			dstData[p * 4 + 1] = img->data()[i * 4 + 1];
			dstData[p * 4 + 0] = img->data()[i * 4 + 2];
			dstData[p * 4 + 3] = img->data()[i * 4 + 3];
			//dstData[p * 4 + 0] = img->data()[i * 4 + 0];
			//dstData[p * 4 + 1] = img->data()[i * 4 + 1];
			//dstData[p * 4 + 2] = img->data()[i * 4 + 2];
			//dstData[p * 4 + 3] = img->data()[i * 4 + 3];
		}

		return out;
	}
	RawImage* PNGToRawImage(PNG* img){
	// template<>
	// RawImage* Image::Convert<PNG,RawImage>(PNG* img){
		int channels = 0;
		//if (img->m_flags & RGBA)
		channels = 4;
		int realChannels;
		int width, height;
		stbi_set_flip_vertically_on_load(img->m_flags & FlipOnLoad);
		char* newData = (char*)stbi_load_from_memory((u8*)img->data(), img->m_size, &width, &height, &realChannels, channels);
		if (!newData) {
			return nullptr;
		}
		RawImage* out = TRACK_ALLOC(RawImage);
		new(out)RawImage();
		out->m_data = newData;
		out->width = width;
		out->height = height;
		out->channels = channels;
		out->m_size = out->width * out->height * out->channels;
		out->m_flags = Image::StripInternal(img->m_flags) | Image::OwnerStbi;
		return out;
	}
	template<>
	ICO* Image::Convert<BMP, ICO>(BMP* img) {
		if (!img) return nullptr;
		if (!img->data()) return nullptr;

		ICO* out = ICO::CreateEmpty(1,img->m_size);
		ICO::ICONDIRENTRY* e = out->entry(0);
		e->bitsPerPixel = img->header()->biBitCount;
		e->colorPalette=img->header()->biClrUsed;
		e->colorPlanes=0;
		//e->colorPlanes=header()->biPlanes;
		e->width = img->header()->biWidth;
		e->height = img->header()->biHeight/2;
		e->imageSize = img->m_size;
		out->autoSetOffsets();
		memcpy_s(out->getData(0),e->imageSize,img->data(),img->m_size);
		return out;
	}
	template<>
	BMP* Image::Convert<ICO, BMP>(ICO* img) {
		if (!img) return nullptr;
		if (!img->data()) return nullptr;
		if (img->header()->numImages == 0) return nullptr;

		uint32_t size = img->entry(0)->imageSize;

		char* newData = (char*)Allocate(size);
		if (!newData) {
			log::out << log::RED << "BMP image failed allocating memory\n";
			return nullptr;
		}
		BMP* out = TRACK_ALLOC(BMP);
		new(out)BMP();
		out->m_data = newData;
		out->m_flags = OwnerSelf;
		out->m_size = size;

		memcpy_s(out->m_data, size, img->getData(0), size);
		return out;
	}
	template<typename T>
	T* Image::ReadFile(const char* path) {
		if (sizeof(T) != sizeof(Image)) {
			log::out << log::RED << "Image::ReadFile - bad image template\n";
			return nullptr;
		}

		u64 size;
		auto file = engone::FileOpen(path, &size, engone::FILE_ONLY_READ);
		if(!file)
			return nullptr;

		char* newData = (char*)Allocate(size);
		if (!newData) {
			log::out << log::RED << "Image::ReadFile - failed allocating memory\n";
			engone::FileClose(file);
			return nullptr;
		}
		T* img = TRACK_ALLOC(T);
		new(img)T();
		img->m_data = newData;
		img->m_size = size;
		img->m_flags = StripInternal(img->m_flags) | OwnerSelf;

		engone::FileRead(file, img->m_data,size);

		engone::FileClose(file);
		return img;
	}
	void Image::writeFile(const char* path) {
		if (!m_data) return;
		
		auto file = engone::FileOpen(path, 0, engone::FILE_ALWAYS_CREATE);
		if(!file) return;

		engone::FileWrite(file, m_data, m_size);
		engone::FileClose(file);
	}
	ImageFlags Image::StripInternal(ImageFlags flags) {
		return flags & (~(OwnerSelf | OwnerStbi));
	}
	void Image::setFlags(ImageFlags flags, bool yes) {
		if (yes) {
			m_flags = m_flags | flags;
		} else {
			m_flags = m_flags & (~flags);
		}
	}
	void Image::cleanup() {
		if (m_data) {
			if (m_flags & OwnerSelf) {
				Free(m_data, m_size);
			}
			else if (m_flags & OwnerStbi) {
				stbi_image_free(m_data);
			}
		}
		m_data = nullptr;
		m_size = 0;
		m_flags = 0;
	}
	BMP* BMP::ReadFile(const char* path) { return Image::ReadFile<BMP>(path); }
	RawImage* RawImage::ReadFromPNG(const std::string& path, ImageFlags flags) {
		int channels = 0;
		if (flags & RGBA)
			channels = 4;
		int realChannels;
		int width, height;
		stbi_set_flip_vertically_on_load(flags & FlipOnLoad);
		char* newData = (char*)stbi_load(path.c_str(), &width, &height, &realChannels, channels);
		if (!newData) {
			return nullptr;
		}
		RawImage* img = TRACK_ALLOC(RawImage);
		new(img)RawImage();

		img->m_data = newData;
		img->width = width;
		img->height = height;
		img->channels = channels;
		img->m_size = img->width * img->height * img->channels;
		img->m_flags = StripInternal(flags) | OwnerStbi;
		return img;
	}
	RawImage* RawImage::LoadFromPNG(int id, ImageFlags flags) {
		HRSRC hs = FindResourceA(NULL, MAKEINTRESOURCEA(id), "PNG"); // Todo: Use FindResourceW.
		HGLOBAL hg = LoadResource(NULL, hs);
		void* ptr = LockResource(hg);
		DWORD size = SizeofResource(NULL, hs);

		int channels = 0;
		if (flags & RGBA)
			channels = 4;
		int realChannels;
		int width, height;
		stbi_set_flip_vertically_on_load(flags & FlipOnLoad);
		char* newData = (char*)stbi_load_from_memory((u8*)ptr, size, &width, &height, &realChannels, channels);
		if (!newData) {
			return nullptr;
		}
		RawImage* img = TRACK_ALLOC(RawImage);
		new(img)RawImage();
		img->m_data = newData;
		img->width = width;
		img->height = height;
		img->channels = channels;
		img->m_size = img->width * img->height * img->channels;
		img->m_flags = StripInternal(flags) | OwnerStbi;
		return img;
	}
	PNG* PNG::Load(int id) {
		HRSRC hs = FindResourceA(NULL, MAKEINTRESOURCEA(id), "PNG");
		if (!hs) {
			int err = GetLastError();
			printf("ICO::Load - windows error %d\n", err);
		}
		HGLOBAL hg = LoadResource(NULL, hs);
		void* ptr = LockResource(hg);
		DWORD size = SizeofResource(NULL, hs);

		PNG* img = TRACK_ALLOC(PNG);
		new(img)PNG();
		img->m_data = (char*)ptr;
		img->m_size = size;
		img->m_flags = 0;

		return img;
	}
	PNG* PNG::ReadFile(const char* path) { return Image::ReadFile<PNG>(path); }
	ICO* ICO::Load(int id) {
		//HANDLE hnd = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(id), IMAGE_ICON, 0, 0, 0);

		HRSRC hs = FindResourceA(NULL, MAKEINTRESOURCEA(id), "ICO");
		if (!hs) {
			int err = GetLastError();
			printf("ICO::Load - windows error %d\n", err);
		}
		HGLOBAL hg = LoadResource(NULL, hs);
		void* ptr = LockResource(hg);
		DWORD size = SizeofResource(NULL, hs);

		ICO* img = TRACK_ALLOC(ICO);
		new(img)ICO();
		img->m_data = (char*)ptr;
		img->m_size = size;
		img->m_flags = 0;

		//img->m_data = (char*)hnd;
		//img->m_size = 64*64*4;

		return img;
	}
	ICO* ICO::CreateEmpty(uint32_t imageCount, uint32_t size) {
		uint32_t total = sizeof(ICONDIR) + imageCount * sizeof(ICONDIRENTRY) + size;
		char* newData = (char*)Allocate(total);
		if (!newData) {
			log::out << log::RED << "ICO::CreateEmpty - image failed allocating memory\n";
			return {};
		}
		ICO* img = TRACK_ALLOC(ICO);
		new(img)ICO();
		img->m_data = newData;
		img->m_flags = OwnerSelf;
		img->m_size = total;
		memset(img->m_data, 0, sizeof(ICONDIR) + imageCount * sizeof(ICONDIRENTRY));
		img->header()->numImages = imageCount;
		img->header()->type = 1;
		return img;
	}
	ICO* ICO::ReadFile(const char* path) { return Image::ReadFile<ICO>(path); }
	void ICO::autoSetOffsets() {
		if (!m_data) return;
		uint32_t offset = sizeof(ICONDIR) + header()->numImages * sizeof(ICONDIRENTRY);
		for (int i = 0; i < header()->numImages; i++) {
			entry(i)->offset = offset;
			offset += entry(i)->imageSize;
		}
	}
}