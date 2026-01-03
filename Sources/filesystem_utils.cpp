#include "filesystem_utils.hpp"
#include "extensions.hpp"

#include <algorithm>
#include <filesystem>

/*
    =========================================================
        CATEGORY_ALIAS_MAP
    =========================================================

    Maps each canonical category name to a large set of
    possible user-created folder names (aliases).

    WHY THIS EXISTS:
    ----------------
    Users are messy. They create folders like:
        "images", "pics", "camera roll", "screenshots", etc.

    Instead of creating *another* "Image Files" folder,
    we detect these aliases and normalize them into
    one canonical folder name.

    This keeps the directory clean and predictable.
*/
const std::map<std::string, std::set<std::string>> CATEGORY_ALIAS_MAP = {

    // -------- IMAGES --------
    { "Image Files",
        {
            "img", "imgs", "image", "images", "pic", "pics", "picture", "pictures", "photo", "photos",
            "photography", "camera", "camera roll", "gallery", "photo gallery", "screenshots", "wallpapers",
            "backgrounds", "portraits", "landscapes", "selfies", "family photos", "vacation photos",
            "travel photos", "event photos", "wedding photos", "birthday photos", "nature photos",
            "street photos", "raw images", "edited photos", "final images", "scans", "prints", "artwork",
            "illustrations", "graphics", "icons", "logos", "thumbnails", "references", "inspiration", "concept art"
        }
    },

    // -------- VIDEOS --------
    { "Video Files",
        {
            "video", "videos", "vid", "vids", "movie", "movies", "films", "clips", "recordings", "lectures",
            "screen captures", "tutorial videos","courses", "vlogs", "reels", "shorts", "vacation videos",
            "travel videos", "family videos", "event videos", "wedding videos", "gameplay", "walkthroughs",
            "streams", "webinars", "meetings recordings", "interviews", "trailers", "screen recordings",
            "edits", "final cuts", "raw footage", "b roll", "montage", "highlights", "dashcam",
            "timelapse", "slow motion", "drone footage"
        }
    },

    // -------- AUDIO --------
    { "Audio Files",
        {
            "audio", "audios", "music", "songs", "tracks", "albums", "playlist", "playlists", "podcast",
            "audiobooks", "voice notes", "voice recordings", "lectures audio", "interviews audio", "sfx",
            "meetings audio", "sound effects", "background music", "instrumentals", "beats", "loops",
            "samples", "recordings", "live recordings", "concerts", "practice", "rehearsals", "demos",
            "draft mixes", "final mixes", "masters", "exports", "ringtones", "notifications",
            "alarms", "ambient sounds", "nature sounds", "podcasts"
        }
    },

    // -------- DOCUMENTS --------
    { "Text Files",
        {
            "text", "texts", "text files", "txt files", "notes", "plain text", "logs", "markdown",
            "readme", "documentation", "draft notes"
        }
    },

    { "PDF Files",
        {
            "pdf", "pdfs", "pdf files", "documents pdf", "manuals pdf", "ebooks", "reports pdf",
            "invoices pdf", "statements pdf", "scanned pdfs"
        }
    },

    { "Word Files",
        {
            "word", "word files", "documents word", "doc files", "docx files", "letters", "reports word",
            "essays", "assignments", "resumes", "cover letters"
        }
    },

    { "Excel Files",
        {
            "excel", "excel files", "spreadsheets", "sheets", "financial sheets", "budgets", "expenses",
            "accounts", "tracking sheets", "reports excel", "tables"
        }
    },

    { "PowerPoint Files",
        {
            "powerpoint", "powerpoint files", "presentations", "slides", "ppt files", "pptx files",
            "pitch decks", "lecture slides", "meeting slides"
        }
    },

    // -------- PROGRAMMING LANGUAGES --------
    { "C Files",
        {
            "c", "c files", "c source", "c language", "c programs"
        }
    },

    { "C++ Files",
        {
            "cpp", "c++", "cplusplus", "cpp files", "c++ source", "c++ programs"
        }
    },

    { "Java Files",
        {
            "java", "java files", "java source", "java programs"
        }
    },

    { "Python Files",
        {
            "python", "python files", "python source", "py scripts", "python programs", "python scripts",
        }
    },

    { "JavaScript Files",
        {
            "javascript", "javascript files", "js files", "js source"
        }
    },

    { "TypeScript Files",
        {
            "typescript", "typescript files", "ts files", "ts source"
        }
    },

    { "Web Files",
        {
            "web","web files","html files","css files","frontend","frontend files"
        }
    },

    { "Shell Scripts",
        {
            "shell", "shell scripts", "bash scripts", "terminal scripts"
        }
    },

    { "Go Files",
        {
            "go", "golang", "go files", "go source", "go programs"
        }
    },

    { "Rust Files",
        {
            "rust", "rust files", "rust source", "rs files", "rust programs"
        }
    },

    { "PHP Files",
        {
            "php", "php files", "php source", "php scripts"
        }
    },


    // -------- DATABASE --------
    { "Database Files",
        {
            "database", "databases", "db", "db files", "sqlite", "sql files"
        }
    },

    // -------- ARCHIVES --------
    { "Archive Files",
        {
            "archive", "archives", "compressed", "compressed files", "zip files", "rar files", "backups",
            "backup archives"
        }
    },

    // -------- EXECUTABLES / BINARIES --------
    { "Executable Files",
        {
            "executables", "binaries", "apps", "applications", "programs", "installers"
        }
    },

    // -------- LIBRARIES --------
    { "Library Files",
        {
            "libraries", "libs", "shared libraries", "static libraries"
        }
    },

    // -------- CONFIG --------
    { "Config Files",
        {
            "config", "configs", "configuration", "settings", "env files", "environment config"
        }
    }
};

/*
    =========================================================
        ALIAS_LOOKUP
    =========================================================

    Reverse lookup table:
        alias -> canonical category

    Built ONCE at program startup.

    This gives us:
        O(1) lookup when normalizing folders.
*/
const std::map<std::string, std::string> ALIAS_LOOKUP = []
{
    std::map<std::string, std::string> lookup;

    for (const std::pair< std::string, std::set<std::string> >& category : CATEGORY_ALIAS_MAP ) {
        for (const std::string& alias : category.second) {
            lookup[alias] = category.first;
        }
    }

    return lookup;
}();


/*
    =========================================================
        validate_path
    =========================================================

    Ensures the provided root path is:
    - existing
    - a directory
    - accessible (permission-safe)

    WHY NOT JUST std::filesystem::exists?
    -------------------------------------
    Because existence ≠ permission.
*/
path_status validate_path ( const std::string& root_path )
{
    if ( ! std::filesystem::exists(root_path) )
    {
        return path_status::not_found;
    }
    else if ( ! std::filesystem::is_directory(root_path) )
    {
        return path_status::not_directory;
    }
    bool accessible = true;
    try {
        // Attempting directory_iterator construction to detect permission issues
        std::filesystem::directory_iterator varname(root_path);
        accessible = true;
    }
    catch (std::filesystem::filesystem_error& e) {
        if (e.code() == std::errc::permission_denied)
        {
            accessible = false;
        }
    }
    if (accessible == false)
    {
       return path_status::permission_denied;
    }
    else if (std::filesystem::exists(root_path) && std::filesystem::is_directory(root_path) && accessible ) {
        return path_status::ok;
    }
    else
    {
        return path_status::unknown_path_error;
    }
}
/*
    =========================================================
        get_parent_folder_name
    =========================================================

    Extracts the immediate directory name from a path.

    Example:
        "/home/user/Documents/file.txt" -> "Documents"
*/
std::string get_parent_folder_name(const std::string& current_directory_level_path)
{
    std::filesystem::path p(current_directory_level_path);
    return p.filename().string();
}

/*
    =========================================================
        normalize_category_folder
    =========================================================

    Normalizes alias folders into canonical category folders
    at the CURRENT directory level only.

    IMPORTANT DESIGN RULE:
    ----------------------
    - No recursion here.
    - Only rename folders, never create new ones.
*/
void normalize_category_folder( const std::string& current_directory_level_path )
{
    std::map<std::string, std::filesystem::path> canonical_folder_map;

    for (const std::filesystem::directory_entry& entry: std::filesystem::directory_iterator(current_directory_level_path))
    {
        if ( ! entry.is_directory() )
        {
            continue;
        }

        std::string folder_entry_name = entry.path().filename().string();

        // Skip already-canonical folders
        if (CANONICAL_NAMES.find(folder_entry_name) != CANONICAL_NAMES.end())
        {
            continue;
        }

        // Normalize for lookup
        std::transform(folder_entry_name.begin(), folder_entry_name.end(), folder_entry_name.begin(), ::tolower);

        std::map<std::string, std::string>::const_iterator it = ALIAS_LOOKUP.find(folder_entry_name);

        if (it != ALIAS_LOOKUP.end())
        {
            const std::string canonical_name = it->second;

            // Keep first occurrence only
            if ( canonical_folder_map.find(canonical_name) == canonical_folder_map.end())
            {
                canonical_folder_map[canonical_name] = entry.path();
            }
        }
    }
    
    // Rename alias folders to canonical names
    for ( const std::pair<std::string, std::filesystem::path>& it : canonical_folder_map)
    {
        std::string canonical_folder_name = it.first;
        std::filesystem::path old_path(it.second);
        std::filesystem::path new_path = old_path.parent_path() / canonical_folder_name;

        if (old_path.filename() != canonical_folder_name)
        {
            try {
                std::filesystem::rename(old_path, new_path);
            } catch (...) {
                // If rename fails (permissions, etc.), just skip it.
            }
        }
    }
}

/*
    =========================================================
        create_directory
    =========================================================

    Safely creates a directory if it doesn't exist.

    Does NOT throw.
    Returns explicit status instead.
*/
create_directory_status create_directory ( const std::string& target_directory_path )
{
    if ( ! std::filesystem::exists(target_directory_path) )
    {
        try
        {
            std::filesystem::create_directory(target_directory_path);
            return create_directory_status::successful_creation;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            if (e.code() == std::errc::permission_denied)
            {
                return create_directory_status::permission_denied_failure;
            }
            else
            {
                return create_directory_status::unknown_failure;
            }
        }
    }
    return create_directory_status::already_exists;
}

/*
    =========================================================
        get_unique_path
    =========================================================

    Implementation details:
    1. Check if the "base" path is available.
    2. If not, split the filename into "stem" (name) and "extension".
    3. Loop with a counter until a non-existent path is found.
*/
std::filesystem::path get_unique_path(const std::filesystem::path& destination_dir, const std::string& filename)
{
    // Construct the initial target path
    std::filesystem::path target_path = destination_dir / filename;

    // CASE 1: The name is already unique. Return immediately.
    if (!std::filesystem::exists(target_path))
    {
        return target_path;
    }

    // CASE 2: Collision detected. We must find a new name.

    // Split "document.pdf" into "document" and ".pdf"
    // We use temporary path objects to handle dots correctly.
    std::filesystem::path temp_path(filename);
    std::string stem = temp_path.stem().string();
    std::string extension = temp_path.extension().string();

    int counter = 1;
    do
    {
        // Construct: "stem(counter)extension" -> "document(1).pdf"
        std::string new_filename = stem + "(" + std::to_string(counter) + ")" + extension;
        target_path = destination_dir / new_filename;

        counter++;

    } while (std::filesystem::exists(target_path)); // Keep looking until free

    return target_path;
}

/*
    =========================================================
        atomic_file_transfer
    =========================================================

    Attempts to move a file using filesystem::rename.

    Handles name collisions by appending:
        filename(1).ext, filename(2).ext, ...
*/
file_move_status atomic_file_transfer ( const std::string& source_path, const std::string& destination_dir_path )
{
    try
    {
        std::filesystem::path old_source_path(source_path);

        // making a path object of destination_dir_path
        std::filesystem::path destination_dir(destination_dir_path);

        // Use our new helper to get a safe path
        std::filesystem::path new_unique_destination = get_unique_path(destination_dir, old_source_path.filename().string());

        std::filesystem::rename(old_source_path, new_unique_destination);

        return file_move_status::successful_transfer;
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        if (e.code() == std::errc::permission_denied)
        {
            return file_move_status::permission_denied;
        }
        else if (e.code() == std::errc::cross_device_link)
        {
            return file_move_status::cross_device_error;
        }
        else
        {
            return file_move_status::unknown_failure;
        }
    }
}

/*
    =========================================================
        fallback_transfer
    =========================================================

    Used when atomic transfer is not possible.

    Strategy:
    - Copy file
    - Verify success
    - Delete original
*/
file_move_status fallback_transfer ( const std::string& source_path, const std::string& destination_dir_path )
{
    try
    {
        std::filesystem::path old_source_path(source_path);

        // making a path object of destination_dir_path
        std::filesystem::path destination_dir(destination_dir_path);

        // Use our new helper to get a safe path
        std::filesystem::path new_unique_destination = get_unique_path(destination_dir, old_source_path.filename().string());

        std::filesystem::copy_file(
            old_source_path,
            new_unique_destination,
            std::filesystem::copy_options::overwrite_existing
        );

        std::filesystem::remove(old_source_path);
        return file_move_status::successful_transfer;
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        if (e.code() == std::errc::permission_denied)
        {
            return file_move_status::permission_denied;
        }
        else
        {
            return file_move_status::unknown_failure;
        }
    }
}
