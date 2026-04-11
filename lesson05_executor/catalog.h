#pragma once

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <memory>
#include <optional>
#include <stdexcept>

// ============================================================
// catalog.h — 元数据管理（Catalog）
//
// Catalog 存储数据库中所有表的定义（Schema），
// 以及表中实际存储的数据（本课用内存存储，Lesson 06 改为磁盘）。
// ============================================================

// ---- 值类型 ----
using Value = std::variant<int64_t, std::string>;

// 将 Value 转为字符串（用于打印）
inline std::string ValueToString(const Value& v) {
    if (std::holds_alternative<int64_t>(v))
        return std::to_string(std::get<int64_t>(v));
    return std::get<std::string>(v);
}

// ---- 列定义 ----
struct ColumnDef {
    std::string name;   // 列名
    std::string type;   // "INT" 或 "TEXT"
};

// ---- Schema（表结构）----
struct Schema {
    std::string table_name;
    std::vector<ColumnDef> columns;

    // 根据列名查找列的索引（-1 表示不存在）
    int GetColumnIndex(const std::string& col_name) const {
        for (int i = 0; i < (int)columns.size(); i++) {
            if (columns[i].name == col_name) return i;
        }
        return -1;
    }
};

// ---- Tuple（行/元组）----
// 一行数据，每个元素对应 Schema 中的一列
using Tuple = std::vector<Value>;

// ---- TableData（内存表存储）----
struct TableData {
    Schema schema;
    std::vector<Tuple> rows;  // 所有行

    void InsertRow(const Tuple& row) {
        if (row.size() != schema.columns.size()) {
            throw std::runtime_error("列数不匹配：期望 " +
                std::to_string(schema.columns.size()) + "，实际 " +
                std::to_string(row.size()));
        }
        rows.push_back(row);
    }
};

// ============================================================
// Catalog — 元数据管理器
// ============================================================
class Catalog {
public:
    // 创建表
    void CreateTable(const Schema& schema);

    // 删除表
    void DropTable(const std::string& table_name);

    // 获取表（返回 nullptr 表示不存在）
    TableData* GetTable(const std::string& table_name);
    const TableData* GetTable(const std::string& table_name) const;

    // 表是否存在
    bool TableExists(const std::string& table_name) const;

    // 列出所有表名
    std::vector<std::string> ListTables() const;

private:
    std::unordered_map<std::string, TableData> tables_;
};
