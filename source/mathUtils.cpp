#include "mathUtils.h"

//Not the original code, because it's better
int getSin(uint8_t deg)
{
	return static_cast<int>(sin(deg * (M_PI / 0x80)) * 512.0);
}

int getCos(uint8_t deg)
{
	return static_cast<int>(cos(deg * (M_PI / 0x80)) * 512.0);
}

uint8_t getAtan(int x, int y)
{
	return static_cast<uint8_t>(atan2(-y, -x) * 0x80 / M_PI);
}

//these are good functions
int random(int32_t mi, int32_t ma) {
	return rand() % (ma - mi + 1) + mi;
}

int sign(int x) {
	if (x != 0)
		return x / std::abs(x);

	return 0;
}

int clamp(int x, int mi, int ma) {
	return std::max(std::min(ma, x), mi);
}