#pragma once

#include <filesystem>
#include <string>
#include <vector>

class FileManager {
private:
    enum class Mode {
        FileList,
        TextView
    };

    struct TerminalSize {
        int rows;
        int cols;
    };

    std::filesystem::path currentPath;
    Mode mode;

    std::vector<std::filesystem::directory_entry> entries;
    std::vector<std::string> textLines;

    std::string viewedFileName;
    std::string lastMessage;

    int scrollOffset;

public:
    FileManager();

    void run();

private:
    TerminalSize getTerminalSize() const;

    void refreshEntries();
    void draw();
    void clearScreen() const;

    void drawHeader(const TerminalSize& size) const;
    void drawFileTable(const TerminalSize& size) const;
    void drawTextView(const TerminalSize& size) const;
    void drawMessageLine(const TerminalSize& size) const;
    void drawMenu(const TerminalSize& size) const;
    void drawPrompt() const;

    std::string cutText(const std::string& text, int maxWidth) const;

    void handleCommand(const std::string& command);

    void changeDirectory(const std::string& folderName);
    void openTextFile(const std::string& fileName);
    void searchFile(const std::string& query);
    void copyFile(const std::string& sourceName, const std::string& targetName);
    void deleteFile(const std::string& fileName);

    void scrollUp();
    void scrollDown();
    void returnToFileList();

    int getContentHeight(const TerminalSize& size) const;
};
