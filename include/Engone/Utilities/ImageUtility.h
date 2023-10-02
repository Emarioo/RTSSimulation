#pragma once

/*
NOTE:
First of. The current structure isn't great. The images are structs on the stack which gets moved around.
Dealing with move constructors, move assignment, copy constructor and all that is annoying.
Allocations aren't freed properly because of this.
So maybe do heap and pointers instead.

Another thing, add images, formats as they are needed. Don't add everything at once.
For example. PNG to ICO conversion is useful. RawImage is just generally useful.

PNG to ICO converts png -> raw -> bmp -> ico. You might be able to just do png -> ico because
ico can store png in it's entirety. Read about the ICO format and do some testing.

*/

#include "Engone/Win32Includes.h"
#include "Engone/Typedefs.h"
#include <string>

namespace engone {
	class Image;
	class RawImage;
	class PNG;
	class BMP;
	class ICO;

	enum ImageFlag : u8 {
		FlipOnLoad = 4,
		RGBA = 8,
	};
	typedef u8 ImageFlags;
	class Image {
	public:

		Image() = default;
		~Image() {cleanup(); }

		void writeFile(const char* path);

		// T needs to be a class derived from Image.
		// Flags of out are ignored.
		template<typename T>
		static T* ReadFile(const char* path);

		template<typename A, typename B>
		static B* Convert(A* img);

		void cleanup();

		inline bool valid() const { return m_data!=nullptr; }

		inline char* data() const { return m_data; }

		void setFlags(ImageFlags flags, bool yes);

	// protected:
		enum InternalFlag : u8 {
			OwnerSelf = 1,
			OwnerStbi = 2,
		};

		ImageFlags m_flags=0;
		uint32_t m_size=0;
		char* m_data=nullptr;
		
		static ImageFlags StripInternal(ImageFlags flags);
	};

	class RawImage : public Image {
	public:
		RawImage() = default;
		
		// channels=bitsPerPixel/8
		static RawImage* ReadFromPNG(const std::string& path, ImageFlags flags = RGBA);
		static RawImage* LoadFromPNG(int id, ImageFlags flags = RGBA);
		// based on width, height and channels.
		
		// Image can be turned into BMP
		//template<typename T=BMP>
		//T toBMP();

		int width = 0, height = 0, channels = 4; // also known as bits per pixel. except that it's in bytes.
	};
	// supposed to be png data structure
	class PNG : public Image {
	public:
		PNG() = default;

		// load png resource from executable (windows).
		static PNG* Load(int id);
		static PNG* ReadFile(const char* path);
		//// uses stbi
		//RawImage toRaw(int channels=4);

	};
	RawImage* PNGToRawImage(PNG* img);
	// mainly used for ICO.
	// BITMAPINFOHEADER is the start. There are others.
	// BMP format from wikipedia
	class BMP : public Image {
	public:
		BMP() = default;

		static BMP* ReadFile(const char* path);

		BITMAPINFOHEADER* header() {
			return (BITMAPINFOHEADER*)m_data;
		}
		char* getData() {
			return m_data + sizeof(BITMAPINFOHEADER);
		}
		// image data is formatted to top-down rgba
		// bmp is assumed to be bottom up, argb (bgra if you move from left to right)
		//RawImage toRawImage();

		//template<typename T=ICO>
		//T toICO();

	};
	// from wikipedia ICO file format
	// don't touch the zero elements. They should be zero
	// Image data Has BMP implementation. Not PNG.(you can still convert png to raw to ico)
	class ICO : public Image {
	public:
		ICO() = default;

		// resource from executable
		static ICO* Load(int id);
		static ICO* CreateEmpty(uint32_t imageCount, uint32_t size);
		static ICO* ReadFile(const char* path);

		// allocates memory based on arguments. You need to set some details after.
		// assumes ico and not cur format. You can of course change it.

		struct ICONDIR {
			char zero[2]; // always 0
			uint16_t type; // 1 for icon, 2 for cursor
			uint16_t numImages; // number of images
		};
		struct ICONDIRENTRY {
			u8 width;
			u8 height;
			u8 colorPalette; // 0 if no color palette
			char zero;
			short colorPlanes; // not sure. 0 or 1 is normal.
			short bitsPerPixel;
			uint32_t imageSize; // in bytes
			uint32_t offset; // offset from the beginning.
		};
		ICONDIR* header() {
			return (ICONDIR*)m_data;
		}
		ICONDIRENTRY* entry(uint16_t entryIndex) {
			return (ICONDIRENTRY*)(m_data + sizeof(ICONDIR) + entryIndex * sizeof(ICONDIRENTRY));
		}
		// will set automatically set image offset for all entries.
		// set entry info first. (imageSize is required to calculate this)
		void autoSetOffsets();

		char* getData(uint16_t entryIndex) {
			return (m_data + entry(entryIndex)->offset);
		}

		//// Note that bmp doesn't have ownership of data
		//BMP* toBMP(uint32_t entryIndex) {
		//	BMP img;
		//	img.data = (data + entry(entryIndex)->offset);
		//	img.size = entry(entryIndex)->imageSize;
		//	img.flags = 0;
		//	return img;
		//}
	};
}