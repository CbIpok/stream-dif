#include "stream_dif.h"
#include <sstream>

int main()
{
	dif::StreamDif &differ = dif::StreamDifGetter::get();
	std::vector<uint8_t> data1{ 1,2,3,4,5,6,7,8,9,10,11,12,14,14,15,16};
	std::vector<uint8_t> data2{ 1,2,3,4,5,6,7,8,9,11,11,12,14,14,15,16};
	data1.insert(data1.end(), data1.begin(), data1.end());
	data2.insert(data2.end(), data2.begin(), data2.end());
	differ.addMemory(dif::StreamDif::MethodDestination::REFENENCE, data1.data(), data1.size(), conf::Jxs::Data);
	differ.addMemory(dif::StreamDif::MethodDestination::DEBUG, data2.data(), data2.size(), color::Color::dark_green);
	differ.changeMemoryColor(dif::StreamDif::MethodDestination::DEBUG, 3, 2, color::Color::dark_blue);
	differ.changeMemoryColor(dif::StreamDif::MethodDestination::REFENENCE, 3, 1, color::Color::dark_blue);
	differ.changeMemoryColor(dif::StreamDif::MethodDestination::DEBUG, 6, 2, color::Color::dark_magenta);
	differ.changeMemoryColor(dif::StreamDif::MethodDestination::REFENENCE, 6, 1, color::Color::dark_magenta);
	differ.changeMemoryColor(dif::StreamDif::MethodDestination::DEBUG, 12, 2, color::Color::dark_red);
	differ.changeMemoryColor(dif::StreamDif::MethodDestination::REFENENCE, 12, 1, color::Color::dark_red);
	differ.showDif(8);
	return 0;
}