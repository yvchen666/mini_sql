#include "executor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <cstdio>
#include <stdexcept>

// ============================================================
// catalog.cpp — 带持久化的 Catalog 实现
// ============================================================

void TableData::InsertRow(const Tuple& row) {
    if (row.size() != schema.columns.size()) {
        throw std::runtime_error("列数不匹配");
    }
    rows.push_back(row);
}

Catalog::Catalog(const std::string& data_dir) : data_dir_(data_dir) {
    // 确保数据目录存在（POSIX mkdir，权限 0755）
    mkdir(data_dir_.c_str(), 0755);
    LoadSchemas();
}

void Catalog::CreateTable(const Schema& schema) {
    if (TableExists(schema.table_name)) {
        throw std::runtime_error("表 \"" + schema.table_name + "\" 已存在");
    }
    TableData td;
    td.schema = schema;
    tables_[schema.table_name] = std::move(td);
    SaveSchemas();
}

void Catalog::DropTable(const std::string& table_name) {
    if (!TableExists(table_name)) {
        throw std::runtime_error("表 \"" + table_name + "\" 不存在");
    }
    tables_.erase(table_name);
    // 删除数据文件
    std::string data_file = data_dir_ + "/" + table_name + ".dat";
    std::remove(data_file.c_str());
    SaveSchemas();
}

TableData* Catalog::GetTable(const std::string& table_name) {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) return nullptr;
    return &it->second;
}

const TableData* Catalog::GetTable(const std::string& table_name) const {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) return nullptr;
    return &it->second;
}

bool Catalog::TableExists(const std::string& table_name) const {
    return tables_.count(table_name) > 0;
}

std::vector<std::string> Catalog::ListTables() const {
    std::vector<std::string> names;
    for (const auto& kv : tables_) names.push_back(kv.first);
    return names;
}

// ---- 持久化：Schema 存储为文本格式 ----
// 格式：
//   TABLE table_name
//   col1 TYPE
//   col2 TYPE
//   END
//   TABLE ...

void Catalog::SaveSchemas() {
    std::string schema_file = data_dir_ + "/schemas.txt";
    std::ofstream ofs(schema_file);
    if (!ofs) throw std::runtime_error("无法写入 schema 文件");

    for (auto& [name, td] : tables_) {
        ofs << "TABLE " << name << "\n";
        for (auto& col : td.schema.columns) {
            ofs << col.name << " " << col.type << "\n";
        }
        ofs << "END\n";
    }
}

void Catalog::LoadSchemas() {
    std::string schema_file = data_dir_ + "/schemas.txt";
    std::ifstream ifs(schema_file);
    if (!ifs) return;  // 文件不存在，说明是全新数据库

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.substr(0, 6) == "TABLE ") {
            Schema schema;
            schema.table_name = line.substr(6);
            while (std::getline(ifs, line) && line != "END") {
                std::istringstream ss(line);
                ColumnDef col;
                ss >> col.name >> col.type;
                schema.columns.push_back(col);
            }
            TableData td;
            td.schema = schema;
            tables_[schema.table_name] = std::move(td);
            // 加载该表的数据
            LoadTable(schema.table_name, tables_[schema.table_name].schema);
        }
    }
}

// ---- 持久化：表数据存储为二进制格式 ----
// 格式（每行）：
//   [int32: 列数] [对每列: int8 类型(0=INT,1=TEXT) + 数据]
//   INT: int64_t (8字节)
//   TEXT: int32_t 长度 + 字符串内容

void Catalog::SaveTable(const std::string& table_name) {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) return;

    std::string data_file = data_dir_ + "/" + table_name + ".dat";
    std::ofstream ofs(data_file, std::ios::binary | std::ios::trunc);
    if (!ofs) throw std::runtime_error("无法写入数据文件: " + data_file);

    const TableData& td = it->second;
    int32_t row_count = static_cast<int32_t>(td.rows.size());
    ofs.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));

    for (const auto& row : td.rows) {
        for (const auto& val : row) {
            if (std::holds_alternative<int64_t>(val)) {
                int8_t type = 0;
                int64_t v = std::get<int64_t>(val);
                ofs.write(reinterpret_cast<const char*>(&type), 1);
                ofs.write(reinterpret_cast<const char*>(&v), sizeof(v));
            } else {
                int8_t type = 1;
                const std::string& s = std::get<std::string>(val);
                int32_t len = static_cast<int32_t>(s.size());
                ofs.write(reinterpret_cast<const char*>(&type), 1);
                ofs.write(reinterpret_cast<const char*>(&len), sizeof(len));
                ofs.write(s.data(), len);
            }
        }
    }
}

void Catalog::LoadTable(const std::string& table_name, const Schema& schema) {
    std::string data_file = data_dir_ + "/" + table_name + ".dat";
    std::ifstream ifs(data_file, std::ios::binary);
    if (!ifs) return;  // 数据文件不存在（空表）

    int32_t row_count = 0;
    ifs.read(reinterpret_cast<char*>(&row_count), sizeof(row_count));

    auto& td = tables_[table_name];
    for (int32_t r = 0; r < row_count; r++) {
        Tuple row;
        for (const auto& col : schema.columns) {
            int8_t type = 0;
            ifs.read(reinterpret_cast<char*>(&type), 1);
            if (type == 0) {
                int64_t v = 0;
                ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
                row.push_back(v);
            } else {
                int32_t len = 0;
                ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::string s(len, '\0');
                ifs.read(s.data(), len);
                row.push_back(s);
            }
            (void)col;
        }
        td.rows.push_back(row);
    }
}

void Catalog::SaveAll() {
    SaveSchemas();
    for (auto& kv : tables_) {
        SaveTable(kv.first);
    }
}

void Catalog::LoadAll() {
    LoadSchemas();
}
