# Lesson 06 — 完整 MVP：把所有组件整合成一个数据库

> **本课目标**：将前 5 课的组件整合成一个支持持久化的交互式 SQL 数据库。
> 你将看到 Page、DiskManager、BufferPool、B+Tree、Lexer、Parser、Executor 如何协同工作。

---

## 第一步：回顾——我们有了什么？

```
Lesson 01: Page + DiskManager        → 磁盘上的页面读写
Lesson 02: LRU + BufferPoolManager   → 内存中的页面缓存
Lesson 03: B+ 树                      → 高效索引查找
Lesson 04: Lexer + Parser + AST      → SQL 字符串 → 结构化表示
Lesson 05: Catalog + Executor        → 查询执行

本课：把它们全部串起来！
```

### 1.1 整体架构

```
用户输入 SQL
     │
     ▼
┌──────────────────────────────────────────────────┐
│                   REPL（交互循环）                 │
│  "sql>" 提示符 → 读取 SQL → 执行 → 打印结果       │
└──────────────┬───────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────┐
│              Lexer + Parser                       │
│  SQL 字符串 → Token 序列 → AST                    │
└──────────────┬───────────────────────────────────┘
               │ AST
               ▼
┌──────────────────────────────────────────────────┐
│              ExecutionEngine                      │
│  根据 AST 类型选择执行路径：                        │
│  ├── CREATE TABLE → Catalog.CreateTable           │
│  ├── DROP TABLE   → Catalog.DropTable             │
│  ├── INSERT       → Catalog + B+Tree              │
│  └── SELECT       → 构建 Executor 算子树           │
└──────────────┬───────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────┐
│              Catalog（元数据 + 数据）               │
│  ├── Schema 管理（表结构）                          │
│  ├── 数据存储（行数据）                              │
│  └── 持久化（保存/加载）                             │
└──────────────┬───────────────────────────────────┘
               │ 读写磁盘
               ▼
┌──────────────────────────────────────────────────┐
│           磁盘文件                                 │
│  ├── schemas.txt  （表结构定义）                    │
│  └── {table}.dat  （每张表的数据文件）               │
└──────────────────────────────────────────────────┘
```

---

## 第二步：确定整合顺序

整合工作要**从内到外**：

```
整合顺序：

1. 整合基础组件
   ├── Page（Lesson 01）
   ├── DiskManager（Lesson 01）
   ├── LRUReplacer（Lesson 02）
   └── BufferPoolManager（Lesson 02）

2. 整合存储层
   ├── B+ Tree（Lesson 03）
   └── Catalog（Lesson 05，扩展持久化）

3. 整合查询层
   ├── Lexer + Parser + AST（Lesson 04）
   └── Executor（Lesson 05，扩展为完整执行引擎）

4. 整合交互层
   └── REPL（新增：交互式命令行）

5. 整合持久化
   ├── Schema 保存/加载
   └── 数据保存/加载
```

---

## 第三步：持久化设计

### 3.1 为什么需要持久化？

前 5 课的数据都存在内存中，程序退出就没了。真实数据库必须保证数据**持久存储**。

### 3.2 文件格式设计

```
lesson06_data/             ← 数据目录
├── schemas.txt            ← 所有表的结构定义
└── users.dat              ← users 表的数据
    └── orders.dat         ← orders 表的数据

schemas.txt 格式（文本，方便调试）：
  TABLE users
  COL id INT
  COL name TEXT
  COL age INT
  TABLE orders
  COL order_id INT
  COL amount INT
  END

users.dat 格式（二进制，紧凑高效）：
  [row_count: uint32]
  [type_tag: 0=INT/1=TEXT][value]...  ← 每行重复
  例：行 [1, "Alice", 30]
    00 01 00 00 00 00 00 00 00  (INT, 值=1)
    01 05 41 6C 69 63 65        (TEXT, 长度=5, "Alice")
    00 1E 00 00 00 00 00 00 00  (INT, 值=30)
```

### 3.3 保存和加载流程

```
保存（程序退出或 INSERT 后）：
  1. 遍历所有表
  2. 将 Schema 写入 schemas.txt
  3. 将每张表的行数据写入 {table_name}.dat

加载（程序启动时）：
  1. 读取 schemas.txt，重建所有表结构
  2. 读取每张表的 .dat 文件，恢复行数据
  3. 数据库恢复到上次的状态
```

---

## 第四步：REPL 交互式界面

### 4.1 REPL 设计

```
REPL = Read-Eval-Print Loop（读取-求值-打印-循环）

交互流程：
  sql> CREATE TABLE users (id INT, name TEXT, age INT)
  表 users 创建成功

  sql> INSERT INTO users VALUES (1, 'Alice', 30)
  插入 1 行

  sql> SELECT * FROM users WHERE age > 25
  id             name           age
  --------------------------------------------
  1              Alice          30
  (1 行)

  sql> .tables          ← 元命令：列出所有表
  users

  sql> .exit            ← 退出（自动保存数据）
  数据已保存，再见！
```

### 4.2 多行 SQL 支持

```
为什么需要多行？
  有些 SQL 很长，一行写不完：
  INSERT INTO users VALUES (1, 'Alice', 30)   ← 这是一行
  SELECT * FROM users                          ← 这是一行

规则：
  - 输入以分号结尾 → 执行
  - 输入以点开头（.tables） → 元命令，立即执行
  - 其他 → 继续读取下一行
```

### 4.3 元命令

```
  .tables    列出所有表
  .demo      运行内置演示
  .help      显示帮助
  .exit      退出并保存
  .quit      同 .exit
```

---

## 第五步：核心代码走读

### 5.1 main.cpp 的主循环

```cpp
int main(int argc, char* argv[]) {
    // 1. 初始化数据目录
    std::string data_dir = "lesson06_data";
    fs::create_directories(data_dir);

    // 2. 初始化引擎
    ExecutionEngine engine(data_dir);  // 自动加载已有数据

    // 3. 演示模式
    if (argc > 1 && std::string(argv[1]) == "--demo") {
        RunDemo(engine);
        return 0;
    }

    // 4. 交互式 REPL
    std::string line;
    while (true) {
        std::cout << "sql> ";
        if (!std::getline(std::cin, line)) break;

        line = Trim(line);
        if (line.empty()) continue;

        // 元命令处理
        if (line[0] == '.') {
            HandleMetaCommand(line, engine);
            continue;
        }

        // 多行 SQL：等待分号
        std::string sql = line;
        while (sql.back() != ';') {
            std::cout << "  -> ";
            std::string next;
            if (!std::getline(std::cin, next)) break;
            sql += " " + next;
        }
        sql = Trim(sql);
        if (!sql.empty() && sql.back() == ';') sql.pop_back();

        // 执行 SQL
        try {
            engine.Execute(sql);
        } catch (const std::exception& e) {
            std::cout << "错误: " << e.what() << "\n";
        }
    }

    // 5. 退出时自动保存
    engine.SaveAll();
    return 0;
}
```

### 5.2 ExecutionEngine::Execute 的分发逻辑

```cpp
void ExecutionEngine::Execute(const std::string& sql) {
    // 1. 词法分析
    Lexer lexer(sql);
    // 2. 语法分析
    Parser parser(sql);
    auto stmt = parser.Parse();

    // 3. 根据 AST 类型分发
    switch (stmt->type) {
        case Stmt::Type::CREATE_TABLE: {
            // 构造 Schema → Catalog 建表 → 保存
            Schema schema;
            schema.table_name = stmt->table_name;
            for (auto& [name, type] : stmt->column_defs) {
                schema.columns.push_back({name, type});
            }
            catalog_.CreateTable(schema);
            SaveAll();
            break;
        }
        case Stmt::Type::INSERT: {
            // 验证表存在 → 插入行 → 保存
            TableData* table = catalog_.GetTable(stmt->table_name);
            Tuple row = EvaluateInsertValues(stmt, table);
            table->InsertRow(row);
            SaveAll();
            break;
        }
        case Stmt::Type::SELECT: {
            // 构建算子树 → 执行 → 打印结果
            auto result = ExecuteSelect(stmt);
            PrintResult(result, ...);
            break;
        }
        case Stmt::Type::DROP_TABLE: {
            // 删除表 → 删除数据文件 → 保存 Schema
            catalog_.DropTable(stmt->table_name);
            SaveAll();
            break;
        }
    }
}
```

### 5.3 持久化的关键代码

```cpp
// 保存 Schema 到文本文件
void Catalog::SaveSchemas(const std::string& dir) {
    std::ofstream out(dir + "/schemas.txt");
    for (auto& [name, table] : tables_) {
        out << "TABLE " << name << "\n";
        for (auto& col : table.schema.columns) {
            out << "COL " << col.name << " " << col.type << "\n";
        }
    }
    out << "END\n";
}

// 保存表数据到二进制文件
void Catalog::SaveTableData(const std::string& dir, const std::string& table_name) {
    TableData* table = GetTable(table_name);
    std::ofstream out(dir + "/" + table_name + ".dat", std::ios::binary);

    uint32_t row_count = table->rows.size();
    out.write(reinterpret_cast<char*>(&row_count), sizeof(row_count));

    for (auto& row : table->rows) {
        for (auto& val : row) {
            if (std::holds_alternative<int64_t>(val)) {
                int8_t tag = 0;  // INT
                int64_t v = std::get<int64_t>(val);
                out.write(reinterpret_cast<char*>(&tag), 1);
                out.write(reinterpret_cast<char*>(&v), sizeof(v));
            } else {
                int8_t tag = 1;  // TEXT
                std::string s = std::get<std::string>(val);
                uint32_t len = s.size();
                out.write(reinterpret_cast<char*>(&tag), 1);
                out.write(reinterpret_cast<char*>(&len), sizeof(len));
                out.write(s.data(), len);
            }
        }
    }
}
```

---

## 第六步：运行演示

### 6.1 自动演示模式

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
make lesson06
./lesson06_mvp/lesson06 --demo
```

### 6.2 交互式模式

```bash
./lesson06_mvp/lesson06
```

**交互示例**：

```
sql> CREATE TABLE employees (id INT, name TEXT, salary INT)
表 employees 创建成功

sql> INSERT INTO employees VALUES (1, 'Alice', 5000)
插入 1 行

sql> INSERT INTO employees VALUES (2, 'Bob', 3500)
插入 1 行

sql> INSERT INTO employees VALUES (3, 'Charlie', 6000)
插入 1 行

sql> INSERT INTO employees VALUES (4, 'Diana', 4500)
插入 1 行

sql> SELECT * FROM employees
id             name           salary
------------------------------------------------------------
1              Alice          5000
2              Bob            3500
3              Charlie        6000
4              Diana          4500
(4 行)

sql> SELECT name, salary FROM employees WHERE salary > 4000
name           salary
------------------------------
Alice          5000
Charlie        6000
Diana          4500
(3 行)

sql> DROP TABLE employees
表 employees 已删除

sql> .exit
数据已保存，再见！
```

### 6.3 验证持久化

```bash
# 第一次运行：创建表并插入数据
./lesson06_mvp/lesson06
sql> CREATE TABLE test (id INT, value TEXT)
sql> INSERT INTO test VALUES (1, 'hello')
sql> .exit

# 第二次运行：数据还在！
./lesson06_mvp/lesson06
sql> SELECT * FROM test
id             value
--------------------
1              hello
(1 行)
```

---

## 编译运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
cmake ..
make lesson06

# 自动演示
./lesson06_mvp/lesson06 --demo

# 交互式
./lesson06_mvp/lesson06
```

---

## 本课知识点总结

```
你学到了：

系统集成：
  ✓ 如何将独立组件整合成完整系统
  ✓ 模块间接口设计（Lexer → Parser → Executor → Catalog）
  ✓ 错误传播和全局异常处理

持久化：
  ✓ Schema 的文本格式存储
  ✓ 数据的二进制格式存储
  ✓ 程序启动时的数据恢复
  ✓ 程序退出时的自动保存

交互设计：
  ✓ REPL 模式：读取-求值-打印-循环
  ✓ 多行 SQL 支持
  ✓ 元命令系统
  ✓ 格式化输出

完整数据流：
  用户输入 SQL
    → Lexer 词法分析
    → Parser 语法分析
    → AST
    → ExecutionEngine 分发
    → Catalog 查找元数据
    → Executor 构建算子树
    → 执行并返回结果
    → 格式化打印
    → （可选）持久化到磁盘
```

---

## 课程总回顾

```
恭喜你完成了整个课程！

6 节课的知识地图：

Lesson 01 存储引擎      ──→ 磁盘上的页面管理
Lesson 02 缓冲池        ──→ 内存中的页面缓存
Lesson 03 B+ 树         ──→ 高效的索引结构
Lesson 04 SQL 解析器    ──→ SQL 字符串到 AST
Lesson 05 查询执行器    ──→ 火山模型算子执行
Lesson 06 完整 MVP      ──→ 整合 + 持久化 + REPL

你现在理解了数据库引擎的核心组件：
  1. 存储层：Page + DiskManager
  2. 缓冲层：BufferPool + LRU
  3. 索引层：B+ Tree
  4. 解析层：Lexer + Parser + AST
  5. 执行层：Volcano Model Executors
  6. 元数据：Catalog + Persistence
```

---

## 进阶方向

学完本课程后，你可以继续探索：

| 方向 | 内容 |
|------|------|
| **查询优化** | 谓词下推、常量折叠、代价模型 |
| **事务** | ACID、WAL（预写日志）、MVCC（多版本并发控制） |
| **JOIN** | Nested Loop Join、Hash Join、Sort-Merge Join |
| **排序与聚合** | ORDER BY、GROUP BY、COUNT/SUM/AVG |
| **网络层** | 客户端-服务器架构、TCP 协议 |
| **更多 SQL** | 子查询、DISTINCT、LIMIT、UPDATE、DELETE |

---

## 推荐学习资源

1. **CMU 15-445**：数据库系统（课程项目就是实现一个迷你数据库）
2. **《数据库系统内幕》**：深入理解存储引擎和事务
3. **SQLite 源码**：世界上最广泛部署的数据库，代码质量极高
4. **《FlexBPlusTree》**：了解工业级 B+ 树实现
