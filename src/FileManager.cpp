#include "FileManager.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <sys/ioctl.h>
#include <unistd.h>

namespace fs = std::filesystem;

FileManager::FileManager() {
    currentPath = fs::current_path();
    mode = Mode::FileList;
    scrollOffset = 0;
    lastMessage = "";
    refreshEntries();
}

void FileManager::run() {
    std::string command;

    while (true) {
        draw();

        std::getline(std::cin, command);

        if (command == "exit" || command == "quit") {
            break;
        }

        handleCommand(command);
    }
}

FileManager::TerminalSize FileManager::getTerminalSize() const {
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    TerminalSize size;
    size.rows = w.ws_row;
    size.cols = w.ws_col;

    if (size.rows <= 0) {
        size.rows = 24;
    }

    if (size.cols <= 0) {
        size.cols = 80;
    }

    return size;
}

void FileManager::clearScreen() const {
    std::cout << "\033[2J\033[H";
}

void FileManager::draw() {
    TerminalSize size = getTerminalSize();

    clearScreen();

    drawHeader(size);

    if (mode == Mode::FileList) {
        drawFileTable(size);
    } else {
        drawTextView(size);
    }

    drawMessageLine(size);
    drawMenu(size);
    drawPrompt();

    std::cout.flush();
}

void FileManager::drawHeader(const TerminalSize& size) const {
    std::string title;

    if (mode == Mode::FileList) {
        title = "Current directory: " + currentPath.string();
    } else {
        title = "Viewing file: " + viewedFileName;
    }

    std::cout << cutText(title, size.cols) << "\n";
}

int FileManager::getContentHeight(const TerminalSize& size) const {
    int reservedLines = 5;
    int contentHeight = size.rows - reservedLines;

    if (contentHeight < 3) {
        contentHeight = 3;
    }

    return contentHeight;
}

std::string FileManager::cutText(const std::string& text, int maxWidth) const {
    if (maxWidth <= 0) {
        return "";
    }

    if ((int)text.size() <= maxWidth) {
        return text;
    }

    if (maxWidth <= 3) {
        return text.substr(0, maxWidth);
    }

    return text.substr(0, maxWidth - 3) + "...";
}

void FileManager::refreshEntries() {
    entries.clear();

    try {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            entries.push_back(entry);
        }

        std::sort(entries.begin(), entries.end(),
            [](const fs::directory_entry& a, const fs::directory_entry& b) {
                if (a.is_directory() != b.is_directory()) {
                    return a.is_directory() > b.is_directory();
                }

                return a.path().filename().string() < b.path().filename().string();
            }
        );
    } catch (const fs::filesystem_error& error) {
        lastMessage = "Failed to read directory: " + std::string(error.what());
    }
}

void FileManager::drawFileTable(const TerminalSize& size) const {
    int contentHeight = getContentHeight(size);

    int nameWidth = size.cols - 30;

    if (nameWidth < 10) {
        nameWidth = 10;
    }

    std::cout << cutText("Name", nameWidth)
              << " | Type       | Size\n";

    std::cout << std::string(std::min(size.cols, 80), '-') << "\n";

    int tableRows = contentHeight - 2;

    if (tableRows < 1) {
        tableRows = 1;
    }

    int start = scrollOffset;
    int end = std::min(start + tableRows, (int)entries.size());

    if (scrollOffset > 0 && tableRows > 0) {
        std::cout << "...\n";
        start++;
    }

    for (int i = start; i < end; ++i) {
        const auto& entry = entries[i];

        std::string name = entry.path().filename().string();
        std::string type = entry.is_directory() ? "directory" : "file";
        std::string sizeText = "-";

        if (entry.is_regular_file()) {
            try {
                sizeText = std::to_string(entry.file_size()) + " b";
            } catch (...) {
                sizeText = "?";
            }
        }

        std::cout << cutText(name, nameWidth)
                  << " | "
                  << cutText(type, 10)
                  << " | "
                  << cutText(sizeText, 10)
                  << "\n";
    }

    if (end < (int)entries.size()) {
        std::cout << "...\n";
    }

    int printedRows = end - start + 2;

    while (printedRows < contentHeight) {
        std::cout << "\n";
        printedRows++;
    }
}

void FileManager::drawTextView(const TerminalSize& size) const {
    int contentHeight = getContentHeight(size);

    int start = scrollOffset;
    int end = std::min(start + contentHeight, (int)textLines.size());

    if (scrollOffset > 0 && contentHeight > 0) {
        std::cout << "...\n";
        start++;
    }

    for (int i = start; i < end; ++i) {
        std::cout << cutText(textLines[i], size.cols) << "\n";
    }

    if (end < (int)textLines.size()) {
        std::cout << "...\n";
    }

    int printedRows = end - start;

    if (scrollOffset > 0) {
        printedRows++;
    }

    if (end < (int)textLines.size()) {
        printedRows++;
    }

    while (printedRows < contentHeight) {
        std::cout << "\n";
        printedRows++;
    }
}

void FileManager::drawMessageLine(const TerminalSize& size) const {
    std::cout << cutText(lastMessage, size.cols) << "\n";
}

void FileManager::drawMenu(const TerminalSize& size) const {
    std::vector<std::string> commands;

    if (mode == Mode::FileList) {
        commands = {
            "ls",
            "cd <folder>",
            "open <file>",
            "search <text>",
            "copy <from> <to>",
            "delete <file>",
            "scroll up",
            "scroll down",
            "exit"
        };
    } else {
        commands = {
            "scroll up",
            "scroll down",
            "return"
        };
    }

    std::string line;

    for (const std::string& command : commands) {
        std::string item = "[" + command + "] ";

        if ((int)(line.size() + item.size()) > size.cols) {
            std::cout << line << "\n";
            line.clear();
        }

        line += item;
    }

    if (!line.empty()) {
        std::cout << line << "\n";
    }
}

void FileManager::drawPrompt() const {
    std::cout << "$> ";
}

void FileManager::handleCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }

    if (mode == Mode::TextView) {
        if (command == "scroll up") {
            scrollUp();
        } else if (command == "scroll down") {
            scrollDown();
        } else if (command == "return") {
            returnToFileList();
        } else {
            lastMessage = "Unknown command in text view mode.";
        }

        return;
    }

    std::stringstream ss(command);

    std::string action;
    ss >> action;

    if (action == "ls") {
        refreshEntries();
        lastMessage = "";
    } else if (action == "cd") {
        std::string folderName;
        ss >> folderName;

        if (folderName.empty()) {
            lastMessage = "Usage: cd <folder>";
        } else {
            changeDirectory(folderName);
        }
    } else if (action == "open") {
        std::string fileName;
        ss >> fileName;

        if (fileName.empty()) {
            lastMessage = "Usage: open <file>";
        } else {
            openTextFile(fileName);
        }
    } else if (action == "search") {
        std::string query;
        ss >> query;

        if (query.empty()) {
            lastMessage = "Usage: search <text>";
        } else {
            searchFile(query);
        }
    } else if (action == "copy") {
        std::string sourceName;
        std::string targetName;

        ss >> sourceName >> targetName;

        if (sourceName.empty() || targetName.empty()) {
            lastMessage = "Usage: copy <from> <to>";
        } else {
            copyFile(sourceName, targetName);
        }
    } else if (action == "delete") {
        std::string fileName;
        ss >> fileName;

        if (fileName.empty()) {
            lastMessage = "Usage: delete <file>";
        } else {
            deleteFile(fileName);
        }
    } else if (command == "scroll up") {
        scrollUp();
    } else if (command == "scroll down") {
        scrollDown();
    } else {
        lastMessage = "Unknown command.";
    }
}

void FileManager::changeDirectory(const std::string& folderName) {
    fs::path inputPath(folderName);
    fs::path newPath;

    if (inputPath.is_absolute()) {
        newPath = inputPath;
    } else if (folderName == "..") {
        newPath = currentPath.parent_path();
    } else {
        newPath = currentPath / inputPath;
    }

    if (!fs::exists(newPath)) {
        lastMessage = "Directory does not exist.";
        return;
    }

    if (!fs::is_directory(newPath)) {
        lastMessage = "This is not a directory.";
        return;
    }

    currentPath = fs::canonical(newPath);
    scrollOffset = 0;
    refreshEntries();
    lastMessage = "";
}
void FileManager::openTextFile(const std::string& fileName) {
    fs::path filePath = currentPath / fileName;

    if (!fs::exists(filePath)) {
        lastMessage = "File does not exist.";
        return;
    }

    if (!fs::is_regular_file(filePath)) {
        lastMessage = "This is not a regular file.";
        return;
    }

    std::ifstream file(filePath);

    if (!file.is_open()) {
        lastMessage = "Failed to open file.";
        return;
    }

    textLines.clear();

    std::string line;

    while (std::getline(file, line)) {
        textLines.push_back(line);
    }

    viewedFileName = fileName;
    mode = Mode::TextView;
    scrollOffset = 0;
    lastMessage = "";
}

void FileManager::searchFile(const std::string& query) {
    entries.clear();

    try {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            std::string fileName = entry.path().filename().string();

            if (fileName.find(query) != std::string::npos) {
                entries.push_back(entry);
            }
        }

        scrollOffset = 0;
        lastMessage = "Search completed. Found: " + std::to_string(entries.size());
    } catch (const fs::filesystem_error& error) {
        lastMessage = "Search failed: " + std::string(error.what());
    }
}

void FileManager::copyFile(const std::string& sourceName, const std::string& targetName) {
    fs::path sourcePath = currentPath / sourceName;
    fs::path targetPath = currentPath / targetName;

    if (!fs::exists(sourcePath)) {
        lastMessage = "Failed to copy: source file does not exist.";
        return;
    }

    if (!fs::is_regular_file(sourcePath)) {
        lastMessage = "Failed to copy: source is not a regular file.";
        return;
    }

    lastMessage = "Copying...";
    draw();

    try {
        fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing);
        refreshEntries();
        lastMessage = "Copied.";
    } catch (const fs::filesystem_error& error) {
        lastMessage = "Failed to copy: " + std::string(error.what());
    }
}

void FileManager::deleteFile(const std::string& fileName) {
    fs::path filePath = currentPath / fileName;

    if (!fs::exists(filePath)) {
        lastMessage = "Failed to delete: file does not exist.";
        return;
    }

    if (!fs::is_regular_file(filePath)) {
        lastMessage = "Failed to delete: this is not a regular file.";
        return;
    }

    try {
        fs::remove(filePath);
        refreshEntries();
        lastMessage = "Deleted.";
    } catch (const fs::filesystem_error& error) {
        lastMessage = "Failed to delete: " + std::string(error.what());
    }
}

void FileManager::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
    }

    lastMessage = "";
}

void FileManager::scrollDown() {
    TerminalSize size = getTerminalSize();
    int contentHeight = getContentHeight(size);

    int totalItems;

    if (mode == Mode::FileList) {
        totalItems = entries.size();
    } else {
        totalItems = textLines.size();
    }

    if (scrollOffset + contentHeight < totalItems) {
        scrollOffset++;
    }

    lastMessage = "";
}

void FileManager::returnToFileList() {
    mode = Mode::FileList;
    textLines.clear();
    viewedFileName = "";
    scrollOffset = 0;
    lastMessage = "";
    refreshEntries();
}
