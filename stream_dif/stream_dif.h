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
#include <set>

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

		void changePositionsColor(MethodDestination destination, const std::vector<size_t>& positions, color::Color elementsColor)
		{
			auto& [destinationVector, destinationVectorColor] = getDestinationVector(destination);
			for (auto& i : positions)
			{
				if (i < destinationVectorColor.size())
					destinationVectorColor.at(i) = elementsColor;
			}
		}

		void showDif(size_t lenWidht)
		{
			int pos = getDifPos(0);
			std::cout << std::endl << pos << std::endl;
			if (pos == -1)
			{
				std::cout << " no dif" << std::endl;
				return;
			}
			int upPos = (pos < posUpDif) ? pos : posUpDif;
			for (size_t difIndex = 0; difIndex < DifCoutToShow; difIndex++)
			{
				pos = showDifFromPos(lenWidht, pos - upPos);
				std::cout << "...." << std::endl;
				pos = getDifPos(pos);
				std::cout << std::endl << pos << std::endl;
				if (pos == -1)
				{
					std::cout << " no dif more" << std::endl;
					return;
				}
			}
		}

		void clear(MethodDestination destination)
		{
			auto& [dataVector, colorVector] = getDestinationVector(destination);
			dataVector.clear();
			colorVector.clear();
		}

		void inspectType(size_t lenWidht, color::Color type)
		{
			auto& [referenceData, referenceColors] = dataReference;
			auto& [toDebugData, toDebugColors] = dataToDebug;
			int diffLen = (std::min)(referenceData.size(), toDebugData.size());
			int delta = 32;
			size_t difIndex = 0;
			for (int pos = 0; pos < diffLen && difIndex < DifCoutToShow; difIndex++)
			{
				while (!((referenceColors.at(pos) == type || toDebugColors.at(pos) == type)
					&& (referenceColors[pos] != toDebugColors[pos] || referenceData[pos] != toDebugData[pos])))
				{
					int newPos = getDifPos(pos);
					assert(referenceColors[newPos] != toDebugColors[newPos] || referenceData[newPos] != toDebugData[newPos]);
					if (newPos == -1)
					{
						std::cout << " no dif more" << std::endl;
						return;
					}
					if (newPos == pos)
						pos++;
					else
						pos = newPos;
				}
				
				int upPos = (pos < posUpDif) ? pos : posUpDif;
				if (referenceColors.at(pos) == type || toDebugColors.at(pos) == type)
				{
					auto [posUp, posDown] = findTypesNear(pos, type, 5);
					int lastPos = -1;
					for (int iPosUp = posUp.size() - 1;iPosUp >= 0;iPosUp--)
					{
						lastPos = showDifFromPos(lenWidht ,posUp[iPosUp] - upPos);
					}
					std::cout << std::endl;
					pos = showDifFromPos(lenWidht, pos-upPos);
					std::cout << std::endl;
					for (auto& typePos : posDown)
					{
						lastPos = showDifFromPos(lenWidht, typePos-upPos);
					}
					std::cout << std::endl <<"next" << std::endl;
				}
				

				
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
			dataFile.write((const char*)dataVector.data(), dataVector.size() * sizeof(ElementType));
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
				ssAdress << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex << pos;
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
						if (sizeof(ElementType) != sizeof(uint8_t))
							std::cout << " ";
					}
					else
						break;
					pos++;
				}
			}

			void setPos(size_t pos)
			{
				this->pos = pos;
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
		void loadToVector(const std::string& fileName, std::vector<T>& dataVector)
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

		int getDifPos(size_t posStart)
		{
			auto& [referenceData, referenceColors] = dataReference;
			auto& [toDebugData, toDebugColors] = dataToDebug;
			size_t difLenght = (std::min)(referenceData.size(), toDebugData.size());
			for (size_t pos = posStart; pos < difLenght; pos++)
			{
				if (compareElements(pos))
				{
					assert(referenceColors[pos] != toDebugColors[pos] || referenceData[pos] != toDebugData[pos]);
					return pos;
				}
					
			}
			return -1;
		}

		size_t showDifFromPos(size_t lineWidht, size_t pos)
		{
			DataDiplay refDisplay(dataReference);
			DataDiplay debugDisplay(dataToDebug);
			auto& [referenceData, referenceColors] = dataReference;
			auto& [toDebugData, toDebugColors] = dataToDebug;
			size_t difLenght = (std::min)(referenceData.size(), toDebugData.size());
			size_t linePrinted = 0;
			refDisplay.setPos(pos);
			debugDisplay.setPos(pos);
			for (; pos < difLenght && linePrinted < maxDifLen;)
			{
				size_t curLineWidht = (std::min)(lineWidht, difLenght - pos);
				std::vector<color::Color> difResult(curLineWidht, color::Color::black);
				for (size_t localLinePos = 0; localLinePos < curLineWidht; localLinePos++)
				{
					size_t curPos = pos + localLinePos;
					if (compareElements(curPos))
						difResult.at(localLinePos) = color::Color::white;
				}
				refDisplay.showLine(difResult);
				std::cout << "\t";
				debugDisplay.showLine(difResult);
				std::cout << std::endl;
				pos += curLineWidht;
				linePrinted++;
			}
			return pos;
		}

		bool compareElements(size_t pos)
		{
			auto& [referenceData, referenceColors] = dataReference;
			auto& [toDebugData, toDebugColors] = dataToDebug;
			return (referenceData.at(pos) != toDebugData.at(pos) ||
				referenceColors.at(pos) != toDebugColors.at(pos))
				&& toDebugColors.at(pos) != difIgnore;
		}

		std::pair<std::vector<int>, std::vector<int>> findTypesNear(size_t pos, color::Color type, int nearDistance)
		{
			auto& [referenceData, referenceColors] = dataReference;
			auto& [toDebugData, toDebugColors] = dataToDebug;
			int delta = 6;
			std::vector<int> up;
			std::vector<int> down;
			int nearFound = 0;
			for (int i = pos - delta; i > 0 && nearFound < nearDistance; i--)
			{
				if (referenceColors[i] == type)
				{
					up.push_back(i);
					nearFound += 1;
					i -= 6;
				}
				
			}
			nearFound = 0;
			int diffLen = (std::min)(referenceData.size(), toDebugData.size());
			for (int i = pos + delta; i < diffLen && nearFound < nearDistance; i ++)
			{
				if (referenceColors[i] == type)
				{
					down.push_back(i);
					//assert(_byteswap_ulong(referenceColors.at(i)) == 0xff2004);
					nearFound += 1;
					i += 6;
				}
			}
			return { up, down };
		}

		color::Color defaultColor = color::dark_blue;
		DestinationVectors dataReference;
		DestinationVectors dataToDebug;
		int maxDifLen = 16;
		int posUpDif = 64;
		size_t DifCoutToShow = 10;
		color::Color difIgnore{ color::Color::dark_red };
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