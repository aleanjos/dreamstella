#ifndef DISC_HXX
#define DISC_HXX

#include <kos.h>
#include <vector>
#include <string>


struct FileEntry {
    std::string name;
    std::string fullPath;
    bool isDirectory;
};

extern std::vector<FileEntry> romList;
extern std::string currentDirectory;

void scanRoms();

#endif
