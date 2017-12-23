#include <stdint.h>
#include "usage.h"

namespace Usage
{

static const struct {
	uint8_t usage;
	Qt::KeyboardModifier modifiers;
	Qt::Key key;
	const char *string;
} keyboard[] = {
	// http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
	{4, Qt::NoModifier, Qt::Key_A, "A"},			// Note 4
	{5, Qt::NoModifier, Qt::Key_B, "B"},
	{6, Qt::NoModifier, Qt::Key_C, "C"},			// Note 4
	{7, Qt::NoModifier, Qt::Key_D, "D"},
	{8, Qt::NoModifier, Qt::Key_E, "E"},
	{9, Qt::NoModifier, Qt::Key_F, "F"},
	{10, Qt::NoModifier, Qt::Key_G, "G"},
	{11, Qt::NoModifier, Qt::Key_H, "H"},
	{12, Qt::NoModifier, Qt::Key_I, "I"},
	{13, Qt::NoModifier, Qt::Key_J, "J"},
	{14, Qt::NoModifier, Qt::Key_K, "K"},
	{15, Qt::NoModifier, Qt::Key_L, "L"},
	{16, Qt::NoModifier, Qt::Key_M, "M"},			// Note 4
	{17, Qt::NoModifier, Qt::Key_N, "N"},
	{18, Qt::NoModifier, Qt::Key_O, "O"},			// Note 4
	{19, Qt::NoModifier, Qt::Key_P, "P"},			// Note 4
	{20, Qt::NoModifier, Qt::Key_Q, "Q"},			// Note 4
	{21, Qt::NoModifier, Qt::Key_R, "R"},
	{22, Qt::NoModifier, Qt::Key_S, "S"},			// Note 4
	{23, Qt::NoModifier, Qt::Key_T, "T"},
	{24, Qt::NoModifier, Qt::Key_U, "U"},
	{25, Qt::NoModifier, Qt::Key_V, "V"},
	{26, Qt::NoModifier, Qt::Key_W, "W"},			// Note 4
	{27, Qt::NoModifier, Qt::Key_X, "X"},			// Note 4
	{28, Qt::NoModifier, Qt::Key_Y, "Y"},			// Note 4
	{29, Qt::NoModifier, Qt::Key_Z, "Z"},			// Note 4
	{30, Qt::NoModifier, Qt::Key_1, "1"},			// Note 4
	{31, Qt::NoModifier, Qt::Key_2, "2"},			// Note 4
	{32, Qt::NoModifier, Qt::Key_3, "3"},			// Note 4
	{33, Qt::NoModifier, Qt::Key_4, "4"},			// Note 4
	{34, Qt::NoModifier, Qt::Key_5, "5"},			// Note 4
	{35, Qt::NoModifier, Qt::Key_6, "6"},			// Note 4
	{36, Qt::NoModifier, Qt::Key_7, "7"},			// Note 4
	{37, Qt::NoModifier, Qt::Key_8, "8"},			// Note 4
	{38, Qt::NoModifier, Qt::Key_9, "9"},			// Note 4
	{39, Qt::NoModifier, Qt::Key_0, "0"},			// Note 4
	{40, Qt::NoModifier, Qt::Key_Return, "Enter"},		// Note 5
	{41, Qt::NoModifier, Qt::Key_Escape, "Escape"},
	{42, Qt::NoModifier, Qt::Key_Backspace, "Backspace"},	// Note 13
	{43, Qt::NoModifier, Qt::Key_Tab, "Tab"},
	{44, Qt::NoModifier, Qt::Key_Space, "Space"},
	{45, Qt::NoModifier, Qt::Key_Minus, "-"},		// Note 4
	{46, Qt::NoModifier, Qt::Key_Equal, "="},		// Note 4
	{47, Qt::NoModifier, Qt::Key_BracketLeft, "["},		// Note 4
	{48, Qt::NoModifier, Qt::Key_BracketRight, "]"},	// Note 4
	{49, Qt::NoModifier, Qt::Key_Backslash, "\\"},
	{50, Qt::NoModifier, Qt::Key_QuoteLeft, "`"},		// Note 2
	{51, Qt::NoModifier, Qt::Key_Semicolon, ";"},		// Note 4
	{52, Qt::NoModifier, Qt::Key_Apostrophe, "'"},		// Note 4
	{53, Qt::NoModifier, Qt::Key_QuoteLeft, "`(?)"},	// Note 4
	{54, Qt::NoModifier, Qt::Key_Comma, ","},		// Note 4
	{55, Qt::NoModifier, Qt::Key_Period, "."},		// Note 4
	{56, Qt::NoModifier, Qt::Key_Slash, "/"},		// Note 4
	{57, Qt::NoModifier, Qt::Key_CapsLock, "CapsLock"},	// Note 11
	{58, Qt::NoModifier, Qt::Key_F1, "F1"},
	{59, Qt::NoModifier, Qt::Key_F2, "F2"},
	{60, Qt::NoModifier, Qt::Key_F3, "F3"},
	{61, Qt::NoModifier, Qt::Key_F4, "F4"},
	{62, Qt::NoModifier, Qt::Key_F5, "F5"},
	{63, Qt::NoModifier, Qt::Key_F6, "F6"},
	{64, Qt::NoModifier, Qt::Key_F7, "F7"},
	{65, Qt::NoModifier, Qt::Key_F8, "F8"},
	{66, Qt::NoModifier, Qt::Key_F9, "F9"},
	{67, Qt::NoModifier, Qt::Key_F10, "F10"},
	{68, Qt::NoModifier, Qt::Key_F11, "F11"},
	{69, Qt::NoModifier, Qt::Key_F12, "F12"},
	//{70, Qt::NoModifier, PrintScreen, "PrintScreen"},	// Note 1
	{71, Qt::NoModifier, Qt::Key_ScrollLock, "ScrollLock"},	// Note 11
	{72, Qt::NoModifier, Qt::Key_Pause, "Pause"},		// Note 1
	{73, Qt::NoModifier, Qt::Key_Insert, "Insert"},		// Note 1
	{74, Qt::NoModifier, Qt::Key_Home, "Home"},		// Note 1
	{75, Qt::NoModifier, Qt::Key_PageUp, "PageUp"},		// Note 1
	{76, Qt::NoModifier, Qt::Key_Delete, "Delete"},		// Note 1;14
	{77, Qt::NoModifier, Qt::Key_End, "End"},		// Note 1
	{78, Qt::NoModifier, Qt::Key_PageDown, "PageDown"},	// Note 1
	{79, Qt::NoModifier, Qt::Key_Right, "→"},		// Note 1
	{80, Qt::NoModifier, Qt::Key_Left, "←"},		// Note 1
	{81, Qt::NoModifier, Qt::Key_Down, "↓"},		// Note 1
	{82, Qt::NoModifier, Qt::Key_Up, "↑"},			// Note 1
	{83, Qt::NoModifier, Qt::Key_NumLock, "NumLock"},	// Note 11
	{84, Qt::KeypadModifier, Qt::Key_Slash, "KP/"},		// Note 1
	{85, Qt::KeypadModifier, Qt::Key_Asterisk, "KP*"},
	{86, Qt::KeypadModifier, Qt::Key_Minus, "KP-"},
	{87, Qt::KeypadModifier, Qt::Key_Plus, "KP+"},
	{88, Qt::KeypadModifier, Qt::Key_Enter, "KPEnter"},	// Note 5
	{89, Qt::KeypadModifier, Qt::Key_1, "KP1"},
	{90, Qt::KeypadModifier, Qt::Key_2, "KP2"},
	{91, Qt::KeypadModifier, Qt::Key_3, "KP3"},
	{92, Qt::KeypadModifier, Qt::Key_4, "KP4"},
	{93, Qt::KeypadModifier, Qt::Key_5, "KP5"},
	{94, Qt::KeypadModifier, Qt::Key_6, "KP6"},
	{95, Qt::KeypadModifier, Qt::Key_7, "KP7"},
	{96, Qt::KeypadModifier, Qt::Key_8, "KP8"},
	{97, Qt::KeypadModifier, Qt::Key_9, "KP9"},
	{98, Qt::KeypadModifier, Qt::Key_0, "KP0"},
	{99, Qt::KeypadModifier, Qt::Key_Period, "KP."},
	{100, Qt::NoModifier, Qt::Key_Backslash, "\(?)"},	// Note 3;6
	{101, Qt::NoModifier, Qt::Key_ApplicationLeft, "App(?)"},	// Note 10
	{102, Qt::NoModifier, Qt::Key_PowerOff, "Power(?)"},	// Note 9
	{103, Qt::KeypadModifier, Qt::Key_Equal, "KP="},
	{104, Qt::NoModifier, Qt::Key_F13, "F13"},
	{105, Qt::NoModifier, Qt::Key_F14, "F14"},
	{106, Qt::NoModifier, Qt::Key_F15, "F15"},
	{107, Qt::NoModifier, Qt::Key_F16, "F16"},
	{108, Qt::NoModifier, Qt::Key_F17, "F17"},
	{109, Qt::NoModifier, Qt::Key_F18, "F18"},
	{110, Qt::NoModifier, Qt::Key_F19, "F19"},
	{111, Qt::NoModifier, Qt::Key_F20, "F20"},
	{112, Qt::NoModifier, Qt::Key_F21, "F21"},
	{113, Qt::NoModifier, Qt::Key_F22, "F22"},
	{114, Qt::NoModifier, Qt::Key_F23, "F23"},
	{115, Qt::NoModifier, Qt::Key_F24, "F24"},
	{116, Qt::NoModifier, Qt::Key_Execute, "Execute"},
	{117, Qt::NoModifier, Qt::Key_Help, "Help"},
	{118, Qt::NoModifier, Qt::Key_Menu, "Menu"},
	{119, Qt::NoModifier, Qt::Key_Select, "Select"},
	{120, Qt::NoModifier, Qt::Key_Stop, "Stop"},
	{121, Qt::NoModifier, Qt::Key_Redo, "Redo"},
	{122, Qt::NoModifier, Qt::Key_Undo, "Undo"},
	{123, Qt::NoModifier, Qt::Key_Cut, "Cut"},
	{124, Qt::NoModifier, Qt::Key_Copy, "Copy"},
	{125, Qt::NoModifier, Qt::Key_Paste, "Paste"},
	{126, Qt::NoModifier, Qt::Key_Find, "Find"},
	{127, Qt::NoModifier, Qt::Key_VolumeMute, "VolumeMute"},
	{128, Qt::NoModifier, Qt::Key_VolumeUp, "VolumeUp"},
	{129, Qt::NoModifier, Qt::Key_VolumeDown, "VolumeDown"},
	{130, Qt::NoModifier, Qt::Key_CapsLock, "CapsLock(?)"},	// Note 12
	{131, Qt::NoModifier, Qt::Key_NumLock, "NumLock(?)"},	// Note 12
	{132, Qt::NoModifier, Qt::Key_ScrollLock, "ScrollLock(?)"},	// Note 12
	{133, Qt::NoModifier, Qt::Key_Comma, "KP,"},		// Note 27
	{134, Qt::NoModifier, Qt::Key_Equal, "KP="},		// Note 29
};

uint8_t keyboardUsage(const Qt::KeyboardModifiers modifiers, const Qt::Key key)
{
	for (auto &k: keyboard) {
		if (k.modifiers == modifiers && k.key == key)
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
