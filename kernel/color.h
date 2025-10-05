#ifndef COLOR_H
#define COLOR_H

// reset color
#define RESET "\033[0m"

// text format
#define BOLD "\033[1m"      // 粗体
#define FAINT "\033[2m"     //  faint（弱化）
#define ITALIC "\033[3m"    // 斜体
#define UNDERLINE "\033[4m" // 下划线
#define BLINK "\033[5m"     // 闪烁
#define REVERSE "\033[7m"   // 反显（前景色与背景色交换）
#define HIDDEN "\033[8m"    // 隐藏

// color of text（foreground color）
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

// bright text color（high bright foreground color）
#define BRIGHT_BLACK "\033[90m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

// background color
#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

// bright background color（high bright background color）
#define BG_BRIGHT_BLACK "\033[100m"
#define BG_BRIGHT_RED "\033[101m"
#define BG_BRIGHT_GREEN "\033[102m"
#define BG_BRIGHT_YELLOW "\033[103m"
#define BG_BRIGHT_BLUE "\033[104m"
#define BG_BRIGHT_MAGENTA "\033[105m"
#define BG_BRIGHT_CYAN "\033[106m"
#define BG_BRIGHT_WHITE "\033[107m"

// test macro
#define ERROR(msg) printk(RED "[ERROR]: \t%s" RESET "\n", msg)
#define SUCCESS(msg) printk(GREEN "[SUCCESS]: \t%s" RESET "\n", msg)
#define WARNING(msg) printk(YELLOW "[WARNING]: \t%s" RESET "\n", msg)
#define INFO(msg) printk(BLUE "[INFO]: \t%s" RESET "\n", msg)

#endif // COLOR_TERMINAL_H
