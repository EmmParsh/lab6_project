#pragma once

#include <filesystem>
#include <string>

class FileManager {
private:
    std::filesystem::path currentPath;

public:
    FileManager();

    void run();

private:
    void printMenu() const;

    // Функции участника 1
    void listFiles() const;
    void changeDirectory();

    // Функции участника 2
    void viewTextFile() const;
    void searchFile() const;
    void deleteFile();
    void copyFile() const;
};
