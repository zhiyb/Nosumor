#include <stdint.h>
#include "usage.h"

namespace Usage
{

static const struct {
	uint16_t usage;
	Qt::KeyboardModifier modifiers;
	Qt::Key key;
	const char *string;
} id[] = {
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
	{50, Qt::NoModifier, (Qt::Key)0, "`(?)"},		// Note 2
	{51, Qt::NoModifier, Qt::Key_Semicolon, ";"},		// Note 4
	{52, Qt::NoModifier, Qt::Key_Apostrophe, "'"},		// Note 4
	{53, Qt::NoModifier, Qt::Key_QuoteLeft, "`"},		// Note 4
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
	{70, Qt::NoModifier, Qt::Key_Print, "PrintScreen"},	// Note 1
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
	{135, Qt::NoModifier, (Qt::Key)0, "¥(\\)"},		// Note 15,28
	{136, Qt::NoModifier, Qt::Key_Hiragana, "ひらがな"},	// Note 16
	{137, Qt::NoModifier, (Qt::Key)0, "¥"},			// Note 17
	{138, Qt::NoModifier, Qt::Key_Kana_Shift, "変換"},	// Note 18
	{139, Qt::NoModifier, Qt::Key_Kana_Lock, "無変換"},	// Note 19
	{140, Qt::NoModifier, (Qt::Key)0, "I18N6"},		// Note 20
	{141, Qt::NoModifier, (Qt::Key)0, "I18N7"},		// Note 21
	{142, Qt::NoModifier, (Qt::Key)0, "I18N8"},		// Note 22
	{143, Qt::NoModifier, (Qt::Key)0, "I18N9"},		// Note 22
	{144, Qt::NoModifier, (Qt::Key)0, "LANG1"},		// Note 25
	{145, Qt::NoModifier, (Qt::Key)0, "LANG2"},		// Note 26
	{146, Qt::NoModifier, (Qt::Key)0, "LANG3"},		// Note 30
	{147, Qt::NoModifier, (Qt::Key)0, "LANG4"},		// Note 31
	{148, Qt::NoModifier, (Qt::Key)0, "LANG5"},		// Note 32
	{149, Qt::NoModifier, (Qt::Key)0, "LANG6"},		// Note 8
	{150, Qt::NoModifier, (Qt::Key)0, "LANG7"},		// Note 8
	{151, Qt::NoModifier, (Qt::Key)0, "LANG8"},		// Note 8
	{152, Qt::NoModifier, (Qt::Key)0, "LANG9"},		// Note 8
	{153, Qt::NoModifier, (Qt::Key)0, "AltErase"},		// Note 7
	{154, Qt::NoModifier, Qt::Key_SysReq, "SysReq"},	// Note 1
	{155, Qt::NoModifier, Qt::Key_Cancel, "Cancel"},
	{156, Qt::NoModifier, Qt::Key_Clear, "Clear"},
	{157, Qt::NoModifier, (Qt::Key)0, "Prior"},
	{158, Qt::NoModifier, (Qt::Key)0, "Return"},
	{159, Qt::NoModifier, (Qt::Key)0, "Separator"},
	{160, Qt::NoModifier, (Qt::Key)0, "Out"},
	{161, Qt::NoModifier, (Qt::Key)0, "Oper"},
	{162, Qt::NoModifier, (Qt::Key)0, "Clear"},
	{163, Qt::NoModifier, (Qt::Key)0, "CrSel"},
	{164, Qt::NoModifier, (Qt::Key)0, "ExSel"},
	// 165-175	Reserved
	{176, Qt::KeypadModifier, (Qt::Key)0, "KP00"},
	{177, Qt::KeypadModifier, (Qt::Key)0, "KP000"},
	{178, Qt::NoModifier, (Qt::Key)0, "Thousands"},		// Note 33
	{179, Qt::NoModifier, (Qt::Key)0, "Decimal"},		// Note 34
	{180, Qt::NoModifier, (Qt::Key)0, "Currency"},		// Note 34
	{181, Qt::NoModifier, (Qt::Key)0, "CurrencySub"},	// Note 34
	{182, Qt::KeypadModifier, Qt::Key_BracketLeft, "KP("},
	{183, Qt::KeypadModifier, Qt::Key_BracketRight, "KP)"},
	{184, Qt::KeypadModifier, Qt::Key_BraceLeft, "KP{"},
	{185, Qt::KeypadModifier, Qt::Key_BraceRight, "KP}"},
	{186, Qt::KeypadModifier, Qt::Key_Tab, "KPTab"},
	{187, Qt::KeypadModifier, Qt::Key_Backspace, "KPBackspace"},
	{188, Qt::KeypadModifier, Qt::Key_A, "KPA"},
	{189, Qt::KeypadModifier, Qt::Key_B, "KPB"},
	{190, Qt::KeypadModifier, Qt::Key_C, "KPC"},
	{191, Qt::KeypadModifier, Qt::Key_D, "KPD"},
	{192, Qt::KeypadModifier, Qt::Key_E, "KPE"},
	{193, Qt::KeypadModifier, Qt::Key_F, "KPF"},
	{194, Qt::KeypadModifier, (Qt::Key)0, "KPXOR"},
	{195, Qt::KeypadModifier, (Qt::Key)0, "KP^"},
	{196, Qt::KeypadModifier, Qt::Key_Percent, "KP%"},
	{197, Qt::KeypadModifier, (Qt::Key)0, "KP<"},
	{198, Qt::KeypadModifier, (Qt::Key)0, "KP>"},
	{199, Qt::KeypadModifier, Qt::Key_Ampersand, "KP&"},
	{200, Qt::KeypadModifier, (Qt::Key)0, "KP&&"},
	{201, Qt::KeypadModifier, (Qt::Key)0, "KP|"},
	{202, Qt::KeypadModifier, (Qt::Key)0, "KP||"},
	{203, Qt::KeypadModifier, Qt::Key_Colon, "KP:"},
	{204, Qt::KeypadModifier, (Qt::Key)0, "KP#"},
	{205, Qt::KeypadModifier, Qt::Key_Space, "KPSpace"},
	{206, Qt::KeypadModifier, Qt::Key_At, "KP@"},
	{207, Qt::KeypadModifier, (Qt::Key)0, "KP!"},
	{208, Qt::KeypadModifier, (Qt::Key)0, "KPMemStore"},
	{209, Qt::KeypadModifier, (Qt::Key)0, "KPMemRecall"},
	{210, Qt::KeypadModifier, (Qt::Key)0, "KPMemClear"},
	{211, Qt::KeypadModifier, (Qt::Key)0, "KPMemAdd"},
	{212, Qt::KeypadModifier, (Qt::Key)0, "KPMemSubtract"},
	{213, Qt::KeypadModifier, (Qt::Key)0, "KPMemMultiply"},
	{214, Qt::KeypadModifier, (Qt::Key)0, "KPMemDivide"},
	{215, Qt::KeypadModifier, (Qt::Key)0, "KP+"},
	{216, Qt::KeypadModifier, (Qt::Key)0, "KPClear"},
	{217, Qt::KeypadModifier, (Qt::Key)0, "KPClearEntry"},
	{218, Qt::KeypadModifier, (Qt::Key)0, "KPBinary"},
	{219, Qt::KeypadModifier, (Qt::Key)0, "KPOctal"},
	{220, Qt::KeypadModifier, (Qt::Key)0, "KPDecimal"},
	{221, Qt::KeypadModifier, (Qt::Key)0, "KPHexadecimal"},
	// 222-223	Reserved
	{224, Qt::ControlModifier, Qt::Key_Control, "LCtrl"},
	{225, Qt::ShiftModifier, Qt::Key_Shift, "LShift"},
	{226, Qt::AltModifier, Qt::Key_Alt, "LAlt"},
	{227, Qt::NoModifier, Qt::Key_Meta, "LSuper"},	// Note 10;23
	{228, Qt::ControlModifier, Qt::Key_Control, "RCtrl"},
	{229, Qt::ShiftModifier, Qt::Key_Shift, "RShift"},
	{230, Qt::AltModifier, Qt::Key_Alt, "RAlt"},
	{231, Qt::NoModifier, Qt::Key_Meta, "RSuper"},	// Note 10;23
	// 232-65535	Reserved
};

uint16_t keyboardUsage(const Qt::KeyboardModifiers modifiers, const Qt::Key key)
{
	if (key == 0)
		return 0;
	for (auto &i: id) {
		if (i.modifiers == modifiers && i.key == key)
			return i.usage;
	}
	return 0;
}

const char *keyboardString(const uint16_t usage)
{
	for (auto &i: id) {
		if (i.usage == usage)
			return i.string;
	}
	return 0;
}

}
