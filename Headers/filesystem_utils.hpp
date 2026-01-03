#pragma once

#include <filesystem>
#include <cctype>
#include <string>
#include <map>
#include <set>

/*
    ===============================
        filesystem_utils.hpp
    ===============================

    This header defines the public interface for all filesystem-related
    operations used by the project.

    DESIGN PHILOSOPHY:
    ------------------
    - This module does NOT decide *what* category a file belongs to
      (that is handled by extensions / classification logic).
    - This module handles *how* the filesystem is validated, created,
      normalized, and modified safely.

*/


/*
    path_status
    -----------
    Represents the result of validating a filesystem path.

    Used before performing any operation that assumes:
    - path exists
    - path is accessible
    - path is a directory
*/
enum class path_status
{
    ok,                     // Path exists, is accessible, and is a directory
    not_found,              // Path does not exist
    not_directory,          // Path exists but is not a directory
    permission_denied,      // Path exists but cannot be accessed
    unknown_path_error      // Any unexpected filesystem failure
};


/*
    create_directory_status
    -----------------------
    Represents the outcome of attempting to create a directory.

    Explicit status avoids:
    - guessing based on exceptions
    - silent failures
*/
enum class create_directory_status
{
    already_exists,         // Directory already present
    successful_creation,    // Directory created successfully
    permission_denied_failure,
    unknown_failure
};


/*
    file_move_status
    ----------------
    Represents the outcome of moving a file between locations.

    Used for both atomic and fallback move strategies.
*/
enum class file_move_status
{
    successful_transfer,    // File moved successfully
    permission_denied,      // Access denied at source or destination
    cross_device_error,     // Filesystem does not support atomic move
    unknown_failure
};


/*
    CATEGORY_ALIAS_MAP
    ------------------
    Maps:
        Canonical Category Name → Set of alternative folder names (aliases)

    Purpose:
    - Allows user-friendly or legacy folder names
    - Prevents duplicate category folders
    - Helps normalize messy directory structures

    Example:
        "Image Files" → { "Images", "Pictures", "Pics" }
*/
extern const std::map<std::string, std::set<std::string>> CATEGORY_ALIAS_MAP;


/*
    ALIAS_LOOKUP
    ------------
    Maps:
        Alias name → Canonical category name

    Built once for fast lookup during normalization.
*/
extern const std::map<std::string, std::string> ALIAS_LOOKUP;


/*
    validate_path
    -------------
    Verifies whether the given path:
    - exists
    - is a directory
    - is accessible

    This function should be called BEFORE any filesystem traversal.
*/
path_status validate_path(const std::string& root_path);


/*
    get_parent_folder_name
    ----------------------
    Extracts the immediate parent directory name of a filesystem entry.

    Example:
        "/home/user/Documents/file.txt" → "Documents"

    Used for:
    - detecting misplaced files
    - category folder normalization
*/
std::string get_parent_folder_name(const std::string& entry_path);


/*
    normalize_category_folder
    -------------------------
    Ensures that category folders follow canonical naming.

    Responsibilities:
    - Detect alias folder names
    - Merge alias folders into canonical folders
    - Prevent duplicate category directories

    IMPORTANT:
    ----------
    This function operates at a SINGLE directory level.
    It does NOT recurse.
*/
void normalize_category_folder(const std::string& current_directory_level_path);

/*
    create_directory
    ----------------
    Attempts to create a directory safely.

    Behavior:
    - Does nothing if directory already exists
    - Returns explicit status instead of throwing
*/
create_directory_status create_directory(
    const std::string& target_directory_path
);

/*
    get_unique_path
    ---------------
    Generates a unique filesystem path to prevent accidental data loss.

    WHY THIS IS NECESSARY:
    ----------------------
    When moving "image.jpg" into a folder that already contains "image.jpg",
    standard filesystem operations will either fail or overwrite the existing
    file. This function detects the collision and generates a new name.

    Input:
        - destination_dir: The directory where the file is headed.
        - filename: The original name of the file (e.g., "vacation.png").

    Output:
        - A full std::filesystem::path that is guaranteed to be unique
          within the destination directory.

    Naming Strategy:
        - "file.txt" -> "file(1).txt" -> "file(2).txt" ...
*/
std::filesystem::path get_unique_path(
    const std::filesystem::path& destination_dir,
    const std::string& filename
);

/*
    atomic_file_transfer
    --------------------
    Attempts to move a file using an atomic filesystem operation.

    Guarantees:
    - No partial copy
    - Fast operation

    Limitations:
    - May fail across different filesystems/devices
*/
file_move_status atomic_file_transfer(
    const std::string& source_path,
    const std::string& destination_path
);


/*
    fallback_transfer
    -----------------
    Used when atomic transfer fails.

    Strategy:
    - Copy file to destination
    - Verify success
    - Remove original file

    Slower, but more portable.
*/
file_move_status fallback_transfer(
    const std::string& source_path,
    const std::string& destination_path
);
