#include "btree.h"
#include <algorithm>

BTree::BTree() { root_ = new LeafNode(); }
BTree::~BTree() { DestroyTree(root_); }

void BTree::DestroyTree(BTreeNode* node) {
    if (!node) return;
    if (!node->is_leaf) {
        auto* internal = static_cast<InternalNode*>(node);
        for (auto* child : internal->children) DestroyTree(child);
    }
    delete node;
}

LeafNode* BTree::FindLeaf(int key) const {
    BTreeNode* curr = root_;
    while (!curr->is_leaf) {
        auto* internal = static_cast<InternalNode*>(curr);
        size_t i = 0;
        while (i < internal->keys.size() && key >= internal->keys[i]) i++;
        curr = internal->children[i];
    }
    return static_cast<LeafNode*>(curr);
}

std::optional<std::string> BTree::Search(int key) const {
    LeafNode* leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it != leaf->keys.end() && *it == key) {
        return leaf->values[it - leaf->keys.begin()];
    }
    return std::nullopt;
}

std::vector<std::pair<int, std::string>> BTree::RangeSearch(int low, int high) const {
    std::vector<std::pair<int, std::string>> result;
    LeafNode* leaf = FindLeaf(low);
    while (leaf) {
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] > high) return result;
            if (leaf->keys[i] >= low) result.push_back({leaf->keys[i], leaf->values[i]});
        }
        leaf = leaf->next;
    }
    return result;
}

void BTree::Insert(int key, const std::string& value) {
    LeafNode* leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it != leaf->keys.end() && *it == key) {
        leaf->values[it - leaf->keys.begin()] = value;
        return;
    }
    InsertIntoLeaf(leaf, key, value);
}

void BTree::InsertIntoLeaf(LeafNode* leaf, int key, const std::string& value) {
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    size_t idx = it - leaf->keys.begin();
    leaf->keys.insert(it, key);
    leaf->values.insert(leaf->values.begin() + idx, value);
    if ((int)leaf->keys.size() > BTREE_ORDER) SplitLeaf(leaf);
}

void BTree::SplitLeaf(LeafNode* leaf) {
    LeafNode* right = new LeafNode();
    int split = (BTREE_ORDER + 1) / 2;
    right->keys.assign(leaf->keys.begin() + split, leaf->keys.end());
    right->values.assign(leaf->values.begin() + split, leaf->values.end());
    leaf->keys.resize(split);
    leaf->values.resize(split);
    right->next = leaf->next;
    leaf->next = right;
    int push_up_key = right->keys[0];
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
    right->parent = leaf->parent;
    InsertIntoInternal(static_cast<InternalNode*>(leaf->parent), push_up_key, right);
}

void BTree::InsertIntoInternal(InternalNode* node, int key, BTreeNode* right_child) {
    auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
    size_t idx = it - node->keys.begin();
    node->keys.insert(it, key);
    node->children.insert(node->children.begin() + idx + 1, right_child);
    right_child->parent = node;
    if ((int)node->keys.size() > BTREE_ORDER) SplitInternal(node);
}

void BTree::SplitInternal(InternalNode* node) {
    int split = BTREE_ORDER / 2;
    int push_up_key = node->keys[split];
    InternalNode* right = new InternalNode();
    right->keys.assign(node->keys.begin() + split + 1, node->keys.end());
    right->children.assign(node->children.begin() + split + 1, node->children.end());
    for (auto* child : right->children) child->parent = right;
    node->keys.resize(split);
    node->children.resize(split + 1);
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

bool BTree::Delete(int key) {
    LeafNode* leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it == leaf->keys.end() || *it != key) return false;
    DeleteFromLeaf(leaf, key);
    return true;
}

void BTree::DeleteFromLeaf(LeafNode* leaf, int key) {
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    size_t idx = it - leaf->keys.begin();
    leaf->keys.erase(it);
    leaf->values.erase(leaf->values.begin() + idx);
    int min_keys = (BTREE_ORDER + 1) / 2 - 1;
    if (leaf == root_) return;
    if ((int)leaf->keys.size() < min_keys) FixAfterDelete(leaf);
}

void BTree::FixAfterDelete(BTreeNode* node) {
    if (node == root_) {
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
    size_t pos = 0;
    while (pos < parent->children.size() && parent->children[pos] != node) pos++;
    int min_keys = (BTREE_ORDER + 1) / 2 - 1;

    // 从左兄弟借
    if (pos > 0) {
        BTreeNode* left_sib = parent->children[pos - 1];
        if ((int)left_sib->keys.size() > min_keys) {
            if (node->is_leaf) {
                LeafNode* leaf = static_cast<LeafNode*>(node);
                LeafNode* left = static_cast<LeafNode*>(left_sib);
                leaf->keys.insert(leaf->keys.begin(), left->keys.back());
                leaf->values.insert(leaf->values.begin(), left->values.back());
                left->keys.pop_back(); left->values.pop_back();
                parent->keys[pos - 1] = leaf->keys[0];
            } else {
                InternalNode* curr = static_cast<InternalNode*>(node);
                InternalNode* left = static_cast<InternalNode*>(left_sib);
                curr->keys.insert(curr->keys.begin(), parent->keys[pos - 1]);
                curr->children.insert(curr->children.begin(), left->children.back());
                curr->children.front()->parent = curr;
                parent->keys[pos - 1] = left->keys.back();
                left->keys.pop_back(); left->children.pop_back();
            }
            return;
        }
    }
    // 从右兄弟借
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
    // 合并
    if (pos > 0) {
        BTreeNode* left_sib = parent->children[pos - 1];
        if (node->is_leaf) {
            LeafNode* leaf = static_cast<LeafNode*>(node);
            LeafNode* left = static_cast<LeafNode*>(left_sib);
            for (size_t i = 0; i < leaf->keys.size(); i++) {
                left->keys.push_back(leaf->keys[i]);
                left->values.push_back(leaf->values[i]);
            }
            left->next = leaf->next;
            parent->keys.erase(parent->keys.begin() + pos - 1);
            parent->children.erase(parent->children.begin() + pos);
            delete leaf;
        } else {
            InternalNode* curr = static_cast<InternalNode*>(node);
            InternalNode* left = static_cast<InternalNode*>(left_sib);
            left->keys.push_back(parent->keys[pos - 1]);
            for (auto k : curr->keys) left->keys.push_back(k);
            for (auto* c : curr->children) { c->parent = left; left->children.push_back(c); }
            parent->keys.erase(parent->keys.begin() + pos - 1);
            parent->children.erase(parent->children.begin() + pos);
            curr->children.clear(); delete curr;
        }
    } else {
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
            for (auto* c : right->children) { c->parent = curr; curr->children.push_back(c); }
            parent->keys.erase(parent->keys.begin() + pos);
            parent->children.erase(parent->children.begin() + pos + 1);
            right->children.clear(); delete right;
        }
    }
    if ((int)parent->keys.size() < (BTREE_ORDER + 1) / 2 - 1) FixAfterDelete(parent);
}
