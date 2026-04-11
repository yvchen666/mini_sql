#include "catalog.h"

// ============================================================
// catalog.cpp — Catalog 实现
// ============================================================

void Catalog::CreateTable(const Schema& schema) {
    if (TableExists(schema.table_name)) {
        throw std::runtime_error("表 \"" + schema.table_name + "\" 已存在");
    }
    TableData td;
    td.schema = schema;
    tables_[schema.table_name] = std::move(td);
}

void Catalog::DropTable(const std::string& table_name) {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        throw std::runtime_error("表 \"" + table_name + "\" 不存在");
    }
    tables_.erase(it);
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
    for (const auto& kv : tables_) {
        names.push_back(kv.first);
    }
    return names;
}
