#pragma once
#include <string>

/*
    =========================================================
        organize_status
    =========================================================

    Represents the final outcome of an organize operation.

    DESIGN PHILOSOPHY:
    ------------------
    - No exceptions leaking out of core logic
    - Every failure is explicit and meaningful
    - Caller can decide how to report / recover
*/
enum class organize_status
{
    success,                        // Everything went fine
    path_not_found,                 // Root path does not exist
    not_a_directory,                // Path exists but is not a directory
    permission_denied,              // OS denied access at some point
    directory_creation_failed,      // Failed to create category folder
    already_in_correct_location,    // File was already where it belongs
    atomic_transfer_failed,         // rename() failed due to cross-device issue
    fallback_transfer_failed,       // copy + delete failed
    unknown_error                   // Catch-all for unexpected failures
};

/*
    =========================================================
        transfer_mode
    =========================================================

    Determines how files are moved.

    atomic_transfer_mode:
        - Uses filesystem::rename
        - Fast and atomic
        - Fails across different filesystems

    fallback_transfer_mode:
        - Uses copy + delete
        - Slower
        - Works across devices
*/
enum class transfer_mode
{
    atomic_transfer_mode,
    fallback_transfer_mode
};

/*
    =========================================================
        Public API
    =========================================================
*/

/*
    Validates and organizes the given directory.

    This is the main entry point exposed to users.
*/
organize_status check_directory(const std::string& root_path);

/*
    Handles a single file at a specific directory level.

    Responsibilities:
    - Classify file by extension
    - Decide correct destination
    - Prevent invalid nesting inside category folders
    - Move file safely
*/
organize_status handle_file(
    const std::string& root_path,
    const std::string& entry_path,
    transfer_mode t_mode
);

/*
    Iteratively walks the directory tree and organizes files.

    - No recursion (stack-safe)
    - Works on any depth
    - Idempotent: safe to run multiple times
*/
organize_status organize_directory(
    const std::string& root_path,
    transfer_mode t_mode
);
