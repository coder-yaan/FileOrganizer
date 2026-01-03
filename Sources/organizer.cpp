#include "organizer.hpp"
#include "filesystem_utils.hpp"
#include "extensions.hpp"

#include <filesystem>
#include <vector>

/*
    =========================================================
        process_path_validation
    =========================================================

    Converts low-level filesystem path_status
    into high-level organize_status.

    This keeps the rest of the organizer logic
    clean and independent from filesystem details.
*/
organize_status process_path_validation (const std::string& root_path)
{
    path_status ps = validate_path(root_path);

    switch (ps)
    {
    case path_status::ok:
    {
        return organize_status::success;
    }
    case path_status::not_found:
    {
        return organize_status::path_not_found;
    }
    case path_status::not_directory:
    {
        return organize_status::not_a_directory;
    }
    case path_status::permission_denied:
    {
        return organize_status::permission_denied;
    }
    case path_status::unknown_path_error:
    {
        return organize_status::unknown_error;
    }
    default:{
        return organize_status::unknown_error;
    }
    }
}

/*
    =========================================================
        handle_file
    =========================================================

    Core decision-making unit of the project.

    GIVEN:
    - current_directory_level_path → where we are scanning
    - entry_path → full path of the file
    - transfer_mode → how to move files

    GOAL:
    -----
    - Decide where the file SHOULD live
    - Avoid creating nested category folders
    - Move file safely and predictably
*/
organize_status handle_file(
    const std::string& current_directory_level_path,
    const std::string& entry_path,
    transfer_mode t_mode
    )
{
    // Determine category purely from file extension
    std::string category_name = classify_file_by_extension(entry_path);

    // Name of the directory we are currently inside
    std::string parent_folder_name = get_parent_folder_name(current_directory_level_path);

    /*
        If file is already inside its correct category folder,
        do absolutely nothing.

        This is what makes the program idempotent.
    */
    if (category_name == parent_folder_name)
    {
        return organize_status::already_in_correct_location;
    }

    /*
        If file is already inside its correct category's alias folder,
        again do absolutely nothing.

        REASON: we already normalized our folders in current directory level but if there are more than one alias,
        we only keep the files which maybe appropriate for that folder and other files are moved out

        Example: a directory has two more folders "pics", "photos"
        - anyone's name is changes to canonical name
        - other one is untouched
        - when the program enters that folder, for ex it is "pics", any image related extension file is appropriate there
        - not changing those files location, because for user, it maybe not be a good idea
        - only files those files are moved out whose category defined by extension,
            is not equals to canonical name of current alias folder
    */

    if ( ( ALIAS_LOOKUP.find(parent_folder_name) != ALIAS_LOOKUP.end() )
        && ( ALIAS_LOOKUP.at(parent_folder_name) == category_name ) )
    {
        return organize_status::already_in_correct_location;
    }
    /*
        Decide BASE LOCATION for destination.

        RULE:
        -----
        If the file currently lives inside a canonical category folder
        BUT does NOT belong to that category,
        then move it OUT to the parent directory.

        This prevents:
            Image Files/
                PDF Files/ (nested category mess)
    */
    std::filesystem::path base_location;

    if ( (CANONICAL_NAMES.find(parent_folder_name) != CANONICAL_NAMES.end())
        && (category_name != parent_folder_name) )
    {
        // Move out of the wrong category folder
        base_location = std::filesystem::path(current_directory_level_path).parent_path();
    }
    else if ( ( ALIAS_LOOKUP.find(parent_folder_name) != ALIAS_LOOKUP.end() )
        && ( ALIAS_LOOKUP.at(parent_folder_name) != category_name ) )
    {
        // Move out of the wrong category folder
        base_location = std::filesystem::path(current_directory_level_path).parent_path();
    }
    else
    {
        // Normal case: organize within the same directory level
        base_location = current_directory_level_path;
    }

    // Final destination directory for this file
    std::filesystem::path destination_directory = base_location / category_name;
    std::string destination_dir_path = destination_directory.string();

    // Ensure category directory exists
    create_directory_status creation_result = create_directory(destination_dir_path);

    /*
        =====================================================
            FILE TRANSFER
        =====================================================
    */
    if (t_mode == transfer_mode::atomic_transfer_mode)
    {
        file_move_status atomic_transfer_result =
            atomic_file_transfer(entry_path, destination_dir_path);

        if ((creation_result == create_directory_status::successful_creation ||
             creation_result == create_directory_status::already_exists)
            && atomic_transfer_result == file_move_status::successful_transfer)
        {
            return organize_status::success;
        }
        else if (creation_result == create_directory_status::permission_denied_failure ||
                 atomic_transfer_result == file_move_status::permission_denied)
        {
            return organize_status::permission_denied;
        }
        else if (atomic_transfer_result == file_move_status::cross_device_error)
        {
            return organize_status::atomic_transfer_failed;
        }
        else if (creation_result == create_directory_status::unknown_failure)
        {
            return organize_status::directory_creation_failed;
        }
        else
        {
            return organize_status::unknown_error;
        }
    }
    else
    {
        /*
            Fallback mode: copy + delete
            Used when atomic rename is not possible
        */
        file_move_status fallback_transfer_result =
            fallback_transfer(entry_path, destination_dir_path);

        if ((creation_result == create_directory_status::successful_creation ||
             creation_result == create_directory_status::already_exists)
            && fallback_transfer_result == file_move_status::successful_transfer)
        {
            return organize_status::success;
        }
        else if (creation_result == create_directory_status::permission_denied_failure ||
                 fallback_transfer_result == file_move_status::permission_denied)
        {
            return organize_status::permission_denied;
        }
        else if (creation_result == create_directory_status::unknown_failure)
        {
            return organize_status::directory_creation_failed;
        }
        else
        {
            return organize_status::fallback_transfer_failed;
        }
    }
}


/*
    =========================================================
        organize_directory
    =========================================================

    High-level orchestrator.

    KEY DESIGN CHOICES:
    -------------------
    - Uses explicit stack (vector) instead of recursion
    - Safe for deeply nested directories
    - Processes folders level by level
*/
organize_status organize_directory(const std::string& root_path, transfer_mode t_mode)
{
    // Validate root path before doing anything destructive
    organize_status root_path_state = process_path_validation(root_path);
    if (root_path_state != organize_status::success)
    {
        return root_path_state;
    }

    // Manual stack of directories to process
    std::vector<std::string> directories;
    directories.push_back(root_path);

    while (!directories.empty())
    {
        // Pop one directory from stack
        std::string current_directory_level_path = directories.back();
        directories.pop_back();

        /*
            Normalize folder names at this level first.

            This ensures:
            - "images", "pics", etc → "Image Files"
            - No duplicate category folders are created
        */
        normalize_category_folder(current_directory_level_path);

        // Iterate through current directory contents
        for (const std::filesystem::directory_entry& entry_in_directory :
             std::filesystem::directory_iterator(current_directory_level_path))
        {
            std::string entry_path = entry_in_directory.path().string();

            if (std::filesystem::is_regular_file(entry_path))
            {
                organize_status s =
                    handle_file(current_directory_level_path, entry_path, t_mode);

                // Already correct = silently skip
                if (s == organize_status::already_in_correct_location)
                {
                    continue;
                }
                // Any real failure stops the operation
                else if (s != organize_status::success)
                {
                    return s;
                }
            }
            else if (std::filesystem::is_directory(entry_path))
            {
                // Skip hidden/system directories
                std::string name = entry_in_directory.path().filename().string();
                if (!name.empty() && name[0] != '.')
                {
                    directories.push_back(entry_path);
                }
            }
        }
    }

    return organize_status::success;
}
