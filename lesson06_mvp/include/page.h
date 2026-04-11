#pragma once
#include <cstdint>
#include <cstring>

using page_id_t = int32_t;
using frame_id_t = int32_t;

static constexpr page_id_t INVALID_PAGE_ID = -1;
static constexpr size_t PAGE_SIZE = 4096;

class Page {
public:
    Page() : page_id_(INVALID_PAGE_ID), is_dirty_(false), pin_count_(0) {
        memset(data_, 0, PAGE_SIZE);
    }
    page_id_t GetPageId() const { return page_id_; }
    void SetPageId(page_id_t id) { page_id_ = id; }
    char* GetData() { return data_; }
    const char* GetData() const { return data_; }
    bool IsDirty() const { return is_dirty_; }
    void SetDirty(bool d) { is_dirty_ = d; }
    int GetPinCount() const { return pin_count_; }
    void IncrPinCount() { pin_count_++; }
    void DecrPinCount() { if (pin_count_ > 0) pin_count_--; }
    void Reset() {
        page_id_ = INVALID_PAGE_ID;
        is_dirty_ = false;
        pin_count_ = 0;
        memset(data_, 0, PAGE_SIZE);
    }
private:
    page_id_t page_id_;
    bool      is_dirty_;
    int       pin_count_;
    char      data_[PAGE_SIZE];
};
