/*
	ILB2PNG - Extracts images from an AoW1 ILB file and converts them to PNG
*/

#include <array>
#include <vector>
#include <iostream>
#include <memory>
#include <fstream>
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef std::array<char, 1024> Palette;

void translate16(void *pixelData, size_t dataW, size_t dataH, size_t xoff, size_t yoff, uint8_t* pngData, size_t pngW, size_t pngH, uint32_t transparent = 0xFFFFFFFF, uint32_t blend = 0);
void translateRLE16(void *pixelData, size_t dataW, size_t dataH, size_t xoff, size_t yoff, uint8_t* pngData, size_t pngW, size_t pngH, uint32_t transparent = 0xFFFFFFFF, uint32_t blend = 0);
void translateRLE8(void *pixelData, size_t dataW, size_t dataH, size_t xoff, size_t yoff, uint8_t* pngData, size_t pngW, size_t pngH, std::shared_ptr<Palette> pallete, uint32_t transparent, uint32_t mode);

struct Image
{
	size_t width;
	size_t height;
	size_t xoff;
	size_t yoff;
	uint8_t *data;
	uint32_t mode;
	std::string name;

	Image(size_t width, size_t height)
	{
		this->width = width;
		this->height = height;

		this->data = new uint8_t[width * height * 4];
		memset(this->data, 0, width * height * 4);
	}

	~Image()
	{
		delete[] data;
	}

	void composite(std::shared_ptr<Image> other)
	{
		// I hope this.width >= other.width always holds true...
		size_t thisOffset;
		size_t otherOffset;
		float sr = 0;
		float sg = 0;
		float sb = 0;
		float sa = 0;

		float dr = 0;
		float dg = 0;
		float db = 0;
		float da = 0;

		float inva = 0;
		float outa = 0;

		for (size_t y = 0; y < other->height; ++y)
		{
			for (size_t x = 0; x < other->width; ++x)
			{
				thisOffset = 4 * (x + other->xoff) + (4 * (y + other->yoff) * this->width);
				otherOffset = 4 * x + (4 * y * other->width);
				
				sr = other->data[otherOffset + 0];
				sg = other->data[otherOffset + 1];
				sb = other->data[otherOffset + 2];
				sa = other->data[otherOffset + 3];

				// Ignore completely transparent pixels
				if (sa == 0)
					continue;

				// Don't bother with alpha for completely opaque pixels either
				if (sa == 255)
				{
					this->data[thisOffset + 0] = (uint8_t)sr;
					this->data[thisOffset + 1] = (uint8_t)sg;
					this->data[thisOffset + 2] = (uint8_t)sb;
					this->data[thisOffset + 3] = (uint8_t)sa;
					continue;
				}

				sr /= 255.0f;
				sg /= 255.0f;
				sb /= 255.0f;
				sa /= 255.0f;

				inva = 1 - sa;

				dr = this->data[thisOffset + 0] / 255.0f;
				dg = this->data[thisOffset + 1] / 255.0f;
				db = this->data[thisOffset + 2] / 255.0f;
				da = this->data[thisOffset + 3] / 255.0f;

				outa = (sa + (da * inva));

				if (outa)
				{
					dr = (sr * sa + dr * da * inva) / outa;
					dg = (sg * sa + dg * da * inva) / outa;
					db = (sb * sa + db * da * inva) / outa;
					da = outa;
				}
				else
				{
					dr = dg = db = 0;
				}

				this->data[thisOffset + 0] = (uint8_t)(dr * 255.0f);
				this->data[thisOffset + 1] = (uint8_t)(dg * 255.0f);
				this->data[thisOffset + 2] = (uint8_t)(db * 255.0f);
				this->data[thisOffset + 3] = (uint8_t)(da * 255.0f);
			}
		}
	}
};

std::shared_ptr<Image> readType8(std::fstream &ilbFile, uint32_t imgDirectory, std::vector< std::shared_ptr<Palette> > &palettes);
std::shared_ptr<Image> readType16(std::fstream &ilbFile, uint32_t imgDirectory, bool sprite = false, bool isRLE = false, bool isTransparent = false);

int main(int argc, char* *argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: ilb2png <ilbfile> [outdir]" << std::endl;
		return 0;
	}

	std::filesystem::path ilbPath = argv[1];
	std::filesystem::path outDir = "";

	if (argc < 3)
	{
		outDir = std::filesystem::current_path();
		outDir /= ilbPath.stem();
	}
	else
	{
		outDir = argv[2];
	}

	if (!std::filesystem::is_regular_file(ilbPath))
	{
		std::cerr << "[ERR ] File does not exist: " << ilbPath.string() << std::endl;
		return -1;
	}

	std::fstream ilbFile(ilbPath, std::fstream::in | std::fstream::binary);

	if (!std::filesystem::is_directory(outDir))
	{
		try
		{
			std::filesystem::create_directories(outDir);
		}
		catch (std::filesystem::filesystem_error &e)
		{
			std::cerr << "[ERR ] Failed to create output directory " << outDir.string() << ": " << e.what() << std::endl;
			return -2;
		}
	}

	// If we're here, it should be good filesystem-wise from here on out.
	uint32_t magic = 0;
	// Magic should be 0x424C4904, or 0x04,'ILB'
	ilbFile.read((char*)&magic, sizeof(uint32_t));

	// Unknown, seemingly useless value
	uint32_t unkID = 0;
	ilbFile.read((char*)&unkID, sizeof(uint32_t));

	// Float because idklol
	float version = 0;
	ilbFile.read((char*)&version, sizeof(float));

	// Length of the header even though it doesn't change within versions
	uint32_t headerLength = 0;
	ilbFile.read((char*)&headerLength, sizeof(uint32_t));

	uint32_t imgDirectory = 0;
	uint32_t fileSize = 0;
	uint32_t paletteCount = 0;

	if (version == 4.0f)
	{
		ilbFile.read((char*)&imgDirectory, sizeof(uint32_t));
		ilbFile.read((char*)&fileSize, sizeof(uint32_t));
	}

	std::cout << "Reading " << ilbPath.string() << " - ID " << unkID << std::endl;
	
	ilbFile.read((char*)&paletteCount, sizeof(uint32_t));

	if(paletteCount != 1)
		std::cout << "There are " << paletteCount << " palettes." << std::endl;
	else
		std::cout << "There  is 1 palette." << std::endl;

	std::vector< std::shared_ptr<Palette> > palettes;
	uint32_t palType = 0;
	for (size_t i = 0; i < paletteCount; ++i)
	{
		std::shared_ptr<Palette> palette = std::make_shared<Palette>();
		palettes.push_back(palette);

		ilbFile.read((char*)&palType, sizeof(uint32_t));

		if (palType != 0x88801B18)
		{
			std::cerr << "[ERR ] Unknown palette type!" << std::endl;
			return -3;
		}

		// As far as anyone has been able to find, the palettes have only ever been
		// r8g8b8x8
		ilbFile.read((*palette).data(), 1024);
	}

	// Now we have the image list
	uint32_t imageID = 0;
	bool composite = false;

	do
	{
		std::shared_ptr<Image> image;

		ilbFile.read((char*)&imageID, sizeof(uint32_t));
		if (imageID == 0xFFFFFFFF)
			break;

		uint32_t type = 0;
		do
		{
			std::shared_ptr<Image> layer;

			ilbFile.read((char*)&type, sizeof(uint32_t));

			// If the type is 256, it's a composite image
			// we still need the *actual* type though, so read that too
			if (type == 256)
			{
				composite = true;
				ilbFile.read((char*)&type, sizeof(uint32_t));
			}
			else
			{
				composite = false;
			}

			if (type == 0xFFFFFFFF)
			{
				break;
			}

			std::cout << "Reading type " << type << " image for ID " << imageID << std::endl;

			switch (type)
			{
			case 0:
				// Empty image
				if (image)
					std::cout << "End of composite image " << imageID << std::endl;
				else
					std::cout << "Empty Image." << std::endl;
				break;
			case 2:
				layer = readType8(ilbFile, imgDirectory, palettes);
				break;
			case 16:
				layer = readType16(ilbFile, imgDirectory);
				break;
			case 17:
				layer = readType16(ilbFile, imgDirectory, true, true, false);
				break;
			case 18:
				layer = readType16(ilbFile, imgDirectory, true, true, true);
				break;
			case 22:
				layer = readType16(ilbFile, imgDirectory, true);
				break;
			default:
				std::cout << "Unhandled type " << type << std::endl;
				char end = 0;
				char scan = 0;
				do
				{
					ilbFile.read(&scan, 1);
					if (ilbFile.eof())
					{
						std::cerr << "Ran off the end of the file!" << std::endl;
						return -5;
					}

					if (scan == -1)
						end++;
					else
						end = 0;

				} while (end < 4);
				type = 0xFFFFFFFF;
			}

			if (layer)
			{
				if (image)
					image->composite(layer);
				else
				{
					std::cout << "Image " << imageID << ": " << layer->name << " is " << layer->width << " x " << layer->height << " and has a blend mode of " << (layer->mode & 0x00FF) << ":" << ((layer->mode >> 8) & 0x00FF) << std::endl;
					image = layer;
				}
			}

		} while (type != 0xFFFFFFFF);
		
		if (image)
		{
			std::string filename = (outDir / (std::to_string(imageID) + ".png")).string();
			std::cout << "Writing " << filename << std::endl;
			stbi_write_png(filename.c_str(), image->width, image->height, 4, image->data, 4 * image->width);
		}

	} while (!ilbFile.eof());

	std::cout << "Done." << std::endl;

	return 0;
}

struct CommonInfo
{
	char infoByte;
	std::string imageName;
	uint32_t width;
	uint32_t height;
	uint32_t xshift;
	uint32_t yshift;

	uint32_t subID;

	uint32_t size;
	uint32_t offset;

	uint32_t totalW;
	uint32_t totalH;

	uint32_t drawmode;
	uint32_t blendValue;

	uint32_t colorset;
};

std::shared_ptr<CommonInfo> readCommonInfo(std::fstream &ilbFile, bool pal = false)
{
	std::shared_ptr<CommonInfo> info = std::make_shared<CommonInfo>();

	ilbFile.read(&info->infoByte, 1);

	uint32_t nameLength = 0;
	ilbFile.read((char*)&nameLength, sizeof(uint32_t));

	char nameByte = 0;
	for (uint32_t i = 0; i < nameLength; ++i)
	{
		ilbFile.read(&nameByte, 1);
		info->imageName.append(1, nameByte);
	}

	ilbFile.read((char*)&info->width, sizeof(uint32_t));
	ilbFile.read((char*)&info->height, sizeof(uint32_t));

	ilbFile.read((char*)&info->xshift, sizeof(uint32_t));
	ilbFile.read((char*)&info->yshift, sizeof(uint32_t));

	ilbFile.read((char*)&info->subID, sizeof(uint32_t));

	char unknownA = 0;
	ilbFile.read(&unknownA, 1);

	ilbFile.read((char*)&info->size, sizeof(uint32_t));

	info->offset = 0;
	if (info->infoByte != 1)
		ilbFile.read((char*)&info->offset, sizeof(uint32_t));

	ilbFile.read((char*)&info->totalW, sizeof(uint32_t));
	ilbFile.read((char*)&info->totalH, sizeof(uint32_t));

	info->drawmode = 0;
	info->blendValue = 0;

	if (pal)
	{
		char unknownB = 0;
		ilbFile.read(&unknownB, 1);
	}
	
	if (info->infoByte == 3)
	{
		ilbFile.read((char*)&info->drawmode, sizeof(uint32_t));
		ilbFile.read((char*)&info->blendValue, sizeof(uint32_t));
	}

	ilbFile.read((char*)&info->colorset, sizeof(uint32_t));
	
	return info;
}

struct SpriteInfo
{
	uint32_t clipW;
	uint32_t clipH;
	uint32_t clipX;
	uint32_t clipY;
	uint32_t trans;
};

std::shared_ptr<SpriteInfo> readSpriteInfo(std::fstream &ilbFile)
{
	std::shared_ptr<SpriteInfo> info = std::make_shared<SpriteInfo>();

	ilbFile.read((char*)&info->clipW, sizeof(uint32_t));
	ilbFile.read((char*)&info->clipH, sizeof(uint32_t));

	ilbFile.read((char*)&info->clipX, sizeof(uint32_t));
	ilbFile.read((char*)&info->clipY, sizeof(uint32_t));

	ilbFile.read((char*)&info->trans, sizeof(uint32_t));

	return info;
}

std::shared_ptr<Image> readType16(std::fstream &ilbFile, uint32_t imgDirectory, bool isSprite, bool isRLE, bool isTransparent)
{
	std::shared_ptr<CommonInfo> info = readCommonInfo(ilbFile);
	std::shared_ptr<SpriteInfo> sprite;
	if (isSprite)
		sprite = readSpriteInfo(ilbFile);

	uint32_t unknown = 0;
	if (isRLE)
		ilbFile.read((char*)&unknown, sizeof(uint32_t));

	// Here on out is type-specific

	bool reseek = info->infoByte != 1;

	std::streampos oldpos = 0;

	if (reseek)
	{
		oldpos = ilbFile.tellg();
		ilbFile.seekg(info->offset + imgDirectory, std::fstream::beg);
	}

	// Read the image data

	char *imgData = new char[info->size];
	ilbFile.read(imgData, info->size);

	std::shared_ptr<Image> img = std::make_shared<Image>(info->totalW, info->totalH);
	img->xoff = info->xshift;
	img->yoff = info->yshift;
	img->name = info->imageName;

	if (isTransparent)
		img->mode = 0x0001;
	else
		img->mode = info->drawmode;

	switch (info->colorset)
	{
	case 0x56509310:
		if (isSprite)
		{
			if (isRLE)
				translateRLE16(imgData, sprite->clipW, sprite->clipH, sprite->clipX, sprite->clipY, img->data, info->totalW, info->totalH, sprite->trans, info->drawmode | (info->blendValue << 16));
			else
				translate16(imgData, sprite->clipW, sprite->clipH, sprite->clipX, sprite->clipY, img->data, info->totalW, info->totalH, sprite->trans, info->drawmode | (info->blendValue << 16));
		}
		else
			translate16(imgData, info->width, info->height, info->xshift, info->yshift, img->data, info->totalW, info->totalH, 0xFFFFFFFF, info->drawmode | (info->blendValue << 16));
		break;
	default:
		std::cout << "Unknown pixel format!";
	}

	delete[] imgData;

	if (reseek)
	{
		ilbFile.seekg(oldpos);
	}

	return img;
}

std::shared_ptr<Image> readType8(std::fstream &ilbFile, uint32_t imgDirectory, std::vector< std::shared_ptr<Palette> > &palettes)
{
	std::shared_ptr<CommonInfo> info = readCommonInfo(ilbFile, true);
	std::shared_ptr<SpriteInfo> sprite = readSpriteInfo(ilbFile);

	uint32_t unknown = 0;
	ilbFile.read((char*)&unknown, sizeof(uint32_t));

	// Here on out is type-specific

	bool reseek = info->infoByte != 1;

	std::streampos oldpos = 0;

	if (reseek)
	{
		oldpos = ilbFile.tellg();
		ilbFile.seekg(info->offset + imgDirectory, std::fstream::beg);
	}

	// Read the image data

	char *imgData = new char[info->size];
	ilbFile.read(imgData, info->size);

	std::shared_ptr<Image> img = std::make_shared<Image>(info->totalW, info->totalH);
	img->xoff = info->xshift;
	img->yoff = info->yshift;
	img->name = info->imageName;

	translateRLE8(imgData, sprite->clipW, sprite->clipH, sprite->clipX, sprite->clipY, img->data, info->totalW, info->totalH, palettes[info->colorset], sprite->trans, info->drawmode | (info->blendValue << 16));

	delete[] imgData;

	if (reseek)
	{
		ilbFile.seekg(oldpos);
	}

	return img;
}

void translate16(void *pixelData, size_t dataW, size_t dataH, size_t xoff, size_t yoff, uint8_t* pngData, size_t pngW, size_t pngH, uint32_t transparent, uint32_t mode)
{
	uint16_t *data = static_cast<uint16_t*>(pixelData);

	uint32_t pixel = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;

	int show = mode & 0x00FF;
	int blend = (mode >> 8) & 0x00FF;
	bool doBlend = false;

	if (show == 2 && blend == 1)
		a = (((blend >> 16) & 0x00FF) * 255) / 100;
	else if (show == 1)
		a = 128;

	for (size_t y = 0; y < dataH; ++y)
	{
		for (size_t x = 0; x < dataW; ++x)
		{
			pixel = data[x + dataW*y];
			size_t pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);

			if (pixel == transparent)
			{
				pngData[pngPos + 0] = 0;
				pngData[pngPos + 1] = 0;
				pngData[pngPos + 2] = 0;
				pngData[pngPos + 3] = 0;
				continue;
			}

			r = (((pixel >> 11) & 0x1F) * 255) / 31;
			g = (((pixel >>  5) & 0x3F) * 255) / 63;
			b = (((pixel >>  0) & 0x1F) * 255) / 31;

			if (show == 2 && (blend == 2 || blend == 3))
			{
				// Additive, just treat the general brightness as the alpha because lazy
				a = (r + g + b) / 3;

				// Super bright!
				if (blend == 3)
				{
					uint32_t brighta = (a * 175) / 100;
					if (brighta > 255)
						a = 255;
					else
						a = (uint8_t)brighta;
				}
			}
			else if (show == 2 && blend == 4)
			{
				// Multiply, just do the inverse of the above
				a = 255 - ((r + g + b) / 3);
			}

			pngData[pngPos + 0] = r;
			pngData[pngPos + 1] = g;
			pngData[pngPos + 2] = b;
			pngData[pngPos + 3] = a;
		}
	}
}

void translateRLE16(void *pixelData, size_t dataW, size_t dataH, size_t xoff, size_t yoff, uint8_t* pngData, size_t pngW, size_t pngH, uint32_t transparent, uint32_t mode)
{
	uint16_t *data = static_cast<uint16_t*>(pixelData);

	uint32_t pixel = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;

	int show = mode & 0x00FF;
	int blend = (mode >> 8) & 0x00FF;
	bool doBlend = false;

	if (show == 2 && blend == 1)
		a = (((blend >> 16) & 0x00FF) * 255) / 100;
	else if (show == 1)
		a = 128;

	size_t offset = 0;
	for (size_t y = 0; y < dataH; ++y)
	{
		uint32_t scanSize = data[offset];
		offset++;
		scanSize |= data[offset] << 16;
		offset++;

		scanSize /= 2;

		if (scanSize & 0x01)
			scanSize++;

		scanSize-=2;

		size_t x = 0;
		bool wasTransparent = false;
		
		for (size_t i = 0; i < scanSize; ++i)
		{
			pixel = data[offset + i];
			size_t pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);

			if (wasTransparent)
			{
				size_t count = pixel / 2;
				for (size_t j = 0; j < count; ++j)
				{
					pngData[pngPos + 0] = 0;
					pngData[pngPos + 1] = 0;
					pngData[pngPos + 2] = 0;
					pngData[pngPos + 3] = 0;
					x++;
					pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);
				}

				wasTransparent = false;
				continue;
			}

			if (pixel == transparent)
			{
				if ((i + 1) < scanSize)
				{
					wasTransparent = true;
				}
				else
				{
					pngData[pngPos + 0] = 0;
					pngData[pngPos + 1] = 0;
					pngData[pngPos + 2] = 0;
					pngData[pngPos + 3] = 0;
					x++;
					pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);
				}

				continue;
			}

			r = (((pixel >> 11) & 0x1F) * 255) / 31;
			g = (((pixel >> 5) & 0x3F) * 255) / 63;
			b = (((pixel >> 0) & 0x1F) * 255) / 31;

			if (show == 2 && (blend == 2 || blend == 3))
			{
				// Additive, just treat the general brightness as the alpha because lazy
				a = (r + g + b) / 3;

				// Super bright!
				if (blend == 3)
				{
					uint32_t brighta = (a * 175) / 100;
					if (brighta > 255)
						a = 255;
					else
						a = (uint8_t)brighta;
				}
			}
			else if (show == 2 && blend == 4)
			{
				// Multiply, just do the inverse of the above
				a = 255 - ((r + g + b) / 3);
			}

			pngData[pngPos + 0] = r;
			pngData[pngPos + 1] = g;
			pngData[pngPos + 2] = b;
			pngData[pngPos + 3] = a;

			x++;
		}

		offset += scanSize;
	}
}

void translateRLE8(void *pixelData, size_t dataW, size_t dataH, size_t xoff, size_t yoff, uint8_t* pngData, size_t pngW, size_t pngH, std::shared_ptr<Palette> pallete, uint32_t transparent, uint32_t mode)
{
	uint8_t *data = static_cast<uint8_t*>(pixelData);

	uint32_t pixel = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;

	int show = mode & 0x00FF;
	int blend = (mode >> 8) & 0x00FF;
	bool doBlend = false;

	if (show == 2 && blend == 1)
		a = (((blend >> 16) & 0x00FF) * 255) / 100;
	else if (show == 1)
		a = 128;

	size_t offset = 0;
	for (size_t y = 0; y < dataH; ++y)
	{
		uint32_t scanSize = data[offset];
		offset++;
		scanSize |= data[offset] << 8;
		offset++;
		scanSize |= data[offset] << 16;
		offset++;
		scanSize |= data[offset] << 24;
		offset++;

		scanSize;

/*		if (scanSize & 0x01)
			scanSize++;*/

		scanSize -= 4;

		size_t x = 0;
		bool wasTransparent = false;

		for (size_t i = 0; i < scanSize; ++i)
		{
			pixel = data[offset + i];
			size_t pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);

			if (wasTransparent)
			{
				size_t count = pixel;
				for (size_t j = 0; j < count; ++j)
				{
					pngData[pngPos + 0] = 0;
					pngData[pngPos + 1] = 0;
					pngData[pngPos + 2] = 0;
					pngData[pngPos + 3] = 0;
					x++;
					pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);
				}

				wasTransparent = false;
				continue;
			}

			if (pixel == transparent)
			{
				if ((i + 1) < scanSize)
				{
					wasTransparent = true;
				}
				else
				{
					pngData[pngPos + 0] = 0;
					pngData[pngPos + 1] = 0;
					pngData[pngPos + 2] = 0;
					pngData[pngPos + 3] = 0;
					x++;
					pngPos = (4 * (x + xoff)) + (4 * (y + yoff) * pngW);
				}

				continue;
			}

			r = (*pallete)[4 * pixel + 0];
			g = (*pallete)[4 * pixel + 1];
			b = (*pallete)[4 * pixel + 2];

			if (show == 2 && (blend == 2 || blend == 3))
			{
				// Additive, just treat the general brightness as the alpha because lazy
				a = (r + g + b) / 3;

				// Super bright!
				if (blend == 3)
				{
					uint32_t brighta = (a * 175) / 100;
					if (brighta > 255)
						a = 255;
					else
						a = (uint8_t)brighta;
				}
			}
			else if (show == 2 && blend == 4)
			{
				// Multiply, just do the inverse of the above
				a = 255 - ((r + g + b) / 3);
			}

			pngData[pngPos + 0] = r;
			pngData[pngPos + 1] = g;
			pngData[pngPos + 2] = b;
			pngData[pngPos + 3] = a;

			x++;
		}

		offset += scanSize;
	}
}
