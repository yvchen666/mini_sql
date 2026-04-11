#pragma once
#include "page.h"
#include <string>
#include <fstream>

class DiskManager {
public:
    explicit DiskManager(const std::string& db_file);
    ~DiskManager();
    void WritePage(page_id_t page_id, const char* data);
    void ReadPage(page_id_t page_id, char* data);
    page_id_t AllocatePage();
    page_id_t GetPageCount() const { return next_page_id_; }
    const std::string& GetFileName() const { return db_file_name_; }
private:
    std::string  db_file_name_;
    std::fstream db_file_;
    page_id_t    next_page_id_;
};
