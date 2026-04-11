# Lesson 04 — SQL 解析器：从字符串到 AST

> **本课目标**：理解编译原理的基础知识，实现一个 SQL 词法分析器（Lexer）和语法分析器（Parser）。
> 你将写出三个核心组件：`Token`/`Lexer`（词法分析）、`AST`（抽象语法树）、`Parser`（语法分析）。

---

## 第一步：理解问题——数据库如何理解 SQL？

### 1.1 SQL 执行的三阶段

当你输入 `SELECT name FROM users WHERE age > 25` 时，数据库要做这些事：

```
SQL 字符串                   词法分析                  语法分析
"SELECT name               Token 序列               AST（树形结构）
 FROM users                [SELECT, name,            SelectStmt
 WHERE age > 25"            FROM, users,              ├── columns: [name]
     │                      WHERE, age, >,            ├── table: users
     │                      25, SEMICOLON]             └── where: age > 25
     │                          │                          │
     ▼                          ▼                          ▼
  用户输入 ─────→ Lexer（词法分析）─────→ Parser（语法分析）──→ 执行器
```

本课实现前两个阶段。

### 1.2 为什么不直接用正则匹配？

```
正则能做到的：
  匹配 "SELECT * FROM users"  → 固定模式

正则做不到的：
  匹配嵌套表达式：WHERE (a > 1 AND (b < 2 OR c = 3))
  匹配任意 SQL 语法：列数可变、WHERE 可选、嵌套子查询

所以我们需要：词法分析 + 语法分析
```

---

## 第二步：确定编码顺序

```
编写顺序：

1. Token 定义 + TokenType 枚举  ← 最基础的：SQL 中的"词"有哪些类型
   ↑
2. AST 节点定义                ← 目标结构：解析后的结果长什么样
   ↑
3. Lexer（词法分析器）          ← 把字符串变成 Token 序列
   ↑
4. Parser（语法分析器）         ← 把 Token 序列变成 AST
   ↑
5. main.cpp                    ← 测试验证
```

**为什么先定义 AST 再写 Parser？**
- Parser 的任务是"产出 AST"
- 先定义好 AST 的结构，Parser 的代码才有明确的目标
- 就像盖房子：先画图纸（AST），再施工（Parser）

---

## 第三步：定义 Token 和 TokenType

### 3.1 什么是 Token？

Token 是 SQL 中最小的、不可再分的语法单位。

```
SQL: SELECT name FROM users WHERE age > 25

Token 序列：
  [SELECT] [name] [FROM] [users] [WHERE] [age] [>] [25]

每个 Token 有：
  - 类型（TokenType）：关键字？标识符？数字？运算符？
  - 值（value）：原始文本
  - 行号（line）：用于报错
```

### 3.2 TokenType 分类

```cpp
enum class TokenType {
    // 关键字
    SELECT, FROM, WHERE, INSERT, INTO, VALUES,
    CREATE, TABLE, DROP, INT, TEXT,
    AND, OR,

    // 标识符（表名、列名）
    IDENTIFIER,

    // 字面量（值）
    INT_LITERAL,       // 123
    STRING_LITERAL,    // "hello"

    // 运算符
    STAR, COMMA, LPAREN, RPAREN, SEMICOLON,
    EQ, NEQ, LT, GT, LTE, GTE,

    // 特殊
    END_OF_FILE, UNKNOWN
};
```

**为什么关键字和标识符要分开？**
- 关键字有特殊含义（SELECT、FROM 等是语法的一部分）
- 标识符是用户定义的名字（表名、列名）
- Parser 需要区分它们来决定走哪条语法分支

### 3.3 Token 结构

```cpp
struct Token {
    TokenType type;
    std::string value;
    int line;

    Token(TokenType t, const std::string& v, int l)
        : type(t), value(v), line(l) {}
};
```

---

## 第四步：定义 AST 节点

### 4.1 什么是 AST？

AST（Abstract Syntax Tree，抽象语法树）是 SQL 的结构化表示：

```
SQL: SELECT name, age FROM users WHERE age > 25

AST：
                    SelectStmt
                   /     |      \
          columns    table    where
          /    \       |        |
        name   age   users   BinaryOp(>)
                            /      \
                        ColRef(age)  IntLit(25)
```

### 4.2 AST 节点设计

```cpp
// ---- 表达式节点 ----
struct Expr {
    enum class Type {
        INT_LITERAL,     // 整数字面量：25
        STRING_LITERAL,  // 字符串字面量："hello"
        COLUMN_REF,      // 列引用：age, name
        BINARY_OP        // 二元运算：age > 25
    } type;

    int64_t int_val;           // INT_LITERAL 的值
    std::string str_val;       // STRING_LITERAL 或 COLUMN_REF 的名字
    std::string op;            // 二元运算符：>, <, =, AND, OR
    std::shared_ptr<Expr> left;   // 左操作数
    std::shared_ptr<Expr> right;  // 右操作数
};

// ---- 语句节点 ----
struct Stmt {
    enum class Type { SELECT, INSERT, CREATE_TABLE, DROP_TABLE } type;

    // SELECT 的字段
    std::vector<std::string> select_columns;

    // 表名（SELECT/INSERT/DROP 共用）
    std::string table_name;

    // WHERE 条件
    std::shared_ptr<Expr> where_expr;

    // INSERT 的值列表
    std::vector<std::shared_ptr<Expr>> insert_values;

    // CREATE TABLE 的列定义
    std::vector<std::pair<std::string, std::string>> column_defs;  // (name, type)
};
```

**为什么用 `shared_ptr`？**
- AST 是树形结构，节点之间有共享（同一个 Expr 可能被多处引用）
- `shared_ptr` 自动管理内存，不需要手动 delete
- 比 `unique_ptr` 更灵活，允许多个节点指向同一个子树

---

## 第五步：实现 Lexer（词法分析器）

### 5.1 Lexer 的工作

```
输入："SELECT name FROM users WHERE age > 25"

处理过程（逐字符扫描）：

  S-E-L-E-C-T  → 关键字 SELECT
  空格         → 跳过
  n-a-m-e      → 标识符 "name"
  空格         → 跳过
  F-R-O-M      → 关键字 FROM
  空格         → 跳过
  u-s-e-r-s    → 标识符 "users"
  空格         → 跳过
  W-H-E-R-E    → 关键字 WHERE
  空格         → 跳过
  a-g-e        → 标识符 "age"
  空格         → 跳过
  >            → 运算符 GT
  空格         → 跳过
  2-5          → 整数字面量 25

输出：[SELECT, name, FROM, users, WHERE, age, >, 25]
```

### 5.2 代码详解

```cpp
class Lexer {
public:
    explicit Lexer(const std::string& source)
        : source_(source), pos_(0), line_(1) {}

    // 获取下一个 Token
    Token NextToken() {
        SkipWhitespace();  // 跳过空白字符

        if (pos_ >= source_.size()) {
            return Token(TokenType::END_OF_FILE, "", line_);
        }

        char c = source_[pos_];

        // 根据首字符分类处理
        if (std::isalpha(c) || c == '_') {
            return ReadIdentifierOrKeyword();
        } else if (std::isdigit(c)) {
            return ReadNumber();
        } else if (c == '\'') {
            return ReadString();
        } else {
            return ReadOperator();
        }
    }

private:
    void SkipWhitespace() {
        while (pos_ < source_.size() &&
               (source_[pos_] == ' ' || source_[pos_] == '\t' ||
                source_[pos_] == '\n' || source_[pos_] == '\r')) {
            if (source_[pos_] == '\n') line_++;
            pos_++;
        }
    }

    Token ReadIdentifierOrKeyword() {
        std::string word;
        while (pos_ < source_.size() &&
               (std::isalnum(source_[pos_]) || source_[pos_] == '_')) {
            word += source_[pos_++];
        }

        // 检查是否是关键字（不区分大小写）
        std::string upper = ToUpper(word);
        if (auto it = keywords_.find(upper); it != keywords_.end()) {
            return Token(it->second, word, line_);
        }
        return Token(TokenType::IDENTIFIER, word, line_);
    }

    // ... ReadNumber, ReadString, ReadOperator 类似
};
```

**为什么关键字不区分大小写？**
- SQL 标准规定关键字不区分大小写
- `SELECT`、`select`、`Select` 都合法
- 但标识符（表名、列名）通常区分大小写

**为什么逐字符扫描而不是用正则？**
- 手写扫描器性能更好（避免正则引擎开销）
- 对错误处理有完全控制
- 可以精确追踪位置信息（行列号）

---

## 第六步：实现 Parser（语法分析器）

### 6.1 什么是递归下降解析？

递归下降是一种**自顶向下**的解析方法：
- 每个语法规则对应一个函数
- 函数通过递归调用来处理嵌套结构

### 6.2 SQL 的语法规则（简化版）

```
Stmt       → SelectStmt | InsertStmt | CreateStmt | DropStmt
SelectStmt → SELECT ColumnList FROM IDENTIFIER [WHERE Expr]
InsertStmt → INSERT INTO IDENTIFIER VALUES ValueList
CreateStmt → CREATE TABLE IDENTIFIER ( ColumnDefList )
DropStmt   → DROP TABLE IDENTIFIER
ColumnList → * | Column (, Column)*
ValueList  → Value (, Value)*
Expr       → Comparison (AND|OR Comparison)*
Comparison → Value (= | != | < | > | <= | >=) Value
Value      → INT_LITERAL | STRING_LITERAL | IDENTIFIER
```

### 6.3 解析过程图解

```
输入：SELECT name, age FROM users WHERE age > 25

Parser::Parse()
  → 看到第一个 Token 是 SELECT → 调用 ParseSelect()

Parser::ParseSelect()
  → 消费 SELECT
  → 调用 ParseColumnList()：
       消费 name → 消费 COMMA → 消费 age
       返回 ["name", "age"]
  → 消费 FROM
  → 消费 users → table_name = "users"
  → 看到下一个 Token 是 WHERE → 调用 ParseExpression()
       → ParseComparison():
           消费 age → COLUMN_REF
           消费 >   → 操作符
           消费 25  → INT_LITERAL
           返回 BinaryOp(>, ColumnRef(age), IntLit(25))
  → 构造 SelectStmt { columns, table, where }
  → 返回 AST
```

### 6.4 关键代码

```cpp
class Parser {
public:
    explicit Parser(const std::string& source) : lexer_(source) {
        // 预读第一个 Token（叫"前瞻"lookahead）
        Advance();
    }

    std::shared_ptr<Stmt> Parse() {
        switch (current_token_.type) {
            case TokenType::SELECT: return ParseSelect();
            case TokenType::INSERT: return ParseInsert();
            case TokenType::CREATE: return ParseCreate();
            case TokenType::DROP:   return ParseDrop();
            default:
                throw std::runtime_error("未知的 SQL 语句");
        }
    }

private:
    Lexer lexer_;
    Token current_token_;  // 当前 Token（前瞻）

    void Advance() {
        current_token_ = lexer_.NextToken();
    }

    // 检查当前 Token 的类型，如果匹配则消费（前进到下一个）
    bool Match(TokenType type) {
        if (current_token_.type == type) {
            Advance();
            return true;
        }
        return false;
    }

    // 必须匹配，否则报错
    void Expect(TokenType type, const std::string& msg) {
        if (!Match(type)) {
            throw std::runtime_error("语法错误（第 " +
                std::to_string(current_token_.line) + " 行）: " + msg);
        }
    }

    std::shared_ptr<Stmt> ParseSelect() {
        auto stmt = std::make_shared<Stmt>();
        stmt->type = Stmt::Type::SELECT;

        Expect(TokenType::SELECT, "期望 SELECT");
        stmt->select_columns = ParseColumnList();
        Expect(TokenType::FROM, "期望 FROM");
        stmt->table_name = current_token_.value;
        Expect(TokenType::IDENTIFIER, "期望表名");

        // 可选的 WHERE 子句
        if (current_token_.type == TokenType::WHERE) {
            Advance();
            stmt->where_expr = ParseExpression();
        }

        return stmt;
    }

    std::shared_ptr<Expr> ParseExpression() {
        // 解析 AND/OR
        auto left = ParseComparison();

        while (current_token_.type == TokenType::AND ||
               current_token_.type == TokenType::OR) {
            std::string op = current_token_.value;
            Advance();
            auto right = ParseComparison();
            auto expr = std::make_shared<Expr>();
            expr->type = Expr::Type::BINARY_OP;
            expr->op = op;
            expr->left = left;
            expr->right = right;
            left = expr;
        }

        return left;
    }

    std::shared_ptr<Expr> ParseComparison() {
        auto left = ParsePrimary();

        if (IsComparisonOp(current_token_.type)) {
            std::string op = current_token_.value;
            Advance();
            auto right = ParsePrimary();
            auto expr = std::make_shared<Expr>();
            expr->type = Expr::Type::BINARY_OP;
            expr->op = op;
            expr->left = left;
            expr->right = right;
            return expr;
        }

        return left;
    }

    std::shared_ptr<Expr> ParsePrimary() {
        if (current_token_.type == TokenType::INT_LITERAL) {
            auto expr = std::make_shared<Expr>();
            expr->type = Expr::Type::INT_LITERAL;
            expr->int_val = std::stoll(current_token_.value);
            Advance();
            return expr;
        }
        if (current_token_.type == TokenType::STRING_LITERAL) {
            auto expr = std::make_shared<Expr>();
            expr->type = Expr::Type::STRING_LITERAL;
            expr->str_val = current_token_.value;
            Advance();
            return expr;
        }
        if (current_token_.type == TokenType::IDENTIFIER) {
            auto expr = std::make_shared<Expr>();
            expr->type = Expr::Type::COLUMN_REF;
            expr->str_val = current_token_.value;
            Advance();
            return expr;
        }
        throw std::runtime_error("意外的 Token: " + current_token_.value);
    }
};
```

**为什么叫"递归下降"？**
- `ParseExpression` 调用 `ParseComparison`
- `ParseComparison` 调用 `ParsePrimary`
- `ParseExpression` 又会递归调用自己（处理嵌套表达式）
- 函数调用栈的"下降"对应语法树从上到下的展开

**什么是"前瞻"（Lookahead）？**
- Parser 始终保持一个"当前 Token"
- 通过查看当前 Token 来决定走哪条语法分支
- 这是 LL(1) 解析器：只需向前看 1 个 Token

---

## 第七步：main.cpp 演示

**运行结果**：

```
=== Lesson 04: SQL 解析器 ===

--- 解析 CREATE TABLE ---
SQL: CREATE TABLE users (id INT, name TEXT, age INT)
Token 序列: CREATE TABLE users ( id COMMA name TEXT COMMA age INT )
AST: CreateTableStmt { table=users, columns=[(id,INT),(name,TEXT),(age,INT)] }

--- 解析 INSERT ---
SQL: INSERT INTO users VALUES (1, 'Alice', 30)
Token 序列: INSERT INTO users VALUES ( 1 COMMA Alice COMMA 30 )
AST: InsertStmt { table=users, values=[1, Alice, 30] }

--- 解析 SELECT ---
SQL: SELECT name, age FROM users WHERE age > 25
Token 序列: SELECT name COMMA age FROM users WHERE age > 25
AST: SelectStmt { columns=[name, age], table=users, where=(age > 25) }

--- 解析 DROP TABLE ---
SQL: DROP TABLE users
AST: DropTableStmt { table=users }
```

---

## 编译运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
cmake ..
make lesson04
./lesson04_sql_parser/lesson04
```

---

## 本课知识点总结

```
你学到了：

编译原理基础：
  ✓ 词法分析（Lexing）：字符串 → Token 序列
  ✓ 语法分析（Parsing）：Token 序列 → AST
  ✓ 递归下降：每个语法规则一个函数，通过递归处理嵌套
  ✓ 前瞻（Lookahead）：用当前 Token 决定走哪条分支

代码结构：
  ✓ Lexer：逐字符扫描，识别 Token 类型
  ✓ Parser：递归下降，构建 AST
  ✓ AST：树形结构，表示 SQL 的语义

设计思想：
  ✓ 关注点分离：Lexer 只管分词，Parser 只管结构
  ✓ 不可变性：Token 一旦生成就不变
  ✓ 错误恢复：遇到错误时报告位置和信息
```

---

## 思考题

1. **如何支持括号表达式，如 `WHERE (a > 1 AND b < 2) OR c = 3`？**
   <details><summary>提示</summary>在 ParsePrimary 中添加对 LPAREN 的处理：遇到 `(` 就调用 ParseExpression()，然后期望 `)`。</details>

2. **如何支持 NOT 运算符，如 `WHERE NOT (age > 25)`？**
   <details><summary>提示</summary>在 ParseExpression 和 ParseComparison 之间加一层 ParseUnary：检查是否有 NOT，有就创建 UnaryOp 节点。</details>

3. **为什么不直接在 Parser 中执行 SQL，而要先构建 AST？**
   <details><summary>提示</summary>(1) AST 可以做查询优化（如谓词下推）；(2) AST 可以序列化回 SQL；(3) 关注点分离——Parser 不应该知道如何执行。</details>

---

## 下一课预告

Lesson 05 将实现 **查询执行器**：遍历 AST，构建火山模型的算子树来执行 SQL。
