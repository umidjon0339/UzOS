#ifndef DEF_SAFE_TEXT_H
#define DEF_SAFE_TEXT_H

// Copy 'text' into 'out' (width + 1 bytes) so it fits 'width' columns:
// control characters become '?' (logged), and a longer text is cut
// with '»' as its last character. Returns 1 if nothing had to be replaced.
int def_safe_text(const char *text, char *out, int width);

#endif
