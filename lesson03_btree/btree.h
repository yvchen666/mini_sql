#pragma once

#include <vector>
#include <string>
#include <optional>
#include <iostream>

// ============================================================
// btree.h — B+树索引实现
//
// 本实现使用内存中的 B+树（不与磁盘页面绑定），
// 目的是清晰展示 B+树的核心算法。
//
// 参数：
//   ORDER（阶数）= 每个节点最多存放的键数
//   最少键数 = ceil(ORDER/2) - 1（根节点除外）
//
// 键类型：int（整数）
// 值类型：std::string
// ============================================================

static constexpr int BTREE_ORDER = 4;  // 每个节点最多 4 个键

// ============================================================
// BTreeNode — B+树节点基类
// ============================================================
struct BTreeNode {
    bool is_leaf;                    // 是否为叶子节点
    std::vector<int> keys;           // 键数组
    BTreeNode* parent;               // 父节点指针（方便向上传播分裂）

    explicit BTreeNode(bool leaf) : is_leaf(leaf), parent(nullptr) {}
    virtual ~BTreeNode() = default;
};

// ============================================================
// InternalNode — 内部节点
// 存储 n 个键和 n+1 个子节点指针
// 键 keys[i] 满足：children[i] 中所有键 < keys[i] <= children[i+1] 中所有键
// ============================================================
struct InternalNode : public BTreeNode {
    std::vector<BTreeNode*> children;  // 子节点指针，数量 = keys.size() + 1

    InternalNode() : BTreeNode(false) {}
    ~InternalNode() override = default;
};

// ============================================================
// LeafNode — 叶子节点
// 存储实际的键值对，并通过 next 指针连接成链表（支持范围查询）
// ============================================================
struct LeafNode : public BTreeNode {
    std::vector<std::string> values;  // 与 keys 一一对应的值
    LeafNode* next;                   // 指向下一个叶子节点（范围查询用）

    LeafNode() : BTreeNode(true), next(nullptr) {}
    ~LeafNode() override = default;
};

// ============================================================
// BTree — B+树主类
// ============================================================
class BTree {
public:
    BTree();
    ~BTree();

    // 插入键值对
    // 如果键已存在，更新其值
    void Insert(int key, const std::string& value);

    // 查找键对应的值
    // 返回 std::nullopt 表示键不存在
    std::optional<std::string> Search(int key) const;

    // 删除键
    // 返回 true 表示删除成功，false 表示键不存在
    bool Delete(int key);

    // 范围查询：返回 [low, high] 范围内的所有键值对
    std::vector<std::pair<int, std::string>> RangeSearch(int low, int high) const;

    // 打印树结构（调试用）
    void Print() const;

    // 获取树的高度
    int Height() const;

private:
    BTreeNode* root_;  // 根节点

    // ---- 查找辅助 ----
    // 找到键应该在的叶子节点
    LeafNode* FindLeaf(int key) const;

    // ---- 插入辅助 ----
    // 向叶子节点插入键值对（可能触发分裂）
    void InsertIntoLeaf(LeafNode* leaf, int key, const std::string& value);

    // 分裂叶子节点
    void SplitLeaf(LeafNode* leaf);

    // 向内部节点插入键和右子节点（可能触发分裂）
    void InsertIntoInternal(InternalNode* node, int key, BTreeNode* right_child);

    // 分裂内部节点
    void SplitInternal(InternalNode* node);

    // ---- 删除辅助 ----
    void DeleteFromLeaf(LeafNode* leaf, int key);
    void FixAfterDelete(BTreeNode* node);

    // ---- 工具函数 ----
    void PrintNode(BTreeNode* node, int depth) const;
    void DestroyTree(BTreeNode* node);
    int HeightHelper(BTreeNode* node) const;
};
