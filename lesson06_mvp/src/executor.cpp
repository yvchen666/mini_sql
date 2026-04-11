#include "executor.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>

// ============================================================
// executor.cpp — 执行器实现（lesson06 整合版）
// ============================================================

// ---- SeqScanExecutor ----
SeqScanExecutor::SeqScanExecutor(const TableData* table) : table_(table), cursor_(0) {}
void SeqScanExecutor::Init() { cursor_ = 0; }
std::optional<Tuple> SeqScanExecutor::Next() {
    if (!table_ || cursor_ >= table_->rows.size()) return std::nullopt;
    return table_->rows[cursor_++];
}
const Schema& SeqScanExecutor::GetOutputSchema() const { return table_->schema; }

// ---- FilterExecutor ----
FilterExecutor::FilterExecutor(std::unique_ptr<ExecutorBase> child, Predicate pred)
    : child_(std::move(child)), predicate_(std::move(pred)) {}
void FilterExecutor::Init() { child_->Init(); }
std::optional<Tuple> FilterExecutor::Next() {
    while (true) {
        auto row = child_->Next();
        if (!row.has_value()) return std::nullopt;
        if (predicate_(*row, child_->GetOutputSchema())) return row;
    }
}
const Schema& FilterExecutor::GetOutputSchema() const { return child_->GetOutputSchema(); }

// ---- ProjectionExecutor ----
ProjectionExecutor::ProjectionExecutor(std::unique_ptr<ExecutorBase> child,
                                       const std::vector<std::string>& output_cols)
    : child_(std::move(child)) {
    const Schema& src = child_->GetOutputSchema();
    output_schema_.table_name = src.table_name;
    if (output_cols.empty() || (output_cols.size() == 1 && output_cols[0] == "*")) {
        for (int i = 0; i < (int)src.columns.size(); i++) {
            col_indices_.push_back(i);
            output_schema_.columns.push_back(src.columns[i]);
        }
    } else {
        for (const auto& col_name : output_cols) {
            int idx = src.GetColumnIndex(col_name);
            if (idx == -1)
                throw std::runtime_error("列 \"" + col_name + "\" 不存在");
            col_indices_.push_back(idx);
            output_schema_.columns.push_back(src.columns[idx]);
        }
    }
}
void ProjectionExecutor::Init() { child_->Init(); }
std::optional<Tuple> ProjectionExecutor::Next() {
    auto row = child_->Next();
    if (!row.has_value()) return std::nullopt;
    Tuple projected;
    for (int idx : col_indices_) projected.push_back((*row)[idx]);
    return projected;
}
const Schema& ProjectionExecutor::GetOutputSchema() const { return output_schema_; }

// ---- ExecutionEngine ----
ExecutionEngine::ExecutionEngine(Catalog* catalog) : catalog_(catalog) {}

Value ExecutionEngine::EvalExpr(const std::shared_ptr<Expr>& expr,
                                 const Tuple& row, const Schema& schema) {
    switch (expr->type) {
        case ExprType::INT_LITERAL: return expr->int_val;
        case ExprType::STR_LITERAL: return expr->str_val;
        case ExprType::COLUMN_REF: {
            int idx = schema.GetColumnIndex(expr->str_val);
            if (idx == -1) throw std::runtime_error("列 \"" + expr->str_val + "\" 不存在");
            return row[idx];
        }
        default:
            throw std::runtime_error("不支持对 BINARY_OP 直接求值");
    }
}

Predicate ExecutionEngine::BuildPredicate(const std::shared_ptr<Expr>& expr) {
    if (!expr) {
        return [](const Tuple&, const Schema&) { return true; };
    }
    if (expr->type != ExprType::BINARY_OP) {
        return [](const Tuple&, const Schema&) { return true; };
    }

    // 处理 AND/OR
    if (expr->op == "AND") {
        auto left_pred  = BuildPredicate(expr->left);
        auto right_pred = BuildPredicate(expr->right);
        return [left_pred, right_pred](const Tuple& t, const Schema& s) {
            return left_pred(t, s) && right_pred(t, s);
        };
    }
    if (expr->op == "OR") {
        auto left_pred  = BuildPredicate(expr->left);
        auto right_pred = BuildPredicate(expr->right);
        return [left_pred, right_pred](const Tuple& t, const Schema& s) {
            return left_pred(t, s) || right_pred(t, s);
        };
    }

    // 比较运算
    auto left_expr  = expr->left;
    auto right_expr = expr->right;
    std::string op  = expr->op;

    return [this, left_expr, right_expr, op](const Tuple& row, const Schema& schema) -> bool {
        Value lv = EvalExpr(left_expr, row, schema);
        Value rv = EvalExpr(right_expr, row, schema);
        if (op == "=")  return lv == rv;
        if (op == "!=") return lv != rv;
        // 数值比较
        if (std::holds_alternative<int64_t>(lv) && std::holds_alternative<int64_t>(rv)) {
            int64_t l = std::get<int64_t>(lv), r = std::get<int64_t>(rv);
            if (op == "<")  return l < r;
            if (op == ">")  return l > r;
            if (op == "<=") return l <= r;
            if (op == ">=") return l >= r;
        }
        // 字符串比较
        if (std::holds_alternative<std::string>(lv) && std::holds_alternative<std::string>(rv)) {
            const std::string& l = std::get<std::string>(lv);
            const std::string& r = std::get<std::string>(rv);
            if (op == "<")  return l < r;
            if (op == ">")  return l > r;
            if (op == "<=") return l <= r;
            if (op == ">=") return l >= r;
        }
        return false;
    };
}

std::string ExecutionEngine::Execute(const std::shared_ptr<Stmt>& stmt) {
    switch (stmt->type) {
        case StmtType::CREATE_TABLE: {
            Schema schema;
            schema.table_name = stmt->create_table_name;
            for (auto& col : stmt->create_columns) {
                schema.columns.push_back({col.name, col.type});
            }
            catalog_->CreateTable(schema);
            catalog_->SaveAll();
            return "表 \"" + stmt->create_table_name + "\" 创建成功";
        }
        case StmtType::DROP_TABLE: {
            catalog_->DropTable(stmt->drop_table_name);
            catalog_->SaveAll();
            return "表 \"" + stmt->drop_table_name + "\" 已删除";
        }
        case StmtType::INSERT: {
            TableData* table = catalog_->GetTable(stmt->insert_table);
            if (!table) throw std::runtime_error("表 \"" + stmt->insert_table + "\" 不存在");
            Tuple row;
            for (auto& v : stmt->insert_values) {
                if (std::holds_alternative<int64_t>(v)) row.push_back(std::get<int64_t>(v));
                else row.push_back(std::get<std::string>(v));
            }
            table->InsertRow(row);
            catalog_->SaveAll();
            return "插入成功 (1 行)";
        }
        case StmtType::SELECT: {
            TableData* table = catalog_->GetTable(stmt->select_table);
            if (!table) throw std::runtime_error("表 \"" + stmt->select_table + "\" 不存在");

            // 构建算子树
            std::unique_ptr<ExecutorBase> exec = std::make_unique<SeqScanExecutor>(table);
            if (stmt->where_clause) {
                Predicate pred = BuildPredicate(stmt->where_clause);
                exec = std::make_unique<FilterExecutor>(std::move(exec), pred);
            }
            exec = std::make_unique<ProjectionExecutor>(std::move(exec), stmt->select_columns);

            // 执行
            exec->Init();
            std::vector<Tuple> result;
            while (true) {
                auto row = exec->Next();
                if (!row.has_value()) break;
                result.push_back(*row);
            }

            // 打印结果
            PrintResult(result, exec->GetOutputSchema());
            return "(" + std::to_string(result.size()) + " 行)";
        }
    }
    return "未知语句";
}

void ExecutionEngine::PrintResult(const std::vector<Tuple>& rows, const Schema& schema) {
    if (rows.empty()) { std::cout << "(0 行)\n"; return; }

    // 计算每列最大宽度
    std::vector<size_t> widths;
    for (auto& col : schema.columns) widths.push_back(col.name.size());
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            widths[i] = std::max(widths[i], ValueToString(row[i]).size());
        }
    }

    // 打印列头
    for (size_t i = 0; i < schema.columns.size(); i++) {
        std::cout << std::left << std::setw(widths[i] + 2) << schema.columns[i].name;
    }
    std::cout << "\n";
    for (size_t w : widths) std::cout << std::string(w + 2, '-');
    std::cout << "\n";

    // 打印数据行
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            std::cout << std::left << std::setw(widths[i] + 2) << ValueToString(row[i]);
        }
        std::cout << "\n";
    }
}
