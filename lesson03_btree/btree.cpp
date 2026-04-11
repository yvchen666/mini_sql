#include "btree.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

// ============================================================
// btree.cpp — B+树实现
// ============================================================

// ---- 构造/析构 ----

BTree::BTree() {
    // 初始时，根节点是一个空的叶子节点
    root_ = new LeafNode();
}

BTree::~BTree() {
    DestroyTree(root_);
}

void BTree::DestroyTree(BTreeNode* node) {
    if (!node) return;
    if (!node->is_leaf) {
        auto* internal = static_cast<InternalNode*>(node);
        for (auto* child : internal->children) {
            DestroyTree(child);
        }
    }
    delete node;
}

// ---- 查找 ----

LeafNode* BTree::FindLeaf(int key) const {
    BTreeNode* curr = root_;
    // 从根节点向下遍历，直到叶子节点
    while (!curr->is_leaf) {
        auto* internal = static_cast<InternalNode*>(curr);
        // 找到第一个 keys[i] > key 的位置
        // 则应该去 children[i]（即该键左侧的子树）
        size_t i = 0;
        while (i < internal->keys.size() && key >= internal->keys[i]) {
            i++;
        }
        curr = internal->children[i];
    }
    return static_cast<LeafNode*>(curr);
}

std::optional<std::string> BTree::Search(int key) const {
    LeafNode* leaf = FindLeaf(key);
    // 在叶子节点的 keys 数组中二分查找
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it != leaf->keys.end() && *it == key) {
        size_t idx = it - leaf->keys.begin();
        return leaf->values[idx];
    }
    return std::nullopt;
}

std::vector<std::pair<int, std::string>> BTree::RangeSearch(int low, int high) const {
    std::vector<std::pair<int, std::string>> result;
    LeafNode* leaf = FindLeaf(low);

    // 从找到的叶子节点开始，沿链表向右遍历
    while (leaf != nullptr) {
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] > high) {
                return result;
            }
            if (leaf->keys[i] >= low) {
                result.push_back({leaf->keys[i], leaf->values[i]});
            }
        }
        leaf = leaf->next;
    }
    return result;
}

// ---- 插入 ----

void BTree::Insert(int key, const std::string& value) {
    LeafNode* leaf = FindLeaf(key);

    // 检查键是否已存在（更新值）
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it != leaf->keys.end() && *it == key) {
        size_t idx = it - leaf->keys.begin();
        leaf->values[idx] = value;  // 更新
        return;
    }

    InsertIntoLeaf(leaf, key, value);
}

void BTree::InsertIntoLeaf(LeafNode* leaf, int key, const std::string& value) {
    // 找到插入位置（保持有序）
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    size_t idx = it - leaf->keys.begin();
    leaf->keys.insert(it, key);
    leaf->values.insert(leaf->values.begin() + idx, value);

    // 如果叶子节点超过容量，分裂
    if ((int)leaf->keys.size() > BTREE_ORDER) {
        SplitLeaf(leaf);
    }
}

void BTree::SplitLeaf(LeafNode* leaf) {
    // 创建新的右叶子节点
    LeafNode* right = new LeafNode();

    // 分裂点：左节点保留前半部分，右节点得到后半部分
    int split = (BTREE_ORDER + 1) / 2;

    // 将后半部分移到右节点
    right->keys.assign(leaf->keys.begin() + split, leaf->keys.end());
    right->values.assign(leaf->values.begin() + split, leaf->values.end());
    leaf->keys.resize(split);
    leaf->values.resize(split);

    // 维护叶子节点链表
    right->next = leaf->next;
    leaf->next = right;

    // 取右节点的第一个键上推到父节点
    int push_up_key = right->keys[0];

    // 如果当前叶子是根节点，创建新的根
    if (leaf == root_) {
        InternalNode* new_root = new InternalNode();
        new_root->keys.push_back(push_up_key);
        new_root->children.push_back(leaf);
        new_root->children.push_back(right);
        leaf->parent = new_root;
        right->parent = new_root;
        root_ = new_root;
        return;
    }

    // 否则，向父节点插入上推的键
    right->parent = leaf->parent;
    InsertIntoInternal(static_cast<InternalNode*>(leaf->parent), push_up_key, right);
}

void BTree::InsertIntoInternal(InternalNode* node, int key, BTreeNode* right_child) {
    // 找到插入位置
    auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
    size_t idx = it - node->keys.begin();
    node->keys.insert(it, key);
    node->children.insert(node->children.begin() + idx + 1, right_child);
    right_child->parent = node;

    // 如果内部节点超过容量，分裂
    if ((int)node->keys.size() > BTREE_ORDER) {
        SplitInternal(node);
    }
}

void BTree::SplitInternal(InternalNode* node) {
    int split = BTREE_ORDER / 2;
    int push_up_key = node->keys[split];  // 中间键上推

    // 创建新的右内部节点
    InternalNode* right = new InternalNode();
    right->keys.assign(node->keys.begin() + split + 1, node->keys.end());
    right->children.assign(node->children.begin() + split + 1, node->children.end());

    // 更新右侧子节点的父指针
    for (auto* child : right->children) {
        child->parent = right;
    }

    // 左节点保留前半部分
    node->keys.resize(split);
    node->children.resize(split + 1);

    // 如果当前节点是根节点，创建新根
    if (node == root_) {
        InternalNode* new_root = new InternalNode();
        new_root->keys.push_back(push_up_key);
        new_root->children.push_back(node);
        new_root->children.push_back(right);
        node->parent = new_root;
        right->parent = new_root;
        root_ = new_root;
        return;
    }

    right->parent = node->parent;
    InsertIntoInternal(static_cast<InternalNode*>(node->parent), push_up_key, right);
}

// ---- 删除 ----

bool BTree::Delete(int key) {
    LeafNode* leaf = FindLeaf(key);

    // 查找键是否存在
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it == leaf->keys.end() || *it != key) {
        return false;  // 键不存在
    }

    DeleteFromLeaf(leaf, key);
    return true;
}

void BTree::DeleteFromLeaf(LeafNode* leaf, int key) {
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    size_t idx = it - leaf->keys.begin();
    leaf->keys.erase(it);
    leaf->values.erase(leaf->values.begin() + idx);

    // 最小键数要求：ceil(ORDER/2) - 1
    int min_keys = (BTREE_ORDER + 1) / 2 - 1;

    // 如果是根节点，允许键数为 0
    if (leaf == root_) return;

    // 如果键数不足，需要从兄弟借键或合并
    if ((int)leaf->keys.size() < min_keys) {
        FixAfterDelete(leaf);
    }
}

void BTree::FixAfterDelete(BTreeNode* node) {
    if (node == root_) {
        // 根节点是内部节点且只剩一个子节点
        if (!node->is_leaf && static_cast<InternalNode*>(node)->keys.empty()) {
            BTreeNode* new_root = static_cast<InternalNode*>(node)->children[0];
            new_root->parent = nullptr;
            root_ = new_root;
            static_cast<InternalNode*>(node)->children.clear();
            delete node;
        }
        return;
    }

    InternalNode* parent = static_cast<InternalNode*>(node->parent);

    // 找到 node 在父节点 children 中的位置
    size_t pos = 0;
    while (pos < parent->children.size() && parent->children[pos] != node) {
        pos++;
    }

    int min_keys = (BTREE_ORDER + 1) / 2 - 1;

    // 尝试从左兄弟借键
    if (pos > 0) {
        BTreeNode* left_sib = parent->children[pos - 1];
        if ((int)left_sib->keys.size() > min_keys) {
            // 从左兄弟借最后一个键
            if (node->is_leaf) {
                LeafNode* leaf = static_cast<LeafNode*>(node);
                LeafNode* left = static_cast<LeafNode*>(left_sib);
                // 把左兄弟最后一个键移到当前节点头部
                leaf->keys.insert(leaf->keys.begin(), left->keys.back());
                leaf->values.insert(leaf->values.begin(), left->values.back());
                left->keys.pop_back();
                left->values.pop_back();
                // 更新父节点分隔键
                parent->keys[pos - 1] = leaf->keys[0];
            } else {
                InternalNode* curr = static_cast<InternalNode*>(node);
                InternalNode* left = static_cast<InternalNode*>(left_sib);
                // 父节点分隔键下移到当前节点
                curr->keys.insert(curr->keys.begin(), parent->keys[pos - 1]);
                curr->children.insert(curr->children.begin(), left->children.back());
                curr->children.front()->parent = curr;
                // 左兄弟最后一个键上推到父节点
                parent->keys[pos - 1] = left->keys.back();
                left->keys.pop_back();
                left->children.pop_back();
            }
            return;
        }
    }

    // 尝试从右兄弟借键
    if (pos < parent->children.size() - 1) {
        BTreeNode* right_sib = parent->children[pos + 1];
        if ((int)right_sib->keys.size() > min_keys) {
            if (node->is_leaf) {
                LeafNode* leaf = static_cast<LeafNode*>(node);
                LeafNode* right = static_cast<LeafNode*>(right_sib);
                leaf->keys.push_back(right->keys.front());
                leaf->values.push_back(right->values.front());
                right->keys.erase(right->keys.begin());
                right->values.erase(right->values.begin());
                parent->keys[pos] = right->keys[0];
            } else {
                InternalNode* curr = static_cast<InternalNode*>(node);
                InternalNode* right = static_cast<InternalNode*>(right_sib);
                curr->keys.push_back(parent->keys[pos]);
                curr->children.push_back(right->children.front());
                curr->children.back()->parent = curr;
                parent->keys[pos] = right->keys.front();
                right->keys.erase(right->keys.begin());
                right->children.erase(right->children.begin());
            }
            return;
        }
    }

    // 无法借键，需要合并
    // 选择合并方向：优先与左兄弟合并
    if (pos > 0) {
        // 与左兄弟合并（当前节点合并到左兄弟）
        BTreeNode* left_sib = parent->children[pos - 1];
        if (node->is_leaf) {
            LeafNode* leaf = static_cast<LeafNode*>(node);
            LeafNode* left = static_cast<LeafNode*>(left_sib);
            // 把当前叶子的所有键移到左兄弟
            for (size_t i = 0; i < leaf->keys.size(); i++) {
                left->keys.push_back(leaf->keys[i]);
                left->values.push_back(leaf->values[i]);
            }
            left->next = leaf->next;
            // 删除父节点中对应的分隔键和当前节点指针
            parent->keys.erase(parent->keys.begin() + pos - 1);
            parent->children.erase(parent->children.begin() + pos);
            delete leaf;
        } else {
            InternalNode* curr = static_cast<InternalNode*>(node);
            InternalNode* left = static_cast<InternalNode*>(left_sib);
            // 父节点分隔键下移
            left->keys.push_back(parent->keys[pos - 1]);
            for (auto k : curr->keys) left->keys.push_back(k);
            for (auto* c : curr->children) {
                c->parent = left;
                left->children.push_back(c);
            }
            parent->keys.erase(parent->keys.begin() + pos - 1);
            parent->children.erase(parent->children.begin() + pos);
            curr->children.clear();
            delete curr;
        }
    } else {
        // 与右兄弟合并（右兄弟合并到当前节点）
        BTreeNode* right_sib = parent->children[pos + 1];
        if (node->is_leaf) {
            LeafNode* leaf = static_cast<LeafNode*>(node);
            LeafNode* right = static_cast<LeafNode*>(right_sib);
            for (size_t i = 0; i < right->keys.size(); i++) {
                leaf->keys.push_back(right->keys[i]);
                leaf->values.push_back(right->values[i]);
            }
            leaf->next = right->next;
            parent->keys.erase(parent->keys.begin() + pos);
            parent->children.erase(parent->children.begin() + pos + 1);
            delete right;
        } else {
            InternalNode* curr = static_cast<InternalNode*>(node);
            InternalNode* right = static_cast<InternalNode*>(right_sib);
            curr->keys.push_back(parent->keys[pos]);
            for (auto k : right->keys) curr->keys.push_back(k);
            for (auto* c : right->children) {
                c->parent = curr;
                curr->children.push_back(c);
            }
            parent->keys.erase(parent->keys.begin() + pos);
            parent->children.erase(parent->children.begin() + pos + 1);
            right->children.clear();
            delete right;
        }
    }

    // 递归修复父节点
    if ((int)parent->keys.size() < (BTREE_ORDER + 1) / 2 - 1) {
        FixAfterDelete(parent);
    }
}

// ---- 打印 ----

void BTree::Print() const {
    std::cout << "B+树结构（ORDER=" << BTREE_ORDER << "）：\n";
    PrintNode(root_, 0);
}

void BTree::PrintNode(BTreeNode* node, int depth) const {
    if (!node) return;
    std::string indent(depth * 4, ' ');
    if (node->is_leaf) {
        LeafNode* leaf = static_cast<LeafNode*>(node);
        std::cout << indent << "[叶子] ";
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            std::cout << leaf->keys[i] << ":" << leaf->values[i];
            if (i + 1 < leaf->keys.size()) std::cout << ", ";
        }
        std::cout << "\n";
    } else {
        InternalNode* internal = static_cast<InternalNode*>(node);
        std::cout << indent << "[内部] 键: ";
        for (size_t i = 0; i < internal->keys.size(); i++) {
            std::cout << internal->keys[i];
            if (i + 1 < internal->keys.size()) std::cout << ", ";
        }
        std::cout << "\n";
        for (auto* child : internal->children) {
            PrintNode(child, depth + 1);
        }
    }
}

int BTree::Height() const {
    return HeightHelper(root_);
}

int BTree::HeightHelper(BTreeNode* node) const {
    if (!node || node->is_leaf) return 1;
    auto* internal = static_cast<InternalNode*>(node);
    return 1 + HeightHelper(internal->children[0]);
}
