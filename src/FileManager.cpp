#include "FileManager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

FileManager::FileManager() {
    currentPath = fs::current_path();
}

void FileManager::run() {
    int choice;

    do {
        std::cout << "\n==============================\n";
        std::cout << "Current directory: " << currentPath << "\n";
        std::cout << "==============================\n";

        printMenu();

        std::cout << "Choose action: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
            case 1:
                listFiles();
                break;
            case 2:
                changeDirectory();
                break;
            case 3:
                viewTextFile();
                break;
            case 4:
                searchFile();
                break;
            case 5:
                deleteFile();
                break;
            case 6:
                copyFile();
                break;
            case 0:
                std::cout << "Goodbye!\n";
                break;
            default:
                std::cout << "Unknown command.\n";
        }

    } while (choice != 0);
}

void FileManager::printMenu() const {
    std::cout << "\nMenu:\n";
    std::cout << "1. Show files and folders\n";
    std::cout << "2. Change directory\n";
    std::cout << "3. View text file\n";
    std::cout << "4. Search file by name\n";
    std::cout << "5. Delete file\n";
    std::cout << "6. Copy file\n";
    std::cout << "0. Exit\n";
}

void FileManager::listFiles() const {
    std::vector<fs::directory_entry> entries;

    try {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            entries.push_back(entry);
        }
    } catch (const fs::filesystem_error& error) {
        std::cout << "Error reading directory: " << error.what() << "\n";
        return;
    }

    std::sort(entries.begin(), entries.end(),
        [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.path().filename().string() < b.path().filename().string();
        }
    );

    std::cout << "\nFiles and folders:\n";

    if (entries.empty()) {
        std::cout << "Directory is empty.\n";
        return;
    }

    for (const auto& entry : entries) {
        if (entry.is_directory()) {
            std::cout << "[DIR]  ";
        } else {
            std::cout << "[FILE] ";
        }

        std::cout << entry.path().filename().string();

        if (entry.is_regular_file()) {
            std::cout << " | size: " << entry.file_size() << " bytes";
        }

        std::cout << "\n";
    }
}

void FileManager::changeDirectory() {
    std::string folderName;

    std::cout << "Enter folder name or .. to go back: ";
    std::getline(std::cin, folderName);

    if (folderName == "..") {
        if (currentPath.has_parent_path()) {
            currentPath = currentPath.parent_path();
        }
        return;
    }

    fs::path newPath = currentPath / folderName;

    if (fs::exists(newPath) && fs::is_directory(newPath)) {
        currentPath = newPath;
    } else {
        std::cout << "Directory not found.\n";
    }
}

void FileManager::viewTextFile() const {
    std::string fileName;

    std::cout << "Enter text file name: ";
    std::getline(std::cin, fileName);

    fs::path filePath = currentPath / fileName;

    if (!fs::exists(filePath)) {
        std::cout << "File does not exist.\n";
        return;
    }

    if (!fs::is_regular_file(filePath)) {
        std::cout << "This is not a regular file.\n";
        return;
    }

    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cout << "Cannot open file.\n";
        return;
    }

    std::cout << "\nContent of file \"" << fileName << "\":\n";
    std::cout << "------------------------------\n";

    std::string line;

    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }

    std::cout << "------------------------------\n";
}

void FileManager::searchFile() const {
    std::string query;

    std::cout << "Enter part of file name: ";
    std::getline(std::cin, query);

    if (query.empty()) {
        std::cout << "Search query is empty.\n";
        return;
    }

    bool found = false;

    std::cout << "\nSearch results:\n";

    try {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            std::string fileName = entry.path().filename().string();

            if (fileName.find(query) != std::string::npos) {
                if (entry.is_directory()) {
                    std::cout << "[DIR]  ";
                } else {
                    std::cout << "[FILE] ";
                }

                std::cout << fileName << "\n";
                found = true;
            }
        }
    } catch (const fs::filesystem_error& error) {
        std::cout << "Search error: " << error.what() << "\n";
        return;
    }

    if (!found) {
        std::cout << "No files found.\n";
    }
}

void FileManager::deleteFile() {
    std::string fileName;

    std::cout << "Enter file name to delete: ";
    std::getline(std::cin, fileName);

    fs::path filePath = currentPath / fileName;

    if (!fs::exists(filePath)) {
        std::cout << "File does not exist.\n";
        return;
    }

    if (!fs::is_regular_file(filePath)) {
        std::cout << "You can delete only regular files using this command.\n";
        return;
    }

    std::string answer;

    std::cout << "Are you sure you want to delete \"" << fileName << "\"? yes/no: ";
    std::getline(std::cin, answer);

    if (answer != "yes") {
        std::cout << "Deleting cancelled.\n";
        return;
    }

    try {
        fs::remove(filePath);
        std::cout << "File deleted successfully.\n";
    } catch (const fs::filesystem_error& error) {
        std::cout << "Delete error: " << error.what() << "\n";
    }
}

void FileManager::copyFile() const {
    std::string sourceName;
    std::string targetName;

    std::cout << "Enter source file name: ";
    std::getline(std::cin, sourceName);

    std::cout << "Enter new file name: ";
    std::getline(std::cin, targetName);

    fs::path sourcePath = currentPath / sourceName;
    fs::path targetPath = currentPath / targetName;

    if (!fs::exists(sourcePath)) {
        std::cout << "Source file does not exist.\n";
        return;
    }

    if (!fs::is_regular_file(sourcePath)) {
        std::cout << "Source is not a regular file.\n";
        return;
    }

    try {
        fs::copy_file(
            sourcePath,
            targetPath,
            fs::copy_options::overwrite_existing
        );

        std::cout << "File copied successfully.\n";
    } catch (const fs::filesystem_error& error) {
        std::cout << "Copy error: " << error.what() << "\n";
    }
}