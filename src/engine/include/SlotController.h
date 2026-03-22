#pragma once
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include "config.h"
#include <chrono>
#include <Utils.h>

enum SlotState : uint8_t { FREE, PREPARING, PREPARED, UPLOADING, UPLOADED, RENDERING, RENDERED };

static constexpr uint8_t QUEUE_CAPACITY = BUFFERING_LEVEL + 1;

// Битовые флаги для слота:
// P — есть подготовленные CPU-данные (готовы к upload)
// U — есть загруженные на GPU данные (готовы к рендеру / повторному рендеру)
// R — слот сейчас участвует в рендере
constexpr uint8_t SLOT_FLAG_HAS_PREPARED = 1u << 0;
constexpr uint8_t SLOT_FLAG_HAS_UPLOADED = 1u << 1;
constexpr uint8_t SLOT_FLAG_IS_RENDERING = 1u << 2;

struct SlotData {
    uint32_t frame_id = 0;
    uint8_t  flags = 0;                 // защищается mutex_ внутри SlotController
    SDL_GPUFence* fence = nullptr;
    uint64_t fence_time_ns = 0;
};

static constexpr uint8_t INVALID_SLOT = 0xFF;

class SlotController {
public:
    SlotController();

    // Неблокирующие варианты (как и были)
    uint8_t GetFreeSlotIndex();   // слот без P (CPU-данные можно готовить)
    uint8_t GetPreparedSlot();    // следующий слот с P (для upload-потока)
    uint8_t GetRenderableSlot();  // сначала U-очередь, потом fallback на last_rendering
    uint8_t GetRendering();

    // Новые блокирующие варианты, для перехода от sleep-поллинга к cv
    uint8_t WaitFreeSlotIndex();
    uint8_t WaitPreparedSlot();
    uint8_t GetUploadedSlotUnsafe();
    uint8_t GetRenderableFallbackUnsafe();
    uint8_t WaitRenderableSlot();

    bool IsRenderingSlot(uint8_t slot);

    SlotData* GetSlotsData() { return slots_data; }

    void SetLastRenderedSlot(uint8_t slot);
    void SetSlotState(uint8_t slot_index, SlotState new_state);

    void RemoveFlag(uint8_t slot, uint8_t flag);

    void SetSlotFence(uint8_t slot, SDL_GPUFence* fence);
    void SetSlotFence(uint8_t slot, std::chrono::steady_clock::time_point fence);

    ~SlotController();

    void DebugDump(const char* tag = nullptr);

private:
    SlotData slots_data[BUFFERING_LEVEL];

    uint8_t last_rendering_slot;

    uint8_t prepared_queue[QUEUE_CAPACITY];
    uint8_t prepared_head;
    uint8_t prepared_tail;

    uint8_t uploaded_queue[QUEUE_CAPACITY];
    uint8_t uploaded_head;
    uint8_t uploaded_tail;

    uint8_t rendering_queue[QUEUE_CAPACITY];
    uint8_t rendering_head;
    uint8_t rendering_tail;

    // Общая синхронизация
    std::mutex mutex_;
    std::condition_variable cv_free_;
    std::condition_variable cv_prepared_;
    std::condition_variable cv_renderable_;

    uint8_t next_free_slot_index = 0;

    // Обработчики состояний (теперь без inline, реализация в .cpp)
    void HandleFree(uint8_t slot);
    void HandlePrepared(uint8_t slot);
    void HandleUploaded(uint8_t slot);
    void HandleRendering(uint8_t slot);
    void HandleRendered(uint8_t slot);

    // Внутренние варианты GetComponent* под уже захваченным mutex_
    uint8_t GetFreeSlotIndexUnsafe();
    uint8_t GetPreparedSlotUnsafe();
    uint8_t GetRenderableSlotUnsafe();

    // Вспомогательные (как были, но теперь тоже под mutex_)
    uint8_t GetUploadedSlot();
    uint8_t GetLastRenderingSlotIndex();

    bool    PushPrepared(uint8_t slot);
    uint8_t PopPrepared();

    uint8_t PopUploaded();
    bool    PushUploaded(uint8_t slot);

    void PushRendering(uint8_t slot);
    void PopRendering();
};
