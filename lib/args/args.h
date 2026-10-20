#ifndef ARGS_H
#define ARGS_H

/*
 * args.h - Split a command line into words.
 *
 *     const char *rest = args;
 *     char word[64];
 *     rest = arg_next(rest, word, sizeof(word));   // first word
 *     rest = arg_next(rest, word, sizeof(word));   // second word
 */

// Copy the next space-separated word of 's' into 'out' (at most max - 1
// characters, always NUL-terminated). Returns where the word after it
// starts. 'out' is "" when there are no more words.
const char *arg_next(const char *s, char *out, int max);

#endif
