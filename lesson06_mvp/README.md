# Lesson 06 — 完整 MVP 数据库

## 学习目标

1. 将前5课的所有组件整合为一个完整的数据库系统
2. 实现数据持久化（数据存到磁盘，重启后不丢失）
3. 实现交互式 SQL REPL（Read-Eval-Print Loop）
4. 支持完整的 CREATE TABLE、INSERT、SELECT、DROP TABLE

---

## 系统架构

```
用户输入 SQL
      │
      ▼
┌─────────────────────────────────────────────────┐
│                    REPL                          │
│         (读取 → 解析 → 执行 → 打印)              │
└──────────────────────┬──────────────────────────┘
                       │
            ┌──────────▼──────────┐
            │    SQL 解析器        │  ← 词法分析 + 递归下降解析
            │  Lexer + Parser      │
            └──────────┬──────────┘
                       │ AST
            ┌──────────▼──────────┐
            │    执行引擎          │  ← 根据 AST 构建算子树并执行
            │  ExecutionEngine    │
            └──────────┬──────────┘
                       │
         ┌─────────────┼──────────────┐
         │             │              │
    ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
    │ Catalog │   │  B+树   │   │缓冲池   │
    │(元数据)  │   │ (索引)  │   │(内存缓存)│
    └─────────┘   └─────────┘   └────┬────┘
                                      │
                               ┌──────▼──────┐
                               │  DiskManager │
                               │  (磁盘文件)  │
                               └─────────────┘
                                      │
                               ┌──────▼──────┐
                               │   .db 文件   │
                               └─────────────┘
```

## 持久化方案

本 MVP 采用简化的持久化方案：

1. 表结构（Schema）存储在 `schema.db` 文件中（文本格式）
2. 表数据存储在 `{table_name}.db` 文件中（二进制格式，以页面为单位）
3. 每次写入后立即刷盘（简单但不高效）

真实数据库通常使用 WAL（Write-Ahead Logging）日志来保证崩溃安全。

---

## 使用方法

```bash
./lesson06_mvp/lesson06

# 进入交互式 REPL：
sql> CREATE TABLE users (id INT, name TEXT, age INT);
sql> INSERT INTO users VALUES (1, 'Alice', 30);
sql> INSERT INTO users VALUES (2, 'Bob', 25);
sql> SELECT * FROM users;
sql> SELECT name FROM users WHERE age > 26;
sql> DROP TABLE users;
sql> .exit
```

## 支持的命令

| 命令 | 说明 |
|------|------|
| `CREATE TABLE name (col TYPE, ...)` | 创建表 |
| `INSERT INTO name VALUES (v1, v2, ...)` | 插入一行 |
| `SELECT */cols FROM name [WHERE col op val]` | 查询 |
| `DROP TABLE name` | 删除表 |
| `.tables` | 列出所有表 |
| `.exit` | 退出 |

---

## 编译与运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
make lesson06
./lesson06_mvp/lesson06
```
