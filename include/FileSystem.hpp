#pragma once
#include <functional>
#include <string>
#include <vector>

namespace voxel {
using Progress = std::function<void(const std::string&, unsigned, unsigned)>;
struct DirectoryEntry {
    std::string name;
    bool directory;
};
bool fileExists(const std::string& path);
void ensureDirectory(const std::string& path);
std::vector<DirectoryEntry> listDirectory(const std::string& path);
std::vector<std::string> scanAssets(const std::string& root, const Progress& progress = {});
bool safeRelativePath(const std::string& path);
// Write/flush a temporary file, then rename. Caller uses alternating save slots
// so a previously validated generation is never removed during a failed write.
void writeFileDurably(const std::string& path, const std::string& data);
} // namespace voxel
