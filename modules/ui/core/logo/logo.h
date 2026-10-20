#ifndef LOGO_H
#define LOGO_H

/*
 * logo.h - The UzOS banner and the flag picture.
 */

#define LOGO_FLAG_WIDTH 20
#define LOGO_FLAG_ROWS  8

// The big "UzOS" banner shown at boot.
void logo_print_banner(void);

// One row (0 .. LOGO_FLAG_ROWS - 1) of the Uzbek flag, LOGO_FLAG_WIDTH
// characters wide, printed at the cursor. Used by `uzfetch`.
void logo_print_flag_row(int row);

#endif
