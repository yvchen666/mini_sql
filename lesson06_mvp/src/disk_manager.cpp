#include "disk_manager.h"
#include <iostream>
#include <stdexcept>

DiskManager::DiskManager(const std::string& db_file)
    : db_file_name_(db_file), next_page_id_(0) {
    db_file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_file_.is_open()) {
        db_file_.open(db_file, std::ios::out | std::ios::binary);
        db_file_.close();
        db_file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!db_file_.is_open()) {
        throw std::runtime_error("无法打开数据库文件: " + db_file);
    }
    db_file_.seekg(0, std::ios::end);
    std::streamsize file_size = db_file_.tellg();
    if (file_size > 0) {
        next_page_id_ = static_cast<page_id_t>(file_size / PAGE_SIZE);
    }
}

DiskManager::~DiskManager() {
    if (db_file_.is_open()) { db_file_.flush(); db_file_.close(); }
}

void DiskManager::WritePage(page_id_t page_id, const char* data) {
    size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;
    db_file_.seekp(offset, std::ios::beg);
    db_file_.write(data, PAGE_SIZE);
    db_file_.flush();
}

void DiskManager::ReadPage(page_id_t page_id, char* data) {
    size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;
    db_file_.seekg(0, std::ios::end);
    std::streamsize file_size = db_file_.tellg();
    if (static_cast<std::streamsize>(offset + PAGE_SIZE) > file_size) {
        memset(data, 0, PAGE_SIZE);
        return;
    }
    db_file_.seekg(offset, std::ios::beg);
    db_file_.read(data, PAGE_SIZE);
    if (db_file_.fail()) { db_file_.clear(); memset(data, 0, PAGE_SIZE); }
}

page_id_t DiskManager::AllocatePage() {
    return next_page_id_++;
}
