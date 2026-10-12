#include "fs_security.h"
#include "string.h"

int fs_security_check_path(const char *path) {
    if (!path) return 0;
    
    // Simple check: don't allow ".." to prevent traversing out of allowed directories
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '.' && path[i+1] == '.') {
            return 0; // Deny
        }
    }
    
    return 1; // Allow
}
