#include "shell.h"
#include "vga.h"
#include "string.h"

// Simple integer calculator: calc 2 + 3, calc 10 * 5, etc.
// Supports: +, -, *, /, %
// Evaluates left-to-right (no precedence) for simplicity,
// but handles chained expressions: calc 2 + 3 * 4 = (2+3)*4 = 20

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int parse_int(const char **str) {
    int result = 0;
    int negative = 0;

    // Skip whitespace
    while (**str == ' ') (*str)++;

    // Handle negative numbers
    if (**str == '-') {
        negative = 1;
        (*str)++;
    }

    while (is_digit(**str)) {
        result = result * 10 + (**str - '0');
        (*str)++;
    }

    return negative ? -result : result;
}

void cmd_calc(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("\nUsage: calc <expression>\n");
        vga_print_str("Example: calc 2 + 3\n");
        vga_print_str("         calc 10 * 5\n");
        vga_print_str("         calc 100 / 7\n");
        vga_print_str("         calc 17 %% 5\n");
        vga_print_str("Operators: +  -  *  /  %%\n");
        return;
    }

    const char *ptr = args;

    // Parse first number
    int result = parse_int(&ptr);

    // Process operator-number pairs
    while (1) {
        // Skip whitespace
        while (*ptr == ' ') ptr++;

        // Check for operator
        char op = *ptr;
        if (op != '+' && op != '-' && op != '*' && op != '/' && op != '%') {
            break; // No more operators, we're done
        }
        ptr++; // skip operator

        // Parse next number
        int next = parse_int(&ptr);

        switch (op) {
            case '+': result += next; break;
            case '-': result -= next; break;
            case '*': result *= next; break;
            case '/':
                if (next == 0) {
                    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    vga_print_str("\nError: Division by zero!\n");
                    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                    return;
                }
                result /= next;
                break;
            case '%':
                if (next == 0) {
                    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    vga_print_str("\nError: Modulo by zero!\n");
                    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                    return;
                }
                result %= next;
                break;
        }
    }

    vga_print_str("\n= ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_int(result);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("\n");
}
