#include "executor.h"
#include <iostream>
#include <stdexcept>
#include <iomanip>

// ============================================================
// executor.cpp — 查询执行器实现
// ============================================================

// ---- SeqScanExecutor ----

SeqScanExecutor::SeqScanExecutor(const TableData* table)
    : table_(table), cursor_(0) {}

void SeqScanExecutor::Init() {
    cursor_ = 0;  // 重置扫描位置
}

std::optional<Tuple> SeqScanExecutor::Next() {
    if (!table_ || cursor_ >= table_->rows.size()) {
        return std::nullopt;  // 扫描完毕
    }
    return table_->rows[cursor_++];
}

const Schema& SeqScanExecutor::GetOutputSchema() const {
    return table_->schema;
}

// ---- FilterExecutor ----

FilterExecutor::FilterExecutor(std::unique_ptr<ExecutorBase> child, Predicate predicate)
    : child_(std::move(child)), predicate_(std::move(predicate)) {}

void FilterExecutor::Init() {
    child_->Init();
}

std::optional<Tuple> FilterExecutor::Next() {
    // 不断从子算子拉取，直到找到满足条件的行
    while (true) {
        auto row = child_->Next();
        if (!row.has_value()) return std::nullopt;  // 子算子已无数据
        if (predicate_(*row, child_->GetOutputSchema())) {
            return row;  // 满足过滤条件，返回
        }
        // 不满足条件，继续拉取下一行
    }
}

const Schema& FilterExecutor::GetOutputSchema() const {
    return child_->GetOutputSchema();
}

// ---- ProjectionExecutor ----

ProjectionExecutor::ProjectionExecutor(std::unique_ptr<ExecutorBase> child,
                                       const std::vector<std::string>& output_cols)
    : child_(std::move(child)) {
    const Schema& src = child_->GetOutputSchema();
    output_schema_.table_name = src.table_name;

    if (output_cols.empty() || (output_cols.size() == 1 && output_cols[0] == "*")) {
        // 全部列
        for (int i = 0; i < (int)src.columns.size(); i++) {
            col_indices_.push_back(i);
            output_schema_.columns.push_back(src.columns[i]);
        }
    } else {
        for (const auto& col_name : output_cols) {
            int idx = src.GetColumnIndex(col_name);
            if (idx == -1) {
                throw std::runtime_error("列 \"" + col_name + "\" 不存在于表 \"" +
                                         src.table_name + "\"");
            }
            col_indices_.push_back(idx);
            output_schema_.columns.push_back(src.columns[idx]);
        }
    }
}

void ProjectionExecutor::Init() {
    child_->Init();
}

std::optional<Tuple> ProjectionExecutor::Next() {
    auto row = child_->Next();
    if (!row.has_value()) return std::nullopt;

    // 投影：只取指定列
    Tuple projected;
    for (int idx : col_indices_) {
        projected.push_back((*row)[idx]);
    }
    return projected;
}

const Schema& ProjectionExecutor::GetOutputSchema() const {
    return output_schema_;
}

// ---- InsertExecutor ----

InsertExecutor::InsertExecutor(TableData* table, Tuple row)
    : table_(table), row_(std::move(row)), executed_(false) {}

void InsertExecutor::Init() {
    executed_ = false;
}

std::optional<Tuple> InsertExecutor::Next() {
    if (executed_) return std::nullopt;
    executed_ = true;
    table_->InsertRow(row_);
    // 返回插入的行（可以用于 RETURNING 子句，这里仅作标记）
    return row_;
}

const Schema& InsertExecutor::GetOutputSchema() const {
    return dummy_schema_;
}

// ---- ExecutionEngine ----

ExecutionEngine::ExecutionEngine(Catalog* catalog) : catalog_(catalog) {}

Predicate ExecutionEngine::BuildPredicate(const SimpleExpr& expr) {
    if (expr.type == SimpleExpr::Type::ALWAYS_TRUE) {
        return [](const Tuple&, const Schema&) { return true; };
    }

    std::string col = expr.col;
    std::string op = expr.op;
    Value val = expr.val;

    return [col, op, val](const Tuple& row, const Schema& schema) -> bool {
        int idx = schema.GetColumnIndex(col);
        if (idx == -1) {
            throw std::runtime_error("列 \"" + col + "\" 不存在");
        }
        const Value& row_val = row[idx];

        // 比较（只支持同类型比较）
        if (op == "=") {
            return row_val == val;
        } else if (op == "!=") {
            return row_val != val;
        } else if (op == "<") {
            if (std::holds_alternative<int64_t>(row_val) &&
                std::holds_alternative<int64_t>(val)) {
                return std::get<int64_t>(row_val) < std::get<int64_t>(val);
            }
        } else if (op == ">") {
            if (std::holds_alternative<int64_t>(row_val) &&
                std::holds_alternative<int64_t>(val)) {
                return std::get<int64_t>(row_val) > std::get<int64_t>(val);
            }
        } else if (op == "<=") {
            if (std::holds_alternative<int64_t>(row_val) &&
                std::holds_alternative<int64_t>(val)) {
                return std::get<int64_t>(row_val) <= std::get<int64_t>(val);
            }
        } else if (op == ">=") {
            if (std::holds_alternative<int64_t>(row_val) &&
                std::holds_alternative<int64_t>(val)) {
                return std::get<int64_t>(row_val) >= std::get<int64_t>(val);
            }
        }
        return false;
    };
}

std::vector<Tuple> ExecutionEngine::ExecuteSelect(const SimpleSelect& sel) {
    TableData* table = catalog_->GetTable(sel.table);
    if (!table) {
        throw std::runtime_error("表 \"" + sel.table + "\" 不存在");
    }

    // 构建算子树：[Projection] → [Filter（可选）] → [SeqScan]
    std::unique_ptr<ExecutorBase> exec = std::make_unique<SeqScanExecutor>(table);

    // 如果有 WHERE 子句，添加 Filter 算子
    if (sel.where.has_value()) {
        Predicate pred = BuildPredicate(*sel.where);
        exec = std::make_unique<FilterExecutor>(std::move(exec), pred);
    }

    // 添加 Projection 算子
    exec = std::make_unique<ProjectionExecutor>(std::move(exec), sel.columns);

    // 执行：逐行拉取结果
    exec->Init();
    std::vector<Tuple> result;
    while (true) {
        auto row = exec->Next();
        if (!row.has_value()) break;
        result.push_back(*row);
    }

    return result;
}

void ExecutionEngine::ExecuteInsert(const std::string& table_name, const Tuple& row) {
    TableData* table = catalog_->GetTable(table_name);
    if (!table) {
        throw std::runtime_error("表 \"" + table_name + "\" 不存在");
    }

    InsertExecutor inserter(table, row);
    inserter.Init();
    inserter.Next();  // 执行插入
}

void ExecutionEngine::ExecuteCreate(const Schema& schema) {
    catalog_->CreateTable(schema);
}

void ExecutionEngine::ExecuteDrop(const std::string& table_name) {
    catalog_->DropTable(table_name);
}

void ExecutionEngine::PrintResult(const std::vector<Tuple>& rows, const Schema& schema) {
    if (rows.empty()) {
        std::cout << "(0 行)\n";
        return;
    }

    // 打印列头
    for (size_t i = 0; i < schema.columns.size(); i++) {
        std::cout << std::left << std::setw(15) << schema.columns[i].name;
    }
    std::cout << "\n";
    std::cout << std::string(schema.columns.size() * 15, '-') << "\n";

    // 打印数据行
    for (const auto& row : rows) {
        for (const auto& val : row) {
            std::cout << std::left << std::setw(15) << ValueToString(val);
        }
        std::cout << "\n";
    }
    std::cout << "(" << rows.size() << " 行)\n";
}
