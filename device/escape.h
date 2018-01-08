#ifndef ESCAPE_H
#define ESCAPE_H

#define ESC_FF		"\x0c"
#define ESC_CLEAR	"\e[2J\e[0;0H"

#define ESC_BLACK	"\e[30m"
#define ESC_RED		"\e[31m"
#define ESC_GREEN	"\e[32m"
#define ESC_YELLOW	"\e[33m"
#define ESC_BLUE	"\e[34m"
#define ESC_MAGENTA	"\e[35m"
#define ESC_CYAN	"\e[36m"
#define ESC_WHITE	"\e[37m"

#define ESC_BRIGHT_BLACK	"\e[90m"
#define ESC_BRIGHT_RED		"\e[91m"
#define ESC_BRIGHT_GREEN	"\e[92m"
#define ESC_BRIGHT_YELLOW	"\e[93m"
#define ESC_BRIGHT_BLUE		"\e[94m"
#define ESC_BRIGHT_MAGENTA	"\e[95m"
#define ESC_BRIGHT_CYAN		"\e[96m"
#define ESC_BRIGHT_WHITE	"\e[97m"

#define ESC_RGB(r, g, b)	"\e[38;2;" #r ";" #g ";" #b "m"

#define ESC_RESET	"\e[0m"
#define ESC_BOLD	"\e[1m"
#define ESC_FAINT	"\e[2m"
#define ESC_ITALIC	"\e[3m"
#define ESC_UNDERLINE	"\e[4m"

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
