#include "shell_commands.h"
#include "vga.h"
#include "io.h"
#include "timer.h"

// Simple atoi helper
static int parse_int(const char *str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

static void play_sound(uint32_t nFrequence) {
    if (nFrequence == 0) return;
    uint32_t Div = 1193180 / nFrequence;
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t)(Div & 0xFF));
    outb(0x42, (uint8_t)((Div >> 8) & 0xFF));

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

static void nosound(void) {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

void cmd_beep(const char *args) {
    int freq = 440; // Default A4
    if (args[0] != '\0') {
        int parsed = parse_int(args);
        if (parsed > 20 && parsed < 20000) {
            freq = parsed;
        }
    }

    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_print_str("♪ Beeping at ");
    // Convert freq to string for output
    char buf[16];
    int idx = 0;
    int temp = freq;
    char temp_str[16];
    int t_idx = 0;
    while (temp > 0) {
        temp_str[t_idx++] = '0' + (temp % 10);
        temp /= 10;
    }
    while (t_idx > 0) {
        buf[idx++] = temp_str[--t_idx];
    }
    buf[idx] = '\0';
    vga_print_str(buf);
    vga_print_str(" Hz...\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    play_sound(freq);
    timer_wait(20); // 200ms
    nosound();
}

// Melody frequencies (in Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

void cmd_play(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("🎶 Playing UzOS Victory Jingle!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    int notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5};
    int durations[] = {15, 15, 15, 30, 15, 45};

    for (int i = 0; i < 6; i++) {
        play_sound(notes[i]);
        timer_wait(durations[i]);
        nosound();
        timer_wait(5); // tiny pause between notes
    }
}
