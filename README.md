# 从零实现数据库引擎 — C++ 教学课程

一套**保姆级**的数据库引擎实现教程。6 课从零开始，用纯 C++（仅 STL）逐步构建一个支持 SQL 查询、数据持久化的完整数据库 MVP。

每课包含：**原理讲解 → 编码顺序（先写什么后写什么）→ 逐段代码详解（为什么这么写）→ 完整可编译代码 → 预期输出 → 思考题**。

---

## 你将学到什么

```
Lesson 01  Page + DiskManager       磁盘上的页面读写
Lesson 02  LRU + BufferPoolManager  内存中的页面缓存
Lesson 03  B+ Tree                  高效的索引结构
Lesson 04  Lexer + Parser + AST     SQL 字符串到结构化表示
Lesson 05  Catalog + Executor       火山模型查询执行
Lesson 06  完整 MVP                  整合 + 持久化 + 交互式 REPL
```

完成全部 6 课后，你将理解数据库引擎的完整分层架构：

```
                          用户输入 SQL
                               │
                               ▼
                    ┌─────────────────────┐
                    │     SQL 解析器       │  Lexer + Parser → AST
                    │  (Lesson 04)        │
                    └──────────┬──────────┘
                               │ AST
                               ▼
                    ┌─────────────────────┐
                    │     查询执行器       │  火山模型算子树
                    │  (Lesson 05)        │  SeqScan → Filter → Projection
                    └──────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    │     元数据管理       │  Catalog（表结构 + 数据）
                    │  (Lesson 05)        │
                    └──────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    │     B+ 树索引        │  O(log n) 查找与范围查询
                    │  (Lesson 03)        │
                    └──────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    │     缓冲池管理       │  LRU 缓存 + Pin/Unpin
                    │  (Lesson 02)        │
                    └──────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    │     磁盘管理器       │  页面级文件 I/O
                    │  (Lesson 01)        │
                    └──────────┬──────────┘
                               │
                               ▼
                          磁盘文件 (.db)
```

---

## 课程目录

| 课程 | 目录 | 你将实现 | 核心知识点 |
|:----:|:-----|:---------|:-----------|
| 01 | `lesson01_storage/` | `Page` + `DiskManager` | 页面模型、堆文件、文件偏移计算、脏页标记 |
| 02 | `lesson02_buffer_pool/` | `LRUReplacer` + `BufferPoolManager` | LRU 算法、Pin/Unpin 机制、缓存命中/淘汰、脏页刷盘 |
| 03 | `lesson03_btree/` | `BTree` + `InternalNode` + `LeafNode` | B+ 树查找/插入/删除、节点分裂与合并、范围查询 |
| 04 | `lesson04_sql_parser/` | `Lexer` + `Parser` + `AST` | 词法分析、递归下降解析、LL(1) 前瞻、抽象语法树 |
| 05 | `lesson05_executor/` | `Catalog` + `SeqScan` + `Filter` + `Projection` | 火山模型、算子组合、谓词求值、投影 |
| 06 | `lesson06_mvp/` | 全部整合 + REPL + 持久化 | 系统集成、二进制存储格式、交互式 SQL、数据恢复 |

### 各课关系图

```
  Lesson 01        Lesson 02        Lesson 03
  存储引擎          缓冲池           B+ 树索引
  (底层基础)        (性能优化)        (高效查找)
      │                │                │
      └────────────────┼────────────────┘
                       │
              ┌────────┴────────┐
              │  Lesson 06      │
              │  完整 MVP       │
              │  (集成 + REPL)  │
              └────────┬────────┘
                       │
       ┌───────────────┼───────────────┐
       │               │               │
  Lesson 04       Lesson 05
  SQL 解析器       查询执行器
  (前端)           (后端)
```

---

## 每课内容概要

### Lesson 01 — 存储引擎基础：Page 与 DiskManager

> 编码顺序：`Page` → `DiskManager` → `main.cpp`

数据库在磁盘上就是一个文件，但数据按 4KB 的"页面"组织。本课你将写出：
- **`Page`**：4096 字节的固定大小数据块，附带 page_id 和脏标记
- **`DiskManager`**：通过 `page_id × PAGE_SIZE` 计算偏移，直接定位到文件任意位置读写
- 学到为什么用 `char data_[]` 而不是 `vector<char>`，为什么写入后要 `flush()`

### Lesson 02 — 缓冲池管理：LRU 替换与 BufferPoolManager

> 编码顺序：`Page(含pin_count)` → `LRUReplacer` → `BufferPoolManager` → `main.cpp`

磁盘比内存慢 10 万倍，缓冲池用内存缓存热点页面。本课你将写出：
- **`LRUReplacer`**：双向链表 + 哈希表，O(1) 的淘汰算法
- **`BufferPoolManager`**：管理固定帧数，FetchPage 缓存命中则直接返回，未命中则从磁盘加载
- Pin/Unpin 引用计数保护正在使用的页面不被淘汰

### Lesson 03 — B+ 树索引

> 编码顺序：`BTreeNode` → `InternalNode` + `LeafNode` → `BTree` → `main.cpp`

数据库最常用的索引结构，100 万条数据只需 3 次磁盘 I/O。本课你将写出：
- **内部节点**：只存 key，用来导航到正确的子树
- **叶子节点**：存 key + value，用链表串联（支持范围查询）
- 完整的 Insert/Search/Delete，包括节点分裂与合并

### Lesson 04 — SQL 解析器：从字符串到 AST

> 编码顺序：`TokenType + Token` → `AST 节点` → `Lexer` → `Parser` → `main.cpp`

把 SQL 字符串变成程序能处理的结构化数据。本课你将写出：
- **`Lexer`**：逐字符扫描，识别关键字、标识符、数字、字符串、运算符
- **`Parser`**：递归下降解析，LL(1) 前瞻，生成 AST
- 支持 SELECT（含 WHERE）、INSERT、CREATE TABLE、DROP TABLE

### Lesson 05 — 查询执行器：火山模型

> 编码顺序：`基础类型` → `Catalog` → `SeqScan` → `Filter` → `Projection` → `ExecutionEngine` → `main.cpp`

用经典火山模型执行 SQL 查询。本课你将写出：
- **`Catalog`**：管理表的 Schema（结构定义）和行数据
- **算子**：每个实现 `Init()/Next()` 接口，上层通过 `Next()` 拉取数据
- `SeqScan`（全表扫描）→ `Filter`（WHERE 过滤）→ `Projection`（SELECT 列选择）

### Lesson 06 — 完整 MVP：把所有组件整合

> 整合顺序：基础组件 → 存储层 → 查询层 → 交互层 → 持久化

前 5 课的组件全部整合成一个可用的数据库：
- **交互式 REPL**：`sql>` 提示符，支持多行 SQL、元命令（`.tables`、`.exit`）
- **数据持久化**：Schema 存文本格式，数据存二进制格式，重启后自动恢复
- 支持 `CREATE TABLE`、`INSERT INTO`、`SELECT ... WHERE`、`DROP TABLE`

```
$ ./lesson06_mvp/lesson06
sql> CREATE TABLE employees (id INT, name TEXT, salary INT)
表 employees 创建成功

sql> INSERT INTO employees VALUES (1, 'Alice', 5000)
sql> INSERT INTO employees VALUES (2, 'Bob', 3500)
sql> INSERT INTO employees VALUES (3, 'Charlie', 6000)

sql> SELECT name, salary FROM employees WHERE salary > 4000
name           salary
------------------------------
Alice          5000
Charlie        6000
(2 行)

sql> .exit
数据已保存，再见！
```

---

## 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| C++ 编译器 | 支持 C++17 | GCC 7+ / Clang 5+ / MSVC 2017+ |
| CMake | 3.16+ | 构建系统 |
| 无第三方库 | — | 仅使用 STL |

## 快速开始

```bash
# 克隆项目
git clone https://github.com/yvchen666/SQL_MVP.git
cd SQL_MVP

# 编译所有课程
mkdir build && cd build
cmake ..
make -j$(nproc)

# 按顺序学习（推荐）
./lesson01_storage/lesson01      # 存储引擎
./lesson02_buffer_pool/lesson02  # 缓冲池
./lesson03_btree/lesson03        # B+ 树
./lesson04_sql_parser/lesson04   # SQL 解析器
./lesson05_executor/lesson05     # 查询执行器

# 最终 MVP
./lesson06_mvp/lesson06          # 交互式 SQL 数据库
./lesson06_mvp/lesson06 --demo   # 自动演示模式
```

## 项目结构

```
SQL_MVP/
├── README.md                    ← 你在这里
├── CMakeLists.txt               ← 根构建文件
├── lesson01_storage/            ← 第 1 课：Page + DiskManager
├── lesson02_buffer_pool/        ← 第 2 课：LRU + BufferPoolManager
├── lesson03_btree/              ← 第 3 课：B+ 树索引
├── lesson04_sql_parser/         ← 第 4 课：SQL 词法/语法分析
├── lesson05_executor/           ← 第 5 课：火山模型执行器
└── lesson06_mvp/                ← 第 6 课：完整数据库 MVP
    ├── include/                 ← 整合版头文件
    ├── src/                     ← 整合版源文件
    └── main.cpp                 ← REPL 入口
```

每课目录独立，包含自己的 `CMakeLists.txt`、`README.md` 和完整源码，可单独编译运行。

## 学习建议

1. **按顺序学习**：每课依赖前一课的知识，跳课可能导致理解断层
2. **先读 README 再看代码**：README 中的编码顺序告诉你应该先看哪个文件
3. **动手修改**：改改常量（如 PAGE_SIZE、BTREE_ORDER），观察行为变化
4. **完成思考题**：每课末尾的思考题帮助你深入理解设计决策

## 进阶方向

学完本课程后，可以继续探索：

| 方向 | 内容 |
|------|------|
| 查询优化 | 谓词下推、常量折叠、代价模型 |
| 事务处理 | ACID、WAL 预写日志、MVCC 多版本并发控制 |
| JOIN 算法 | Nested Loop Join、Hash Join、Sort-Merge Join |
| SQL 扩展 | ORDER BY、GROUP BY、子查询、UPDATE、DELETE |
| 网络层 | 客户端-服务器架构、TCP 协议通信 |
| 存储引擎 | LSM Tree、列式存储、压缩 |

## 推荐学习资源

- **CMU 15-445/645**：数据库系统课程（BusTub 项目）
- **《数据库系统内幕》**（Alex Petrov）：深入存储引擎与事务
- **SQLite 源码**：世界上最广泛部署的数据库，代码质量极高
- **《Crafting Interpreters》**：编译原理入门，对理解解析器很有帮助

## 版权与用途

本项目仅用于教学目的，代码力求清晰易读，而非追求生产环境性能。
