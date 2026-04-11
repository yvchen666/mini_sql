# Lesson 04 — SQL 解析器

## 学习目标

1. 理解编译器前端的基本流程：源码 → Token → AST
2. 实现词法分析器（Lexer）：将 SQL 字符串分解为 Token 序列
3. 实现语法分析器（Parser）：用递归下降解析法生成 AST
4. 支持 SELECT、INSERT、CREATE TABLE、DROP TABLE 语句

---

## 核心概念

### SQL 解析的三个阶段

```
原始 SQL 字符串
"SELECT name FROM users WHERE id = 1"
         │
         ▼ 词法分析（Lexer）
Token 序列：
  [SELECT] [name] [FROM] [users] [WHERE] [id] [=] [1]
         │
         ▼ 语法分析（Parser）
AST（抽象语法树）：
  SelectStmt
  ├── columns: ["name"]
  ├── table: "users"
  └── where: BinaryExpr(=)
              ├── left: Column("id")
              └── right: IntLiteral(1)
         │
         ▼ 执行器（Lesson 05）
  结果集
```

### Token 类型

```
关键字：SELECT, FROM, WHERE, INSERT, INTO, VALUES,
        CREATE, TABLE, DROP, INT, TEXT
标识符：users, name, id（变量名、表名、列名）
字面量：42（整数），'hello'（字符串）
符号：  , ( ) = * ;
```

### 递归下降解析

每种语法规则对应一个解析函数，函数之间互相调用：

```
parseStatement()
  ├── parseSelect()     ← 遇到 SELECT
  ├── parseInsert()     ← 遇到 INSERT
  ├── parseCreate()     ← 遇到 CREATE
  └── parseDrop()       ← 遇到 DROP

parseSelect():
  expect(SELECT) → parseColumnList() → expect(FROM) → parseIdentifier()
  → [if WHERE: expect(WHERE) → parseExpression()]
```

---

## AST 节点类型

```
Stmt（语句基类）
  ├── SelectStmt：SELECT columns FROM table [WHERE condition]
  ├── InsertStmt：INSERT INTO table VALUES (v1, v2, ...)
  ├── CreateTableStmt：CREATE TABLE name (col1 TYPE, col2 TYPE, ...)
  └── DropTableStmt：DROP TABLE name

Expr（表达式基类）
  ├── IntLiteralExpr：整数字面量
  ├── StrLiteralExpr：字符串字面量
  ├── ColumnRefExpr：列引用（列名）
  └── BinaryExpr：二元运算（=, >, <）
```

---

## 思考题

1. 词法分析器如何处理多字节字符（如中文）？
2. 递归下降解析器如何处理优先级（例如 `a + b * c`）？
3. 错误恢复（Error Recovery）是什么？为什么重要？

---

## 编译与运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
make lesson04
./lesson04_sql_parser/lesson04
```
