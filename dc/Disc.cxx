#include <kos.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <algorithm>

#include "Disc.hxx"

std::vector<FileEntry> romList;
std::string currentDirectory = "/cd/roms";

bool compareEntries(const FileEntry& a, const FileEntry& b) {
    if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
    return a.name < b.name;
}

void scanRoms()
{
    romList.clear();

    DIR *dir = opendir(currentDirectory.c_str());
    if (!dir)
        return;

    if (currentDirectory != "/cd/roms" && currentDirectory != "/cd") {
        FileEntry up;
        up.name = "Return to previous folder";
        up.fullPath = "..";
        up.isDirectory = true;
        romList.push_back(up);
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        std::string name = ent->d_name;

        if (name == "." || name == "..") continue;

        FileEntry entry;
        entry.name = name;
        entry.fullPath = currentDirectory + "/" + name;
        
        entry.isDirectory = (ent->d_type == 4);

        bool isRom = (name.find(".bin") != std::string::npos || name.find(".a26") != std::string::npos);

        if (entry.isDirectory || isRom)
            romList.push_back(entry);
    }
    
    closedir(dir);

    size_t sortStart = (!romList.empty() && romList[0].fullPath == "..") ? 1 : 0;
    
    if (romList.size() > sortStart) {
        std::sort(romList.begin() + sortStart, romList.end(), compareEntries);
    }
}
