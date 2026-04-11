#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>

// ---- Schema ----
struct Schema {
    std::string table_name;
    std::vector<ColumnDef> columns;
    int GetColumnIndex(const std::string& col_name) const {
        for (int i = 0; i < (int)columns.size(); i++)
            if (columns[i].name == col_name) return i;
        return -1;
    }
};

// ---- Tuple ----
using Tuple = std::vector<Value>;

inline std::string ValueToString(const Value& v) {
    if (std::holds_alternative<int64_t>(v)) return std::to_string(std::get<int64_t>(v));
    return std::get<std::string>(v);
}

// ---- TableData（内存 + 磁盘持久化）----
struct TableData {
    Schema schema;
    std::vector<Tuple> rows;
    void InsertRow(const Tuple& row);
};

// ---- Catalog ----
class Catalog {
public:
    explicit Catalog(const std::string& data_dir);
    void CreateTable(const Schema& schema);
    void DropTable(const std::string& table_name);
    TableData* GetTable(const std::string& table_name);
    const TableData* GetTable(const std::string& table_name) const;
    bool TableExists(const std::string& table_name) const;
    std::vector<std::string> ListTables() const;
    // 持久化：将所有表数据写到磁盘
    void SaveAll();
    // 从磁盘加载所有表数据
    void LoadAll();
private:
    std::string data_dir_;
    std::unordered_map<std::string, TableData> tables_;
    void SaveTable(const std::string& table_name);
    void LoadTable(const std::string& table_name, const Schema& schema);
    void SaveSchemas();
    void LoadSchemas();
};

// ---- 执行器 ----
using Predicate = std::function<bool(const Tuple&, const Schema&)>;

class ExecutorBase {
public:
    virtual ~ExecutorBase() = default;
    virtual void Init() = 0;
    virtual std::optional<Tuple> Next() = 0;
    virtual const Schema& GetOutputSchema() const = 0;
};

class SeqScanExecutor : public ExecutorBase {
public:
    explicit SeqScanExecutor(const TableData* table);
    void Init() override;
    std::optional<Tuple> Next() override;
    const Schema& GetOutputSchema() const override;
private:
    const TableData* table_;
    size_t cursor_;
};

class FilterExecutor : public ExecutorBase {
public:
    FilterExecutor(std::unique_ptr<ExecutorBase> child, Predicate predicate);
    void Init() override;
    std::optional<Tuple> Next() override;
    const Schema& GetOutputSchema() const override;
private:
    std::unique_ptr<ExecutorBase> child_;
    Predicate predicate_;
};

class ProjectionExecutor : public ExecutorBase {
public:
    ProjectionExecutor(std::unique_ptr<ExecutorBase> child,
                       const std::vector<std::string>& output_cols);
    void Init() override;
    std::optional<Tuple> Next() override;
    const Schema& GetOutputSchema() const override;
private:
    std::unique_ptr<ExecutorBase> child_;
    std::vector<int> col_indices_;
    Schema output_schema_;
};

// ---- ExecutionEngine ----
class ExecutionEngine {
public:
    explicit ExecutionEngine(Catalog* catalog);
    // 执行解析后的 AST 语句，返回结果描述字符串
    std::string Execute(const std::shared_ptr<Stmt>& stmt);
    static void PrintResult(const std::vector<Tuple>& rows, const Schema& schema);
private:
    Catalog* catalog_;
    Predicate BuildPredicate(const std::shared_ptr<Expr>& expr);
    Value EvalExpr(const std::shared_ptr<Expr>& expr, const Tuple& row, const Schema& schema);
};
