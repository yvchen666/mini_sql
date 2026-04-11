# Lesson 05 — 查询执行器

## 学习目标

1. 理解火山模型（Volcano Model）的执行流程
2. 实现 Catalog：管理表的元数据（Schema）
3. 实现基本算子：SeqScan（全表扫描）、Insert、Filter
4. 将 AST 翻译为可执行的算子树

---

## 核心概念

### 火山模型（Volcano Model）

火山模型是数据库查询执行的经典模型，每个算子实现同一个接口：

```cpp
class Executor {
    virtual void Init() = 0;         // 初始化
    virtual Tuple* Next() = 0;       // 返回下一条结果（nullptr 表示结束）
    virtual void Close() = 0;        // 清理
};
```

算子之间通过"拉取"（Pull）方式串联：

```
FilterExecutor.Next()
  │ 从子节点拉取
  └─→ SeqScanExecutor.Next()
        │ 从表存储拉取
        └─→ Table[0], Table[1], Table[2], ...
```

### 执行流程示例

```
SQL: SELECT * FROM users WHERE age > 25

执行树：
  Filter(age > 25)     ← 上层调用 Next()，Filter 内部过滤不满足的行
      │
  SeqScan(users)       ← 逐行扫描 users 表

执行过程（由上而下驱动）：
  1. Filter.Next() 被调用
  2. Filter 调用 SeqScan.Next() 获取第一行 (1, Alice, 30)
  3. Filter 检查 30 > 25 → 满足 → 返回给上层
  4. Filter.Next() 被再次调用
  5. Filter 调用 SeqScan.Next() 获取第二行 (2, Bob, 20)
  6. Filter 检查 20 > 25 → 不满足 → 继续拉取
  7. Filter 调用 SeqScan.Next() 获取第三行 (3, Charlie, 28)
  8. Filter 检查 28 > 25 → 满足 → 返回给上层
  ...
```

### Catalog（元数据管理）

```
Catalog 存储所有表的定义（Schema）：

Catalog
├── "users" → Schema{id:INT, name:TEXT, age:INT}
├── "orders" → Schema{id:INT, user_id:INT, amount:INT}
└── ...
```

### Tuple（行/元组）

```
一行数据用 Tuple 表示：
  Tuple = vector<Value>（每列对应一个 Value）
  Value = variant<int64_t, string>

例如：(1, "Alice", 30) → [Int(1), Str("Alice"), Int(30)]
```

---

## 思考题

1. 火山模型的优点是什么？批量模型（Vectorized）又有什么优势？
2. 如果 WHERE 子句中有多个条件（AND/OR），执行器如何处理？
3. 如何添加 ORDER BY 和 LIMIT 支持？

---

## 编译与运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
make lesson05
./lesson05_executor/lesson05
```
