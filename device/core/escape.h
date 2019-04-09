#ifndef ESCAPE_H
#define ESCAPE_H

#define ESC_FF		"\x0c"
#define ESC_CLEAR	"\033[2J\033[0;0H"

#define ESC_BLACK	"\033[30m"
#define ESC_RED		"\033[31m"
#define ESC_GREEN	"\033[32m"
#define ESC_YELLOW	"\033[33m"
#define ESC_BLUE	"\033[34m"
#define ESC_MAGENTA	"\033[35m"
#define ESC_CYAN	"\033[36m"
#define ESC_WHITE	"\033[37m"

#define ESC_BRIGHT_BLACK	"\033[90m"
#define ESC_BRIGHT_RED		"\033[91m"
#define ESC_BRIGHT_GREEN	"\033[92m"
#define ESC_BRIGHT_YELLOW	"\033[93m"
#define ESC_BRIGHT_BLUE		"\033[94m"
#define ESC_BRIGHT_MAGENTA	"\033[95m"
#define ESC_BRIGHT_CYAN		"\033[96m"
#define ESC_BRIGHT_WHITE	"\033[97m"

#define ESC_RGB(r, g, b)	"\033[38;2;" #r ";" #g ";" #b "m"

#define ESC_RESET	"\033[0m"
#define ESC_BOLD	"\033[1m"
#define ESC_FAINT	"\033[2m"
#define ESC_ITALIC	"\033[3m"
#define ESC_UNDERLINE	"\033[4m"

#define ESC_NORMAL	ESC_RESET
#define ESC_DEBUG	ESC_RESET ESC_BRIGHT_BLACK
#define ESC_INFO	ESC_RESET ESC_BRIGHT_BLUE
#define ESC_WARNING	ESC_BOLD ESC_BRIGHT_YELLOW
#define ESC_ERROR	ESC_BOLD ESC_BRIGHT_RED
#define ESC_GOOD	ESC_BOLD ESC_BRIGHT_GREEN

#define ESC_BOOT	ESC_CLEAR ESC_BOLD ESC_BRIGHT_MAGENTA
#define ESC_INIT	ESC_RESET ESC_CYAN
#define ESC_MSG		ESC_RESET ESC_BLUE
#define ESC_READ	ESC_RESET ESC_GREEN
#define ESC_WRITE	ESC_RESET ESC_RED
#define ESC_ENABLE	ESC_GREEN
#define ESC_DISABLE	ESC_RED
#define ESC_DATA	ESC_WHITE

#endif // ESCAPE_H