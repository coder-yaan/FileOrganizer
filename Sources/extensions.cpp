#include "extensions.hpp"
#include <filesystem>
#include <algorithm>

/*
    ============================
        extensions.cpp
    ============================

    This file IMPLEMENTS the classification logic declared
    in extensions.hpp.

    Core philosophy:
    - Add extensions in ONE place
    - Everything else updates automatically
    - No hardcoded conditions like:
        if (ext == "jpg" || ext == "png") ...
*/

/*
    MASTER CATEGORY MAP
    -------------------
    This is the heart of the system.

    RULE:
    -----
    If you want to support a new file type,
    you ONLY modify this map.

    No other code needs to change.
*/
const std::map<std::string, std::set<std::string>> CATEGORY_EXTENSION_MAP = {

    // ---------- IMAGE FILES ----------
    { "Image Files", {
                        "jpg", "jpeg", "png", "gif", "bmp", "webp",
                        "tiff", "svg", "ico", "heic"
                    }},

    // ---------- VIDEO FILES ----------
    { "Video Files", {
                        "mp4", "mkv", "avi", "mov", "wmv", "flv",
                        "webm", "mpeg", "mpg", "3gp", "m4v"
                    }},

    // ---------- AUDIO FILES ----------
    { "Audio Files", {
                        "mp3", "wav", "aac", "flac", "ogg",
                        "m4a", "wma", "opus", "aiff"
                    }},

    // ---------- DOCUMENT FILES ----------
    { "Text Files", {
                       "txt", "md", "log", "rtf", "nfo"
                   }},
    { "PDF Files", {
                      "pdf"
                  }},
    { "Word Files", {
                       "doc", "docx"
                   }},
    { "Excel Files", {
                        "xls", "xlsx"
                    }},
    { "PowerPoint Files", {
                             "ppt", "pptx"
                         }},

    // ---------- PROGRAMMING FILES ----------
    { "C Files", { "c" } },
    { "C++ Files", { "cpp", "cc", "cxx" } },
    { "Header Files", { "h", "hpp", "hh", "hxx" } },
    { "Java Files", { "java" } },
    { "Python Files", { "py" } },
    { "JavaScript Files", { "js" } },
    { "TypeScript Files", { "ts" } },
    { "Web Files", { "html", "css", "scss" } },
    { "Shell Scripts", { "sh" } },
    { "Go Files", { "go" } },
    { "Rust Files", { "rs" } },
    { "PHP Files", { "php" } },

    // ---------- DATA ----------
    { "Data Files", {
                       "csv", "json", "xml", "yaml", "yml"
                   }},

    // ---------- DATABASE ----------
    { "Database Files", {
                           "sql", "db", "sqlite", "sqlite3", "mdb"
                       }},

    // ---------- ARCHIVES ----------
    { "Archive Files", {
                          "zip", "rar", "7z", "tar", "gz",
                          "bz2", "xz", "tgz"
                      }},

    // ---------- EXECUTABLES ----------
    { "Executable Files", {
                             "exe", "msi", "bin", "app", "apk"
                         }},

    // ---------- LIBRARIES ----------
    { "Library Files", {
                          "dll", "so", "dylib", "a", "lib"
                      }},

    // ---------- CONFIGURATION ----------
    { "Config Files", {
                            "ini", "conf", "cfg", "env"
                        }}
};


/*
    CANONICAL_NAMES
    ---------------
    This set defines the official category names.

    Why this matters:
    - Prevents accidental mismatch between classification and folders
    - Useful for validation
*/
/*
    HARDCODED VERSION

const std::set<std::string> CANONICAL_NAMES = {
    "Image Files", "Video Files", "Audio Files",
    "Text Files", "PDF Files", "Word Files", "Excel Files",
    "PowerPoint Files", "C Files", "C++ Files", "Header Files",
    "Java Files", "Python Files", "JavaScript Files",
    "TypeScript Files", "Web Files", "Shell Scripts",
    "Go Files", "Rust Files", "PHP Files", "Data Files",
    "Database Files", "Archive Files",
    "Executable Files", "Library Files", "Config Files"
};

*/
const std::set <std::string> CANONICAL_NAMES = []
{
    std::set <std::string> canonical_names_holder;

    for (const std::pair<std::string, std::set<std::string> >& it: CATEGORY_EXTENSION_MAP )
    {
        canonical_names_holder.insert( it.first );
    }

    return canonical_names_holder;
}();

/*
    EXTENSION_LOOKUP
    ----------------
    Built ONCE at startup using a lambda.

    Converts:
        Category → Extensions
    into:
        Extension → Category

    This avoids nested loops during classification.
*/
const std::map<std::string, std::string> EXTENSION_LOOKUP = []()
{
    std::map<std::string, std::string> lookup;

    for (const std::pair< std::string, std::set<std::string> >& category : CATEGORY_EXTENSION_MAP)
    {
        for (const std::string& extension : category.second)
        {
            lookup[extension] = category.first;
        }
    }

    return lookup;
}();


/*
    classify_file_by_extension
    --------------------------
    Determines the category of a file using its extension.
*/
std::string classify_file_by_extension(const std::string& file_path)
{
    // Safely extract extension using filesystem
    std::filesystem::path path(file_path);
    std::string extension = path.extension().string();

    // If no extension exists, treat as unknown
    if (extension.empty())
        return "Others";

    // Remove leading '.' from extension
    extension.erase(0, 1);

    // Normalize extension to lowercase
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        ::tolower
    );

    // Perform fast lookup
    std::map<std::string, std::string>::const_iterator it = EXTENSION_LOOKUP.find(extension);
    if (it != EXTENSION_LOOKUP.end())
        return it->second;

    // Fallback category
    return "Others";
}
