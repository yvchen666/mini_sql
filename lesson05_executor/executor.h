#pragma once

#include "catalog.h"
#include <memory>
#include <functional>
#include <optional>

// ============================================================
// executor.h — 查询执行器（火山模型）
//
// 火山模型（Volcano Model）：
//   每个算子实现 Init/Next/Close 接口
//   上层算子通过调用下层算子的 Next() 来"拉取"数据
//   Next() 返回 nullptr 表示数据已全部产出
// ============================================================

// ---- 表达式求值 ----
// 对一个 Tuple 求值一个表达式（来自 AST）
// 这里用简化版：直接用 lambda 表示条件

// 谓词函数：接受一行数据，返回是否满足条件
using Predicate = std::function<bool(const Tuple&, const Schema&)>;

// ============================================================
// ExecutorBase — 算子基类（火山模型接口）
// ============================================================
class ExecutorBase {
public:
    virtual ~ExecutorBase() = default;

    // 初始化算子（重置状态，准备开始产出数据）
    virtual void Init() = 0;

    // 返回下一条结果元组
    // 返回 std::nullopt 表示没有更多数据
    virtual std::optional<Tuple> Next() = 0;

    // 获取输出的 Schema
    virtual const Schema& GetOutputSchema() const = 0;
};

// ============================================================
// SeqScanExecutor — 全表扫描算子
// 逐行扫描表中的所有数据
// ============================================================
class SeqScanExecutor : public ExecutorBase {
public:
    // table：要扫描的表
    explicit SeqScanExecutor(const TableData* table);

    void Init() override;
    std::optional<Tuple> Next() override;
    const Schema& GetOutputSchema() const override;

private:
    const TableData* table_;
    size_t cursor_;  // 当前扫描位置
};

// ============================================================
// FilterExecutor — 过滤算子
// 从子算子拉取数据，只返回满足谓词的行
// ============================================================
class FilterExecutor : public ExecutorBase {
public:
    // child：子算子（数据来源）
    // predicate：过滤条件
    FilterExecutor(std::unique_ptr<ExecutorBase> child, Predicate predicate);

    void Init() override;
    std::optional<Tuple> Next() override;
    const Schema& GetOutputSchema() const override;

private:
    std::unique_ptr<ExecutorBase> child_;
    Predicate predicate_;
};

// ============================================================
// ProjectionExecutor — 投影算子
// 从子算子拉取数据，只返回指定的列
// ============================================================
class ProjectionExecutor : public ExecutorBase {
public:
    // child：子算子
    // output_cols：要输出的列名列表（空表示全部）
    ProjectionExecutor(std::unique_ptr<ExecutorBase> child,
                       const std::vector<std::string>& output_cols);

    void Init() override;
    std::optional<Tuple> Next() override;
    const Schema& GetOutputSchema() const override;

private:
    std::unique_ptr<ExecutorBase> child_;
    std::vector<int> col_indices_;  // 要输出的列在原 Schema 中的索引
    Schema output_schema_;
};

// ============================================================
// InsertExecutor — 插入算子
// 将一行数据插入到表中
// ============================================================
class InsertExecutor : public ExecutorBase {
public:
    InsertExecutor(TableData* table, Tuple row);

    void Init() override;
    std::optional<Tuple> Next() override;  // 返回插入的行数（1行）
    const Schema& GetOutputSchema() const override;

private:
    TableData* table_;
    Tuple row_;
    bool executed_;
    Schema dummy_schema_;
};

// ============================================================
// ExecutionEngine — 执行引擎
// 接收 Catalog 和 SQL 语句，构建并执行算子树
// ============================================================

// 简化版 AST（避免依赖 Lesson 04 的头文件）
struct SimpleExpr {
    enum class Type { ALWAYS_TRUE, COMPARE } type;
    std::string col;
    std::string op;
    Value val;
};

struct SimpleSelect {
    std::string table;
    std::vector<std::string> columns;  // 空或 {"*"} 表示全部
    std::optional<SimpleExpr> where;
};

class ExecutionEngine {
public:
    explicit ExecutionEngine(Catalog* catalog);

    // 执行 SELECT，返回结果集
    std::vector<Tuple> ExecuteSelect(const SimpleSelect& sel);

    // 执行 INSERT
    void ExecuteInsert(const std::string& table, const Tuple& row);

    // 执行 CREATE TABLE
    void ExecuteCreate(const Schema& schema);

    // 执行 DROP TABLE
    void ExecuteDrop(const std::string& table);

    // 打印结果集
    static void PrintResult(const std::vector<Tuple>& rows, const Schema& schema);

private:
    Catalog* catalog_;

    // 将 SimpleExpr 转换为 Predicate
    Predicate BuildPredicate(const SimpleExpr& expr);
};
