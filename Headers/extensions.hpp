#pragma once
#include <string>
#include <map>
#include <set>

/*
    ============================
        extensions.hpp
    ============================

    This header acts as the "knowledge base" for file classification.

    Responsibilities of this file:
    1. Declare all file categories in a human-readable way
    2. Declare which extensions belong to which category
    3. Expose fast lookup structures for classification
    4. Provide a single function to classify any file path

    IMPORTANT DESIGN IDEA:
    ----------------------
    - No logic for directory traversal lives here
    - No filesystem mutation happens here
    - This file ONLY answers the question:
        "Given a file, what category does it belong to?"
*/

/*
    CATEGORY_EXTENSION_MAP
    ----------------------
    Maps:
        Category Name  →  Set of file extensions

    Example:
        "Image Files" → { "jpg", "png", "webp", ... }

    Rules:
    - Extensions must NOT include a dot (.)
    - All extensions must be lowercase
    - This is the ONLY place where extensions are added or removed
*/
extern const std::map<std::string, std::set<std::string>> CATEGORY_EXTENSION_MAP;


/*
    EXTENSION_LOOKUP
    ----------------
    Maps:
        file extension → category name

    Example:
        "jpg" → "Image Files"

    Why this exists:
    - Avoids scanning every category for every file
    - Enables O(1) classification time
    - Built once at program startup
*/
extern const std::map<std::string, std::string> EXTENSION_LOOKUP;


/*
    CANONICAL_NAMES
    ---------------
    Contains the official / allowed category folder names.

    Purpose:
    - Helps validate category names
    - Prevents accidental folder creation due to typos
    - Acts as a "source of truth" for filesystem layers
*/
extern const std::set<std::string> CANONICAL_NAMES;


/*
    classify_file_by_extension
    --------------------------
    Input:
        Full file path or file name as a string

    Output:
        Category name (folder name) based on file extension

    Behavior:
    - Extracts extension safely using std::filesystem
    - Normalizes extension (lowercase, no dot)
    - Uses EXTENSION_LOOKUP for fast classification
    - Returns "Others" if extension is unknown or missing
*/
std::string classify_file_by_extension(const std::string& file_path);
