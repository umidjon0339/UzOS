#include "ramdisk.h"
#include "kheap.h"
#include "string.h"
#include "hello_elf.h"

// Read data from a ramdisk file
static uint32_t ramdisk_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->flags != FS_FILE) return 0;
    
    if (offset >= node->size) return 0;
    
    if (offset + size > node->size) {
        size = node->size - offset; // Adjust size to end of file
    }
    
    uint8_t *file_data = (uint8_t *)node->ptr;
    if (!file_data) return 0;
    
    memcpy(buffer, file_data + offset, size);
    return size;
}

// Write data to a ramdisk file
static uint32_t ramdisk_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->flags != FS_FILE) return 0;
    
    // For simplicity, we allocate a fixed 16KB buffer for each file initially or 
    // reallocate if needed. Let's do simple fixed 16KB for our educational OS.
    #define RAMDISK_MAX_FILE_SIZE 16384
    
    if (offset + size > RAMDISK_MAX_FILE_SIZE) {
        size = RAMDISK_MAX_FILE_SIZE - offset;
    }
    
    if (size == 0) return 0;
    
    uint8_t *file_data = (uint8_t *)node->ptr;
    if (!file_data) {
        // Allocate file data on first write
        file_data = kmalloc(RAMDISK_MAX_FILE_SIZE);
        memset(file_data, 0, RAMDISK_MAX_FILE_SIZE);
        node->ptr = (vfs_node_t *)file_data;
    }
    
    memcpy(file_data + offset, buffer, size);
    
    if (offset + size > node->size) {
        node->size = offset + size;
    }
    
    return size;
}

static vfs_node_t *ramdisk_readdir(vfs_node_t *node, uint32_t index) {
    if (node->flags != FS_DIRECTORY) return 0;
    if (index >= (uint32_t)node->num_children) return 0;
    
    return node->children[index];
}

static vfs_node_t *ramdisk_finddir(vfs_node_t *node, const char *name) {
    if (node->flags != FS_DIRECTORY) return 0;
    
    for (int i = 0; i < node->num_children; i++) {
        if (strcmp(node->children[i]->name, name) == 0) {
            return node->children[i];
        }
    }
    return 0;
}

vfs_node_t *ramdisk_create_file(vfs_node_t *dir, const char *name) {
    if (!dir || dir->flags != FS_DIRECTORY || dir->num_children >= 64) return 0;
    
    // Check if it already exists
    if (ramdisk_finddir(dir, name) != 0) return 0;
    
    vfs_node_t *new_file = kmalloc(sizeof(vfs_node_t));
    memset(new_file, 0, sizeof(vfs_node_t));
    
    strncpy(new_file->name, name, 127);
    new_file->flags = FS_FILE;
    new_file->read = ramdisk_read;
    new_file->write = ramdisk_write;
    new_file->parent = dir;
    
    dir->children[dir->num_children++] = new_file;
    return new_file;
}

vfs_node_t *ramdisk_create_dir(vfs_node_t *dir, const char *name) {
    if (!dir || dir->flags != FS_DIRECTORY || dir->num_children >= 64) return 0;
    
    // Check if it already exists
    if (ramdisk_finddir(dir, name) != 0) return 0;
    
    vfs_node_t *new_dir = kmalloc(sizeof(vfs_node_t));
    memset(new_dir, 0, sizeof(vfs_node_t));
    
    strncpy(new_dir->name, name, 127);
    new_dir->flags = FS_DIRECTORY;
    new_dir->readdir = ramdisk_readdir;
    new_dir->finddir = ramdisk_finddir;
    new_dir->parent = dir;
    
    dir->children[dir->num_children++] = new_dir;
    return new_dir;
}

int ramdisk_delete(vfs_node_t *dir, const char *name) {
    if (!dir || dir->flags != FS_DIRECTORY) return -1;
    
    for (int i = 0; i < dir->num_children; i++) {
        if (strcmp(dir->children[i]->name, name) == 0) {
            vfs_node_t *target = dir->children[i];
            
            // If it's a file, free its data buffer
            if (target->flags == FS_FILE && target->ptr) {
                kfree(target->ptr);
            }
            // Note: If it's a directory, we should theoretically delete recursively,
            // but for this simple OS we assume it's empty or we just leak it.
            
            kfree(target);
            
            // Shift remaining children left
            for (int j = i; j < dir->num_children - 1; j++) {
                dir->children[j] = dir->children[j + 1];
            }
            dir->num_children--;
            return 0;
        }
    }
    return -1; // Not found
}

void ramdisk_init(void) {
    // Create the root directory '/'
    fs_root = kmalloc(sizeof(vfs_node_t));
    memset(fs_root, 0, sizeof(vfs_node_t));
    
    strcpy(fs_root->name, "/");
    fs_root->flags = FS_DIRECTORY;
    fs_root->readdir = ramdisk_readdir;
    fs_root->finddir = ramdisk_finddir;
    fs_root->parent = fs_root; // Root is its own parent

    // Preload hello.elf into the root directory
    vfs_node_t *hello_node = ramdisk_create_file(fs_root, "hello");
    if (hello_node) {
        ramdisk_write(hello_node, 0, userland_hello_elf_len, userland_hello_elf);
    }
}
