#ifndef STREAM_DIF_H
#define STREAM_DIF_H

#include <vector>
#include <string>
#include <mutex>
#include <type_traits>
#include <cstddef>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <windows.h>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace color
{
	enum Color
	{
		black = 0,

		dblue, dark_blue = dblue,
		dgreen, dark_green = dgreen,
		dcyan, dark_cyan = dcyan,
		dred, dark_red = dred,
		dmagenta, dark_magenta = dmagenta,
		dyellow, dark_yellow = dyellow, brown = dyellow,

		lgray, light_gray = lgray,
		lgrey = lgray, light_grey = lgray,
		dgray, dark_gray = dgray,
		dgrey = dgray, dark_grey = dgray,

		lblue, light_blue = lblue,
		lgreen, light_green = lgreen,
		lcyan, light_cyan = lcyan,
		lred, light_red = lred,
		lmagenta, light_magenta = lmagenta,
		lyellow, light_yellow = lyellow, yellow = lyellow,

		white
	};

	Color GetBackgroundColor()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return (Color)((csbi.wAttributes & 0xF0) >> 4);
	}

	Color GetForegroundColor()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return (Color)(csbi.wAttributes & 0x0F);
	}

	void SetColor(Color fg, Color bg)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (int)(bg << 4) | (int)fg);
	}

	void SetForegroundColor(Color fg)
	{
		SetColor(fg, GetBackgroundColor());
	}


}

namespace conf
{
	namespace Jxs
	{
		const color::Color PictureHeader = color::Color::dark_blue,
			DataHeader = color::Color::dark_magenta,
			Data = color::Color::dark_green,
			Error = color::Color::dark_red;
	}
	
}

namespace dif
{
	class StreamDif final
	{
	public:

		enum class MethodDestination : int
		{
			REFENENCE,
			DEBUG
		};

		using ElementType = uint8_t;

		template <typename NumberType>
		size_t addNumber(MethodDestination destination, NumberType number, color::Color numberColor)
		{
			size_t elementTypeNumberTypeSizeRatio = sizeof(NumberType) / sizeof(ElementType);
			ElementType* dataToWrite = static_cast<ElementType*>(&number);
			return addMemory(destination, dataToWrite, elementTypeNumberTypeSizeRatio, numberColor);
		}

		template <typename PointerType>
		size_t addMemory(MethodDestination destination, const PointerType data, size_t size, color::Color dataColor)
		{
			check_types<PointerType>();
			size_t elementTypeNumberTypeSizeRatio = sizeof(std::remove_pointer<PointerType>::type) / sizeof(ElementType);
			ElementType* dataToWrite = static_cast<ElementType*>(data);
			auto& [destinationVector, destinationVectorColor] = getDestinationVector(destination);
			size_t toReturn = destinationVector.size() - 1;
			destinationVector.insert(destinationVector.end(), dataToWrite, dataToWrite + elementTypeNumberTypeSizeRatio * size);
			destinationVectorColor.insert(destinationVectorColor.end(), elementTypeNumberTypeSizeRatio * size, dataColor);
			return toReturn;
		}

		template <typename NumberType>
		size_t changeElementColor(MethodDestination destination, size_t index, color::Color elementColor)
		{
			check_types<NumberType>();
			size_t elementTypeNumberTypeSizeRatio = sizeof(NumberType) / sizeof(ElementType);
			changeMemoryColor(destination, index, elementTypeNumberTypeSizeRatio, elementColor);
			return index;
		}

		size_t changeMemoryColor(MethodDestination destination, size_t pos, size_t size, color::Color elementsColor)
		{
			auto& [destinationVector, destinationVectorColor] = getDestinationVector(destination);
			std::fill(destinationVectorColor.begin() + pos, destinationVectorColor.begin() + pos + size, elementsColor);
			return pos;
		}

		void showDif(size_t lineWidht)
		{
			DataDiplay refDisplay(dataReference);
			DataDiplay debugDisplay(dataToDebug);
			size_t pos = 0;
			
			auto& [referenceData, referenceColors] = dataReference;
			auto& [toDebugData, toDebugColors] = dataToDebug;
			size_t difLenght = (std::min)(referenceData.size(), toDebugData.size());
			for (size_t pos = 0; pos < difLenght;)
			{
				size_t curLineWidht = (std::min)(lineWidht, difLenght - pos);
				std::vector<color::Color> difResult(curLineWidht, color::Color::black);
				for (size_t localLinePos = 0; localLinePos < curLineWidht; localLinePos++)
				{
					size_t curPos = pos + localLinePos;
					if (referenceData.at(curPos) != toDebugData.at(curPos) ||
						referenceColors.at(curPos) != toDebugColors.at(curPos))
						difResult.at(localLinePos) = color::Color::white;
				}
				refDisplay.showLine(difResult);
				std::cout << "\t";
				debugDisplay.showLine(difResult);
				std::cout << std::endl;
				pos += curLineWidht;
			}
			

		}

		void show(MethodDestination destination, bool dif = false);

		void loadBin(MethodDestination destination, const std::string& fileName) 
		{
			if (!std::filesystem::exists(fileName))
			{
				std::cout << "data file is not exists!" << std::endl;
				return;
			}
			if (!std::filesystem::exists(fileName))
			{
				std::cout << "color file is not exists!" << std::endl;
				return;
			}
			auto& [dataVector, colorVector] = getDestinationVector(destination);
			loadToVector(fileName, dataVector);
			loadToVector(fileName + ".color", colorVector);
		}

		void saveBin(MethodDestination destination, const std::string& fileName)
		{
			auto& [dataVector, colorVector] = getDestinationVector(destination);
			std::ofstream dataFile(fileName, std::ios::out | std::ios::binary);
			dataFile.write((const char*)dataVector.data(), dataVector.size()*sizeof(ElementType));
			std::ofstream colorFile(fileName + ".color", std::ios::out | std::ios::binary);
			colorFile.write((const char*)colorVector.data(), colorVector.size() * sizeof(ElementType));
		}

	private:
		using DestinationVectors = std::pair<std::vector<ElementType>, std::vector<color::Color>>;

		class DataDiplay final
		{
		public:
			DataDiplay(const DestinationVectors& data) :
				data(data)
			{
				if (data.first.size() != data.second.size())
					throw std::invalid_argument("color and data size must be equal");
			}
			
			void showLine(const std::vector<color::Color>& lineBackgroungColors)
			{
				std::stringstream ssAdress;
				ssAdress << std::setfill('0') << std::setw(sizeof(uint32_t) * 2)	<< std::hex << pos;
				color::SetColor(adressForegroundColor, adressBackgroudColor);
				std::cout << ssAdress.str() << " ";
				for (auto& elementBackGrougColor : lineBackgroungColors)
				{
					if (pos < data.first.size())
					{
						const uint8_t* byteData = static_cast<const uint8_t*>(&data.first.at(pos));
						for (size_t localPos = 0; localPos < sizeof(ElementType); localPos++)
						{
							std::stringstream byte;
							byte << std::setfill('0') << std::setw(sizeof(ElementType) * 2) << std::hex << (size_t)byteData[localPos] << " ";
							color::SetColor(data.second.at(pos), elementBackGrougColor);
							std::cout << byte.str();
						}
						color::SetColor(adressForegroundColor, adressBackgroudColor);
						std::cout << " ";
					}
					else
						break;
					pos++;
				}
			}

		private:
			const DestinationVectors& data;
			color::Color adressBackgroudColor = color::Color::black;
			color::Color adressForegroundColor = color::Color::white;
			size_t pos = 0;
		};

		template <typename PointerType>
		constexpr void check_types()
		{
			static_assert(std::is_pointer<PointerType>(), "data must have reference type");
			static_assert(sizeof(ElementType) <= sizeof(std::remove_reference<PointerType>::type), "this size of data type must be bigger than ElementType size or equal");
			static_assert(sizeof(std::remove_reference<PointerType>::type) % sizeof(ElementType) == 0, "the size of number must be a multiple of the size of ElementType");
		}

		DestinationVectors& getDestinationVector(MethodDestination destination)
		{
			switch (destination)
			{
			case MethodDestination::REFENENCE:
				return dataReference;
			case MethodDestination::DEBUG:
				return dataToDebug;
			}
			throw std::invalid_argument("invalid destination");
		}

		template <typename T>
		void loadToVector(const std::string &fileName, std::vector<T> &dataVector)
		{
			std::ifstream dataFile(fileName, std::ios_base::binary);
			if (!dataFile.eof() && !dataFile.fail())
			{
				dataFile.seekg(0, std::ios_base::end);
				std::streampos fileSize = dataFile.tellg();
				dataVector.resize(fileSize);

				dataFile.seekg(0, std::ios_base::beg);
				dataFile.read((char*)&dataVector[0], fileSize * sizeof(T));
				int d = 0;
			}
			else
			{
				std::cout << "cannot open " << fileName << std::endl;
			}
		}
		color::Color defaultColor = color::dark_blue;
		DestinationVectors dataReference;
		DestinationVectors dataToDebug;


	};
	
	class StreamDifGetter final
	{
	private:
		static StreamDif& instance()
		{
			static StreamDif instance;
			return instance;
		}
	public:
		static StreamDif& get()
		{
			static std::once_flag flag;
			std::call_once(flag, [] { instance(); });
			return instance();
		}
	};
}

#endif 