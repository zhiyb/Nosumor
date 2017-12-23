#include <stdint.h>
#include "usage.h"

namespace Usage
{

static const struct {
	uint8_t usage;
	Qt::Key key;
	const char *string;
} keyboard[] = {
	{4, Qt::Key_A, "A"},	// Note 4
	{5, Qt::Key_B, "B"},
	{6, Qt::Key_C, "C"},	// Note 4
	{7, Qt::Key_D, "D"},
	{8, Qt::Key_E, "E"},
	{9, Qt::Key_F, "F"},
	{10, Qt::Key_G, "G"},
	{11, Qt::Key_H, "H"},
	{12, Qt::Key_I, "I"},
	{13, Qt::Key_J, "J"},
	{14, Qt::Key_K, "K"},
	{15, Qt::Key_L, "L"},
	{16, Qt::Key_M, "M"},	// Note 4
	{17, Qt::Key_N, "N"},
	{18, Qt::Key_O, "O"},	// Note 4
	{19, Qt::Key_P, "P"},	// Note 4
	{20, Qt::Key_Q, "Q"},	// Note 4
	{21, Qt::Key_R, "R"},
	{22, Qt::Key_S, "S"},	// Note 4
	{23, Qt::Key_T, "T"},
	{24, Qt::Key_U, "U"},
	{25, Qt::Key_V, "V"},
	{26, Qt::Key_W, "W"},	// Note 4
	{27, Qt::Key_X, "X"},	// Note 4
	{28, Qt::Key_Y, "Y"},	// Note 4
	{29, Qt::Key_Z, "Z"},	// Note 4
};

uint8_t keyboardUsage(const Qt::Key key)
{
	for (auto &k: keyboard) {
		if (k.key == key)
			return k.usage;
	}
	return 0;
}

const char *keyboardString(const uint8_t usage)
{
	for (auto &k: keyboard) {
		if (k.usage == usage)
			return k.string;
	}
	return 0;
}

}
