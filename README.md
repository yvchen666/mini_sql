# 从零实现数据库引擎 —— C++ 教学课程

## 项目介绍

本课程带你从零开始，用 C++ 实现一个完整的关系型数据库引擎 MVP（最小可行产品）。

每一课聚焦一个核心组件，循序渐进，最终整合为一个能持久化数据、支持 SQL 查询的完整数据库系统。

```
用户输入 SQL
     │
     ▼
┌─────────────┐
│  SQL解析器   │  ← 第4课：词法分析 + 语法分析 → AST
│  (Parser)   │
└─────┬───────┘
      │ AST
      ▼
┌─────────────┐
│  查询执行器  │  ← 第5课：火山模型执行器
│ (Executor)  │
└─────┬───────┘
      │
      ▼
┌─────────────┐
│  B+树索引   │  ← 第3课：高效的键值查找
│  (B+Tree)   │
└─────┬───────┘
      │
      ▼
┌─────────────┐
│  缓冲池管理  │  ← 第2课：内存与磁盘的桥梁
│  (Buffer)   │
└─────┬───────┘
      │
      ▼
┌─────────────┐
│  磁盘管理器  │  ← 第1课：最底层的存储抽象
│(DiskManager)│
└─────────────┘
      │
      ▼
   磁盘文件
```

## 学习路径

| 课程 | 主题 | 核心概念 |
|------|------|----------|
| Lesson 01 | 存储引擎基础 | Page、DiskManager |
| Lesson 02 | 缓冲池管理 | LRU 替换、BufferPoolManager |
| Lesson 03 | B+树索引 | 节点分裂/合并、有序存储 |
| Lesson 04 | SQL 解析器 | 词法分析、递归下降解析、AST |
| Lesson 05 | 查询执行器 | 火山模型、Catalog、SeqScan |
| Lesson 06 | 完整 MVP   | 所有组件整合、持久化 REPL |

## 环境要求

- C++17 或更高版本
- CMake 3.16 或更高版本
- Linux/macOS（Windows 需调整文件路径）

## 快速开始

```bash
# 编译所有课程
cd /home/aoi/AWorkSpace/sql_mvp
mkdir build && cd build
cmake ..
make -j4

# 运行某一课的演示
./lesson01_storage/lesson01
./lesson02_buffer_pool/lesson02
./lesson03_btree/lesson03
./lesson04_sql_parser/lesson04
./lesson05_executor/lesson05
./lesson06_mvp/lesson06
```

## 各课概述

### Lesson 01 — 存储引擎基础
数据库最底层是磁盘文件。本课实现：
- `Page`：固定大小（4096 字节）的数据块，是 I/O 的最小单位
- `DiskManager`：封装文件读写，以页面为粒度进行 I/O

### Lesson 02 — 缓冲池管理
频繁的磁盘 I/O 极慢，缓冲池用内存缓存热点页面。本课实现：
- `LRUReplacer`：LRU 淘汰算法（双向链表 + 哈希表）
- `BufferPoolManager`：管理固定数量的内存帧，调度页面换入换出

### Lesson 03 — B+树索引
B+树是数据库最常用的索引结构。本课实现：
- 内部节点和叶子节点
- Insert/Search/Delete 操作
- 节点分裂与合并

### Lesson 04 — SQL 解析器
把 SQL 字符串变成程序能理解的数据结构。本课实现：
- `Lexer`：将 SQL 字符串切分为 Token 序列
- `Parser`：递归下降解析，生成 AST（抽象语法树）
- 支持 SELECT、INSERT、CREATE TABLE、DROP TABLE

### Lesson 05 — 查询执行器
用"火山模型"逐行产出结果。本课实现：
- `Catalog`：管理表的元数据（schema）
- `Executor`：SeqScan（全表扫描）、Insert（插入）、Filter（过滤）
- 简单内存表存储

### Lesson 06 — 完整 MVP
整合前5课，实现完整的数据库系统：
- 交互式 SQL REPL
- 数据持久化到磁盘
- 支持 CREATE TABLE、INSERT、SELECT、DROP TABLE

## 版权与用途

本项目仅用于教学目的，代码力求清晰易读，而非追求生产环境性能。
