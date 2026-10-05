#include "FileSystem.hpp"
#include "Error.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

namespace voxel {
bool fileExists(const std::string& path) {
    struct stat info {};
    if (stat(path.c_str(), &info) == 0)
        return true;
    if (errno == ENOENT)
        return false;
    throw Error("Cannot inspect '%s': %s", path.c_str(), std::strerror(errno));
}
void ensureDirectory(const std::string& path) {
    if (path.empty())
        throw Error("Directory path is empty");
    for (std::size_t i = 1; i <= path.size(); ++i) {
        if (i != path.size() && path[i] != '/')
            continue;
        const auto part = path.substr(0, i);
        if (part.empty() || part.back() == ':')
            continue;
        if (mkdir(part.c_str(), 0777) != 0 && errno != EEXIST)
            throw Error("Cannot create directory '%s': %s", part.c_str(), std::strerror(errno));
        struct stat info {};
        if (stat(part.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
            throw Error("Expected directory '%s'", part.c_str());
    }
}
std::vector<DirectoryEntry> listDirectory(const std::string& path) {
    struct CloseDir {
        void operator()(DIR* p) const {
            if (p)
                closedir(p);
        }
    };
    std::unique_ptr<DIR, CloseDir> dir(opendir(path.c_str()));
    if (!dir)
        throw Error("Cannot open directory '%s': %s", path.c_str(), std::strerror(errno));
    std::vector<DirectoryEntry> result;
    for (;;) {
        errno = 0;
        const auto entry = readdir(dir.get());
        if (!entry) {
            if (errno)
                throw Error("Cannot read directory '%s': %s", path.c_str(), std::strerror(errno));
            break;
        }
        const std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        if (result.size() >= 16384)
            throw Error("Directory '%s' exceeds 16384 entries", path.c_str());
        const auto full = path + "/" + name;
        struct stat info {};
#ifndef __3DS__
        if (lstat(full.c_str(), &info) != 0)
            throw Error("Cannot inspect '%s'", full.c_str());
        if (S_ISLNK(info.st_mode))
            continue;
#else
        // SD FAT has no symlinks; use libctru's supported stat operation.
        if (stat(full.c_str(), &info) != 0)
            throw Error("Cannot inspect '%s'", full.c_str());
#endif
        if (S_ISDIR(info.st_mode) || S_ISREG(info.st_mode))
            result.push_back({name, S_ISDIR(info.st_mode)});
    }
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.name < b.name; });
    return result;
}
bool safeRelativePath(const std::string& path) {
    if (path.empty() || path.size() > 240 || path.front() == '/' ||
        path.find_first_of("\\:") != std::string::npos)
        return false;
    std::size_t begin = 0;
    while (begin < path.size()) {
        const auto end = path.find('/', begin);
        const auto part = path.substr(begin, end == std::string::npos ? end : end - begin);
        if (part.empty() || part == "." || part == "..")
            return false;
        for (unsigned char c : part)
            if (c < 32 || c == 127)
                return false;
        if (end == std::string::npos)
            return true;
        begin = end + 1;
    }
    return false;
}
std::vector<std::string> scanAssets(const std::string& root, const Progress& progress) {
    struct Pending {
        std::string relative;
        unsigned depth;
    };
    std::vector<Pending> pending{{"", 0}};
    std::vector<std::string> result;
    unsigned visited = 0;
    while (!pending.empty()) {
        auto current = std::move(pending.back());
        pending.pop_back();
        if (current.depth > 16)
            throw Error("Assets '%s': directory nesting exceeds 16", current.relative.c_str());
        if (progress)
            progress("Scanning " + current.relative, 0, 0);
        for (const auto& entry :
             listDirectory(root + (current.relative.empty() ? "" : "/" + current.relative))) {
            if (++visited > 1024)
                throw Error("Asset tree exceeds 1024 entries");
            const auto relative =
                current.relative.empty() ? entry.name : current.relative + "/" + entry.name;
            if (!safeRelativePath(relative))
                throw Error("Invalid asset path '%s'", relative.c_str());
            if (entry.directory)
                pending.push_back({relative, current.depth + 1});
            else {
                auto extension = relative.substr(relative.find_last_of('.') == std::string::npos
                                                     ? relative.size()
                                                     : relative.find_last_of('.'));
                for (auto& c : extension)
                    c = char(std::tolower(static_cast<unsigned char>(c)));
                if (extension == ".png" || extension == ".json")
                    result.push_back(relative);
            }
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}
void writeFileDurably(const std::string& path, const std::string& data) {
    struct stat existing {};
    if (stat(path.c_str(), &existing) == 0 && !S_ISREG(existing.st_mode))
        throw Error("Cannot replace non-file '%s'", path.c_str());
    const auto temp = path + ".tmp";
    FILE* file = std::fopen(temp.c_str(), "wb");
    if (!file)
        throw Error("Cannot write '%s': %s", temp.c_str(), std::strerror(errno));
    bool ok = std::fwrite(data.data(), 1, data.size(), file) == data.size();
    if (ok)
        ok = std::fflush(file) == 0;
    if (ok)
        ok = fsync(fileno(file)) == 0; // FSFILE_Flush through libctru's archive device.
    int saved = ok ? 0 : errno;
    if (std::fclose(file) != 0) {
        if (ok)
            saved = errno;
        ok = false;
    }
    if (!saved)
        saved = EIO;
    if (!ok) {
        std::remove(temp.c_str());
        throw Error("Could not flush '%s': %s", path.c_str(), std::strerror(saved));
    }
    // FAT rename may not replace an existing destination. The other save slot
    // remains intact even if removal, rename, or power fails here.
    if (std::remove(path.c_str()) != 0 && errno != ENOENT)
        throw Error("Cannot replace '%s': %s", path.c_str(), std::strerror(errno));
    if (std::rename(temp.c_str(), path.c_str()) != 0)
        throw Error("Cannot commit '%s': %s", path.c_str(), std::strerror(errno));
}
} // namespace voxel
