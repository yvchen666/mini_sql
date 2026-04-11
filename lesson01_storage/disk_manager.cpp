#include "disk_manager.h"
#include <iostream>
#include <stdexcept>

// ============================================================
// disk_manager.cpp — 磁盘管理器实现
// ============================================================

DiskManager::DiskManager(const std::string& db_file)
    : db_file_name_(db_file), next_page_id_(0) {

    // 以读写模式打开文件；如果文件不存在则先创建
    // ios::in | ios::out：读写模式
    // ios::binary：二进制模式（避免换行符转换）
    db_file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);

    if (!db_file_.is_open()) {
        // 文件不存在，先用 out 模式创建，再重新以读写模式打开
        db_file_.open(db_file, std::ios::out | std::ios::binary);
        db_file_.close();
        db_file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!db_file_.is_open()) {
        throw std::runtime_error("无法打开数据库文件: " + db_file);
    }

    // 计算文件中已有多少页面（用于恢复 next_page_id_）
    db_file_.seekg(0, std::ios::end);
    std::streamsize file_size = db_file_.tellg();
    if (file_size > 0) {
        next_page_id_ = static_cast<page_id_t>(file_size / PAGE_SIZE);
    }

    std::cout << "[DiskManager] 打开数据库文件: " << db_file
              << "，已有 " << next_page_id_ << " 个页面\n";
}

DiskManager::~DiskManager() {
    if (db_file_.is_open()) {
        db_file_.flush();
        db_file_.close();
    }
}

void DiskManager::WritePage(page_id_t page_id, const char* data) {
    // 计算该页面在文件中的字节偏移
    // 例如：page_id=2，PAGE_SIZE=4096，则 offset=8192
    size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;

    // 移动写指针到目标位置
    db_file_.seekp(offset, std::ios::beg);
    if (db_file_.fail()) {
        throw std::runtime_error("WritePage: seekp 失败，page_id=" + std::to_string(page_id));
    }

    // 写入整个页面（PAGE_SIZE 字节）
    db_file_.write(data, PAGE_SIZE);
    if (db_file_.fail()) {
        throw std::runtime_error("WritePage: write 失败，page_id=" + std::to_string(page_id));
    }

    // 立即刷盘（教学用途；生产环境通常批量刷盘）
    db_file_.flush();
}

void DiskManager::ReadPage(page_id_t page_id, char* data) {
    size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;

    // 检查 page_id 是否在文件范围内
    db_file_.seekg(0, std::ios::end);
    std::streamsize file_size = db_file_.tellg();
    if (static_cast<std::streamsize>(offset + PAGE_SIZE) > file_size) {
        throw std::runtime_error("ReadPage: page_id=" + std::to_string(page_id) + " 超出文件范围");
    }

    // 移动读指针到目标位置
    db_file_.seekg(offset, std::ios::beg);
    if (db_file_.fail()) {
        throw std::runtime_error("ReadPage: seekg 失败，page_id=" + std::to_string(page_id));
    }

    // 读取整个页面
    db_file_.read(data, PAGE_SIZE);
    if (db_file_.fail()) {
        throw std::runtime_error("ReadPage: read 失败，page_id=" + std::to_string(page_id));
    }
}

page_id_t DiskManager::AllocatePage() {
    // 简单地返回当前计数器，然后递增
    // 注意：这里没有处理页面回收（DELETE 后的空洞）
    // 真实数据库会维护一个"空闲页面列表"
    return next_page_id_++;
}
