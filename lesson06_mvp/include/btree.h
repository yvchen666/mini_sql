#pragma once
#include <vector>
#include <string>
#include <optional>

static constexpr int BTREE_ORDER = 4;

struct BTreeNode {
    bool is_leaf;
    std::vector<int> keys;
    BTreeNode* parent;
    explicit BTreeNode(bool leaf) : is_leaf(leaf), parent(nullptr) {}
    virtual ~BTreeNode() = default;
};

struct InternalNode : public BTreeNode {
    std::vector<BTreeNode*> children;
    InternalNode() : BTreeNode(false) {}
};

struct LeafNode : public BTreeNode {
    std::vector<std::string> values;
    LeafNode* next;
    LeafNode() : BTreeNode(true), next(nullptr) {}
};

class BTree {
public:
    BTree();
    ~BTree();
    void Insert(int key, const std::string& value);
    std::optional<std::string> Search(int key) const;
    bool Delete(int key);
    std::vector<std::pair<int, std::string>> RangeSearch(int low, int high) const;
private:
    BTreeNode* root_;
    LeafNode* FindLeaf(int key) const;
    void InsertIntoLeaf(LeafNode* leaf, int key, const std::string& value);
    void SplitLeaf(LeafNode* leaf);
    void InsertIntoInternal(InternalNode* node, int key, BTreeNode* right_child);
    void SplitInternal(InternalNode* node);
    void DeleteFromLeaf(LeafNode* leaf, int key);
    void FixAfterDelete(BTreeNode* node);
    void DestroyTree(BTreeNode* node);
};
