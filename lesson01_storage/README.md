# Lesson 01 — 存储引擎基础

## 学习目标

1. 理解数据库为什么要以"页面"为单位管理数据
2. 实现 `Page` 类：固定大小的数据块
3. 实现 `DiskManager` 类：将页面读写到磁盘文件
4. 跑通完整的写入→读取→验证流程

---

## 核心概念

### 为什么要用"页面"（Page）？

操作系统与磁盘通信时，并不是读一个字节就从磁盘取一个字节。磁盘最小读写单位通常是 512 字节或 4096 字节的"扇区/块"。

数据库顺应这一特性，把所有数据组织成固定大小的**页面（Page）**，每次 I/O 都读/写整个页面，好处是：

- 减少 I/O 次数（一次读出整页，可供多次使用）
- 便于管理（页面编号 = 文件偏移 / 页面大小）
- 便于缓冲池缓存（第2课）

```
磁盘文件布局：

┌──────────────────────────────────────────────────────┐
│  Page 0  │  Page 1  │  Page 2  │  Page 3  │  ...    │
│ 4096字节  │ 4096字节  │ 4096字节  │ 4096字节  │        │
└──────────────────────────────────────────────────────┘
  offset=0   offset=4096  offset=8192  offset=12288
```

### Page 结构

```
┌────────────────────────────────┐
│           Page Header          │
│  page_id (4B) | is_dirty (1B)  │
├────────────────────────────────┤
│                                │
│           Page Data            │
│         (4091 字节)             │
│                                │
└────────────────────────────────┘
```

### DiskManager 职责

```
应用层                DiskManager              磁盘文件
   │                      │                      │
   │ WritePage(id, data)   │                      │
   │─────────────────────>│  seek(id * PAGE_SIZE) │
   │                      │─────────────────────>│
   │                      │  write(data, 4096)    │
   │                      │─────────────────────>│
   │                      │                      │
   │ ReadPage(id, buf)     │                      │
   │─────────────────────>│  seek(id * PAGE_SIZE) │
   │                      │─────────────────────>│
   │                      │  read(buf, 4096)      │
   │                      │<─────────────────────│
   │<─────────────────────│                      │
```

---

## 关键代码讲解

### Page 类

```cpp
// 页面大小固定为 4096 字节（与操作系统块大小对齐）
static constexpr size_t PAGE_SIZE = 4096;

class Page {
    page_id_t page_id_;     // 页面编号（在文件中的位置）
    char data_[PAGE_SIZE];  // 实际存储数据的字节数组
    bool is_dirty_;         // 脏页标记：内存数据是否比磁盘新
};
```

**脏页（Dirty Page）**：当页面在内存中被修改，但还没有写回磁盘时，称为"脏页"。数据库需要追踪脏页，确保在适当时机将其刷盘。

### DiskManager 关键操作

```cpp
// 写页面：计算文件偏移，写入 PAGE_SIZE 字节
void WritePage(page_id_t page_id, const char* data) {
    size_t offset = page_id * PAGE_SIZE;
    db_file_.seekp(offset);
    db_file_.write(data, PAGE_SIZE);
}
```

---

## 思考题

1. 为什么页面大小选 4096 字节？选 512 字节或 65536 字节会有什么影响？
2. 如果程序在写页面中途崩溃（只写了一半），会发生什么？数据库如何防止这种情况？（提示：WAL 日志）
3. `page_id` 从 0 开始，如何计算 page_id=5 的页面在文件中的起始偏移？

---

## 编译与运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp
mkdir -p build && cd build
cmake ..
make lesson01
./lesson01_storage/lesson01
```

预期输出：
```
=== Lesson 01: 存储引擎基础 ===
[DiskManager] 创建数据库文件: lesson01.db
写入页面 0，内容: Hello, Database Page 0!
写入页面 1，内容: Hello, Database Page 1!
写入页面 2，内容: Hello, Database Page 2!
读取页面 0 成功，内容: Hello, Database Page 0!
读取页面 1 成功，内容: Hello, Database Page 1!
读取页面 2 成功，内容: Hello, Database Page 2!
验证成功！数据写入和读取正确。
[DiskManager] 共分配了 3 个页面
```
