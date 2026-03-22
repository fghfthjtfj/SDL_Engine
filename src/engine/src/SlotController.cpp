#include "PCH.h"
#include "SlotController.h"

SlotController::SlotController()
    : last_rendering_slot(INVALID_SLOT),
    prepared_head(0), prepared_tail(0),
    uploaded_head(0), uploaded_tail(0),
    rendering_head(0), rendering_tail(0)
{
    for (uint8_t i = 0; i < BUFFERING_LEVEL; ++i) {
        slots_data[i].frame_id = 0;
        slots_data[i].flags = 0;
        slots_data[i].fence = nullptr;
        slots_data[i].fence_time_ns = 0;
    }
}

// ====================== ВНУТРЕННИЕ GetComponent* (под mutex_) ======================

uint8_t SlotController::GetFreeSlotIndexUnsafe()
{
    // Начинаем поиск не с 0, а с сохранённой "точки старта"
    for (uint8_t offset = 0; offset < BUFFERING_LEVEL; ++offset) {
        uint8_t i = static_cast<uint8_t>(
            (next_free_slot_index + offset) % BUFFERING_LEVEL);

        uint8_t f = slots_data[i].flags;
        if ((f & SLOT_FLAG_HAS_PREPARED) == 0) {
            // Нашли "свободный" слот для новой подготовки
            next_free_slot_index = static_cast<uint8_t>((i + 1) % BUFFERING_LEVEL);
            return i;
        }
    }
    return INVALID_SLOT;
}


uint8_t SlotController::GetPreparedSlotUnsafe()
{
    for (;;) {
        // Пустая очередь?
        uint8_t tail = prepared_tail;
        uint8_t head = prepared_head;
        if (tail == head)
            return INVALID_SLOT;

        uint8_t slot = prepared_queue[tail];
        uint8_t f = slots_data[slot].flags;

        bool hasP = (f & SLOT_FLAG_HAS_PREPARED) != 0;
        bool hasU = (f & SLOT_FLAG_HAS_UPLOADED) != 0;
        bool hasR = (f & SLOT_FLAG_IS_RENDERING) != 0;
        uint8_t lr = last_rendering_slot;

        // Если пока слот лежал в очереди, P уже сняли — просто выкидываем из очереди
        if (!hasP) {
            uint8_t next = static_cast<uint8_t>((tail + 1) % QUEUE_CAPACITY);
            prepared_tail = next;
            continue;
        }

        // НЕЛЬЗЯ заливать поверх U/R или в lr — сохраняем порядок, не лезем дальше
        if (hasU || hasR ||
            (BUFFERING_LEVEL > 1 && slot == lr)) {
            return INVALID_SLOT;
        }

        // Этот слот годится. Теперь уже действительно "pop".
        uint8_t next = static_cast<uint8_t>((tail + 1) % QUEUE_CAPACITY);
        prepared_tail = next;
        return slot;
    }
}


uint8_t SlotController::GetUploadedSlot()
{
    // Ожидается, что mutex_ уже захвачен
    for (uint8_t tries = 0; tries < BUFFERING_LEVEL; ++tries) {
        uint8_t slot = PopUploaded();
        if (slot == INVALID_SLOT)
            return INVALID_SLOT;

        uint8_t f = slots_data[slot].flags;
        if ((f & SLOT_FLAG_HAS_UPLOADED) && !(f & SLOT_FLAG_IS_RENDERING)) {
            return slot;
        }
    }
    return INVALID_SLOT;
}

uint8_t SlotController::GetRenderableSlotUnsafe()
{
    // Сначала пытаемся взять новый U-слот
    uint8_t slot = GetUploadedSlot();
    if (slot != INVALID_SLOT)
        return slot;

    // Fallback — последний отрендеренный слот
    uint8_t lr = last_rendering_slot;
    if (lr == INVALID_SLOT)
        return INVALID_SLOT;

    uint8_t f = slots_data[lr].flags;
    if (f & SLOT_FLAG_IS_RENDERING)
        return INVALID_SLOT;

    return lr;
}

// ====================== ПУБЛИЧНЫЕ GetComponent* (неблокирующие) ====================

uint8_t SlotController::GetFreeSlotIndex()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return GetFreeSlotIndexUnsafe();
}

uint8_t SlotController::GetPreparedSlot()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return GetPreparedSlotUnsafe();
}

uint8_t SlotController::GetRenderableSlot()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return GetRenderableSlotUnsafe();
}

uint8_t SlotController::GetRendering()
{
    std::lock_guard<std::mutex> lock(mutex_);

    uint8_t tail = rendering_tail;
    uint8_t head = rendering_head;

    if (tail == head)
        return INVALID_SLOT;

    return rendering_queue[tail];
}

// ====================== БЛОКИРУЮЩИЕ Wait* ================================

uint8_t SlotController::WaitFreeSlotIndex()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
        uint8_t slot = GetFreeSlotIndexUnsafe();
        if (slot != INVALID_SLOT)
            return slot;

        // Ждём, пока кто-то не освободит слот (HandleFree / HandleUploaded / RemoveFlag(P))
        cv_free_.wait(lock);
    }
}

uint8_t SlotController::WaitPreparedSlot()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
        uint8_t slot = GetPreparedSlotUnsafe();
        if (slot != INVALID_SLOT)
            return slot;

        // Ждём появления подходящего PREPARED-слота
        cv_prepared_.wait(lock);
    }
}

// Только новый загруженный кадр, без fallback
uint8_t SlotController::GetUploadedSlotUnsafe()
{
    // как твой GetUploadedSlot, только без Pop/Push-вращений
    for (uint8_t tries = 0; tries < BUFFERING_LEVEL; ++tries) {
        uint8_t slot = PopUploaded();
        if (slot == INVALID_SLOT)
            return INVALID_SLOT;

        uint8_t f = slots_data[slot].flags;
        if ((f & SLOT_FLAG_HAS_UPLOADED) && !(f & SLOT_FLAG_IS_RENDERING)) {
            return slot;
        }
        // если U уже сняли — просто выкинули элемент очереди и идём дальше
    }
    return INVALID_SLOT;
}

// Чистый fallback по last_rendering_slot
uint8_t SlotController::GetRenderableFallbackUnsafe()
{
    uint8_t lr = last_rendering_slot;
    if (lr == INVALID_SLOT)
        return INVALID_SLOT;

    uint8_t f = slots_data[lr].flags;

    // Если слот всё ещё в процессе рендера – использовать нельзя
    if (f & SLOT_FLAG_IS_RENDERING)
        return INVALID_SLOT;

    // На U тут можно не смотреть: даже если U уже снят,
    // на GPU всё ещё лежит последний отрисованный кадр.
    return lr;
}

uint8_t SlotController::WaitRenderableSlot()
{
    std::unique_lock<std::mutex> lock(mutex_);

    // Мягкий бюджет ожидания для нового кадра
    constexpr auto SOFT_WAIT = std::chrono::milliseconds(2);
    // подправишь по вкусу: 1–3 мс обычно хватает

    for (;;) {
        // 1. Сначала пытаемся взять новый загруженный слот
        uint8_t slot = GetUploadedSlotUnsafe();
        if (slot != INVALID_SLOT)
            return slot;

        // 2. Пробуем взять fallback — последний отрендеренный слот
        uint8_t fb = GetRenderableFallbackUnsafe();

        if (fb != INVALID_SLOT) {
            // Есть что показать "по-старому", но сначала попробуем чуть-чуть подождать,
            // вдруг за это время появится новый U-слот.

            cv_renderable_.wait_for(lock, SOFT_WAIT);

            // после пробуждения (или таймаута / спурриозного wakeup) ещё раз проверяем U
            slot = GetUploadedSlotUnsafe();
            if (slot != INVALID_SLOT)
                return slot;

            // нового кадра так и нет — используем fallback и выходим
            return fb;
        }

        // 3. Нет ни нового U, ни fallback (например, самый первый кадр) —
        // просто ждём, пока кто-нибудь нас не разбудит.
        cv_renderable_.wait(lock);
        // после пробуждения цикл начинается заново
    }
}


bool SlotController::IsRenderingSlot(uint8_t slot)
{
    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL)
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
    return (slots_data[slot].flags & SLOT_FLAG_IS_RENDERING) != 0;
}


// ====================== Очереди P/U/R (под mutex_) =======================

bool SlotController::PushPrepared(uint8_t slot)
{
    // mutex_ уже захвачен
    uint8_t head = prepared_head;
    uint8_t tail = prepared_tail;
    uint8_t next = static_cast<uint8_t>((head + 1) % QUEUE_CAPACITY);

    if (next == tail) {
        SDL_Log("SlotController::PushPrepared: prepared queue overflow");
        return false;
    }

    prepared_queue[head] = slot;
    prepared_head = next;
    return true;
}

uint8_t SlotController::PopPrepared()
{
    uint8_t tail = prepared_tail;
    uint8_t head = prepared_head;

    if (tail == head){
		SDL_Log("SlotController::PopPrepared: prepared queue is empty");
        return INVALID_SLOT; // пусто
    }
    uint8_t slot = prepared_queue[tail];
    uint8_t next = static_cast<uint8_t>((tail + 1) % QUEUE_CAPACITY);
    prepared_tail = next;

    return slot;
}

bool SlotController::PushUploaded(uint8_t slot)
{
    uint8_t head = uploaded_head;
    uint8_t tail = uploaded_tail;
    uint8_t next = static_cast<uint8_t>((head + 1) % QUEUE_CAPACITY);

    if (next == tail) {
        SDL_Log("SlotController::PushUploaded: uploaded queue overflow");
        return false;
    }

    uploaded_queue[head] = slot;
    uploaded_head = next;
    return true;
}

uint8_t SlotController::PopUploaded()
{
    uint8_t tail = uploaded_tail;
    uint8_t head = uploaded_head;

    if (tail == head)
        return INVALID_SLOT; // пусто

    uint8_t slot = uploaded_queue[tail];
    uint8_t next = static_cast<uint8_t>((tail + 1) % QUEUE_CAPACITY);
    uploaded_tail = next;

    return slot;
}

void SlotController::PushRendering(uint8_t slot)
{
    uint8_t head = rendering_head;
    uint8_t tail = rendering_tail;
    uint8_t next = static_cast<uint8_t>((head + 1) % QUEUE_CAPACITY);

    if (next == tail) {
        SDL_Log("SlotController::PushRendering: rendering queue overflow");
        return;
    }

    rendering_queue[head] = slot;
    rendering_head = next;
}

void SlotController::PopRendering()
{
    uint8_t tail = rendering_tail;
    uint8_t head = rendering_head;

    if (tail == head)
        return; // пусто

    uint8_t next = static_cast<uint8_t>((tail + 1) % QUEUE_CAPACITY);
    rendering_tail = next;
}

// ====================== Прочие вспомогательные ===========================

uint8_t SlotController::GetLastRenderingSlotIndex()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return last_rendering_slot;
}

void SlotController::SetLastRenderedSlot(uint8_t slot)
{
    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    last_rendering_slot = slot;
}

// ====================== Handle* (через SetSlotState) =====================

void SlotController::HandleFree(uint8_t slot)
{
    std::lock_guard<std::mutex> lock(mutex_);
    slots_data[slot].flags = 0;

    // Любой сброс P даёт потенциально свободный слот для подготовки
    cv_free_.notify_all();
}

void SlotController::HandlePrepared(uint8_t slot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    uint8_t old = slots_data[slot].flags;
    uint8_t newf = static_cast<uint8_t>(old | SLOT_FLAG_HAS_PREPARED);
    slots_data[slot].flags = newf;

    if ((old & SLOT_FLAG_HAS_PREPARED) == 0) {
        if (PushPrepared(slot)) {
            cv_prepared_.notify_one();
        }
    }
}

void SlotController::HandleUploaded(uint8_t slot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    uint8_t oldf = slots_data[slot].flags;
    bool hadUploaded = (oldf & SLOT_FLAG_HAS_UPLOADED) != 0;

    uint8_t newf = oldf;
    newf &= static_cast<uint8_t>(~SLOT_FLAG_HAS_PREPARED); // P -> 0
    newf |= SLOT_FLAG_HAS_UPLOADED;                        // U -> 1
    slots_data[slot].flags = newf;

    // Новый U-слот, который ещё не рендерится, — в очередь рендера
    if (!hadUploaded && !(oldf & SLOT_FLAG_IS_RENDERING)) {
        if (PushUploaded(slot)) {
            cv_renderable_.notify_one();
        }
    }

    // Мы только что сняли P — для потоков подготовки может появиться свободный слот
    if (oldf & SLOT_FLAG_HAS_PREPARED) {
        cv_free_.notify_all();
    }
}

void SlotController::HandleRendering(uint8_t slot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Помечаем слот как участвующий в рендере
    slots_data[slot].flags =
        static_cast<uint8_t>(slots_data[slot].flags | SLOT_FLAG_IS_RENDERING);

    // Запоминаем последний отрендеренный слот для fallback в GetRenderableSlotUnsafe()
    if (slot != last_rendering_slot) {
        cv_prepared_.notify_all(); // или notify_one, см. ниже
    }
    last_rendering_slot = slot;

}


void SlotController::HandleRendered(uint8_t slot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    slots_data[slot].fence = nullptr;
    slots_data[slot].flags =
        static_cast<uint8_t>(slots_data[slot].flags & ~SLOT_FLAG_IS_RENDERING);
    slots_data[slot].flags =
        static_cast<uint8_t>(slots_data[slot].flags & ~SLOT_FLAG_HAS_UPLOADED);

    // Кадр показан, last_rendering_slot может использоваться как fallback для рендера
    cv_renderable_.notify_one();
    if (slot != last_rendering_slot) {
        cv_prepared_.notify_all(); // или notify_one, см. ниже
    }

}

void SlotController::SetSlotState(uint8_t slot, SlotState new_state)
{
    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL)
        return;

    switch (new_state) {
    case FREE:       HandleFree(slot);      break;
    case PREPARED:   HandlePrepared(slot);  break;
    case UPLOADED:   HandleUploaded(slot);  break;
    case RENDERING:  HandleRendering(slot); break;
    case RENDERED:   HandleRendered(slot);  break;

    case PREPARING:
    case UPLOADING:
        break;
    }
}

// ====================== Остальное =======================================

//void SlotController::RemoveFlag(uint8_t slot, uint8_t flag)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL)
//        return;
//
//    std::lock_guard<std::mutex> lock(mutex_);
//    uint8_t before = slots_data[slot].flags;
//    slots_data[slot].flags =
//        static_cast<uint8_t>(before & ~flag);
//
//    // Если внешний код снял PREPARED, тоже может появиться свободный слот
//    if ((flag & SLOT_FLAG_HAS_PREPARED) && (before & SLOT_FLAG_HAS_PREPARED)) {
//        cv_free_.notify_all();
//    }
//
//    // Если внешний код снял UPLOADED / IS_RENDERING, теоретически можно разбудить рендер
//    if ((flag & (SLOT_FLAG_HAS_UPLOADED | SLOT_FLAG_IS_RENDERING)) != 0) {
//        cv_renderable_.notify_one();
//    }
//}

void SlotController::SetSlotFence(uint8_t slot, SDL_GPUFence* fence)
{
    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    slots_data[slot].fence = fence;
}

void SlotController::SetSlotFence(uint8_t slot, std::chrono::steady_clock::time_point t)
{
    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL)
        return;

    uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        t.time_since_epoch()
    ).count();

    std::lock_guard<std::mutex> lock(mutex_);
    slots_data[slot].fence_time_ns = ns;
}

SlotController::~SlotController() = default;

void SlotController::DebugDump(const char* tag)
{
    std::lock_guard<std::mutex> lock(mutex_);

    SDL_Log("==== SlotController::DebugDump %s ====",
        tag ? tag : "");

    SDL_Log(" last_rendering_slot = %u", last_rendering_slot);
    SDL_Log(" prepared_head = %u, prepared_tail = %u", prepared_head, prepared_tail);
    SDL_Log(" uploaded_head = %u, uploaded_tail = %u", uploaded_head, uploaded_tail);
    SDL_Log(" rendering_head = %u, rendering_tail = %u", rendering_head, rendering_tail);

    for (uint8_t i = 0; i < BUFFERING_LEVEL; ++i) {
        const SlotData& sd = slots_data[i];
        uint8_t f = sd.flags;

        SDL_Log(
            " slot %u: flags=0x%02X [P=%u U=%u R=%u], fence=%p, fence_time_ns=%llu",
            i,
            f,
            (f & SLOT_FLAG_HAS_PREPARED) != 0,
            (f & SLOT_FLAG_HAS_UPLOADED) != 0,
            (f & SLOT_FLAG_IS_RENDERING) != 0,
            (void*)sd.fence,
            (unsigned long long)sd.fence_time_ns
        );
    }

    SDL_Log("======================================");
}

//#include "PCH.h"
//#include "SlotController.h"
//
//// Внутренний флаг резервирования — слот захвачен Sim, данные ещё не готовы
//static constexpr uint8_t SLOT_FLAG_RESERVED = 1u << 3;
//
//SlotController::SlotController()
//    : last_rendering_slot(INVALID_SLOT),
//    prepared_head(0), prepared_tail(0),
//    uploaded_head(0), uploaded_tail(0),
//    rendering_head(0), rendering_tail(0)
//{
//    for (auto& s : slots_data) {
//        s.frame_id = 0;
//        s.flags = 0;
//        s.fence = nullptr;
//        s.fence_time_ns = 0;
//    }
//}
//
//// ====================== helpers (под mutex_) ===========================
//
//// Свободен = нет флагов И не является текущим fallback-слотом.
//// Резервирует сразу под мьютексом, чтобы второй поток не взял тот же слот.
//uint8_t SlotController::GetFreeSlotIndexUnsafe()
//{
//    for (uint8_t i = 0; i < BUFFERING_LEVEL; ++i) {
//        if (slots_data[i].flags == 0 && i != last_rendering_slot) {
//            slots_data[i].flags = SLOT_FLAG_RESERVED;
//            return i;
//        }
//    }
//    return INVALID_SLOT;
//}
//
//uint8_t SlotController::GetPreparedSlotUnsafe()
//{
//    return PopPrepared();
//}
//
//uint8_t SlotController::GetRenderableSlotUnsafe()
//{
//    uint8_t slot = PopUploaded();
//    if (slot != INVALID_SLOT) return slot;
//    return last_rendering_slot;
//}
//
//// ====================== Публичные неблокирующие ========================
//
//uint8_t SlotController::GetFreeSlotIndex()
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    return GetFreeSlotIndexUnsafe();
//}
//
//uint8_t SlotController::GetPreparedSlot()
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    return GetPreparedSlotUnsafe();
//}
//
//uint8_t SlotController::GetRenderableSlot()
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    return GetRenderableSlotUnsafe();
//}
//
//uint8_t SlotController::GetRendering()
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    if (rendering_tail == rendering_head) return INVALID_SLOT;
//    return rendering_queue[rendering_tail];
//}
//
//// ====================== Блокирующие Wait* =============================
//
//uint8_t SlotController::WaitFreeSlotIndex()
//{
//    std::unique_lock<std::mutex> lock(mutex_);
//    for (;;) {
//        uint8_t slot = GetFreeSlotIndexUnsafe();
//        if (slot != INVALID_SLOT) return slot;
//        cv_free_.wait(lock);
//    }
//}
//
//uint8_t SlotController::WaitPreparedSlot()
//{
//    std::unique_lock<std::mutex> lock(mutex_);
//    for (;;) {
//        uint8_t slot = PopPrepared();
//        if (slot != INVALID_SLOT) return slot;
//        cv_prepared_.wait(lock);
//    }
//}
//
//uint8_t SlotController::GetUploadedSlotUnsafe()
//{
//    return PopUploaded();
//}
//
//uint8_t SlotController::GetRenderableFallbackUnsafe()
//{
//    return last_rendering_slot;
//}
//
//uint8_t SlotController::WaitRenderableSlot()
//{
//    constexpr auto SOFT_WAIT = std::chrono::milliseconds(2);
//    std::unique_lock<std::mutex> lock(mutex_);
//
//    for (;;) {
//        // Есть новый загруженный кадр — берём сразу
//        uint8_t slot = PopUploaded();
//        if (slot != INVALID_SLOT) return slot;
//
//        // Есть fallback — чуть подождём новый кадр, потом вернём его
//        if (last_rendering_slot != INVALID_SLOT) {
//            cv_renderable_.wait_for(lock, SOFT_WAIT);
//            slot = PopUploaded();
//            if (slot != INVALID_SLOT) return slot;
//            return last_rendering_slot; // состояние не меняем
//        }
//
//        // Совсем ничего нет (первый кадр) — ждём
//        cv_renderable_.wait(lock);
//    }
//}
//
//bool SlotController::IsRenderingSlot(uint8_t slot)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL) return false;
//    std::lock_guard<std::mutex> lock(mutex_);
//    return (slots_data[slot].flags & SLOT_FLAG_IS_RENDERING) != 0;
//}
//
//// ====================== Очереди P/U/R ================================
//
//bool SlotController::PushPrepared(uint8_t slot)
//{
//    uint8_t next = (prepared_head + 1) % QUEUE_CAPACITY;
//    if (next == prepared_tail) {
//        SDL_Log("SlotController::PushPrepared: prepared queue overflow");
//        return false;
//    }
//    prepared_queue[prepared_head] = slot;
//    prepared_head = next;
//    return true;
//}
//
//uint8_t SlotController::PopPrepared()
//{
//    if (prepared_tail == prepared_head) return INVALID_SLOT;
//    uint8_t slot = prepared_queue[prepared_tail];
//    prepared_tail = (prepared_tail + 1) % QUEUE_CAPACITY;
//    return slot;
//}
//
//bool SlotController::PushUploaded(uint8_t slot)
//{
//    uint8_t next = (uploaded_head + 1) % QUEUE_CAPACITY;
//    if (next == uploaded_tail) {
//        SDL_Log("SlotController::PushUploaded: uploaded queue overflow");
//        return false;
//    }
//    uploaded_queue[uploaded_head] = slot;
//    uploaded_head = next;
//    return true;
//}
//
//uint8_t SlotController::PopUploaded()
//{
//    if (uploaded_tail == uploaded_head) return INVALID_SLOT;
//    uint8_t slot = uploaded_queue[uploaded_tail];
//    uploaded_tail = (uploaded_tail + 1) % QUEUE_CAPACITY;
//    return slot;
//}
//
//void SlotController::PushRendering(uint8_t slot)
//{
//    uint8_t next = (rendering_head + 1) % QUEUE_CAPACITY;
//    if (next == rendering_tail) {
//        SDL_Log("SlotController::PushRendering: rendering queue overflow");
//        return;
//    }
//    rendering_queue[rendering_head] = slot;
//    rendering_head = next;
//}
//
//void SlotController::PopRendering()
//{
//    if (rendering_tail == rendering_head) return;
//    rendering_tail = (rendering_tail + 1) % QUEUE_CAPACITY;
//}
//
//// ====================== Handle* ======================================
//
//void SlotController::HandleFree(uint8_t slot)
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    slots_data[slot].flags = 0;
//    if (last_rendering_slot == slot)
//        last_rendering_slot = INVALID_SLOT;
//    cv_free_.notify_all();
//}
//
//void SlotController::HandlePrepared(uint8_t slot)
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    slots_data[slot].flags = SLOT_FLAG_HAS_PREPARED; // снимает RESERVED
//    PushPrepared(slot);
//    cv_prepared_.notify_one();
//}
//
//void SlotController::HandleUploaded(uint8_t slot)
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//
//    // Вытесняем все старые UPLOADED-слоты из очереди — они уже неактуальны.
//    // Освобождаем их сразу, чтобы Sim мог использовать.
//    for (;;) {
//        uint8_t old = PopUploaded();
//        if (old == INVALID_SLOT) break;
//        // Не трогаем last_rendering_slot — он управляется через HandleRendered
//        slots_data[old].flags = 0;
//        cv_free_.notify_one();
//    }
//
//    slots_data[slot].flags = SLOT_FLAG_HAS_UPLOADED;
//    PushUploaded(slot);
//    cv_renderable_.notify_one();
//}
//
//void SlotController::HandleRendering(uint8_t slot)
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//
//    uint8_t old_lr = last_rendering_slot;
//    if (old_lr != INVALID_SLOT && old_lr != slot) {
//        // Старый fallback больше не нужен.
//        // Обнуляем last_rendering_slot ДО notify, чтобы GetFreeSlotIndexUnsafe
//        // не споткнулся о "flags==0 но == last_rendering_slot".
//        last_rendering_slot = INVALID_SLOT;
//        slots_data[old_lr].flags = 0;
//        cv_free_.notify_one();
//    }
//
//    slots_data[slot].flags = SLOT_FLAG_IS_RENDERING;
//    // last_rendering_slot обновится в HandleRendered, когда кадр реально показан
//}
//
//void SlotController::HandleRendered(uint8_t slot)
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    slots_data[slot].fence = nullptr;
//    slots_data[slot].flags = 0;   // flags=0 + last_rendering_slot == slot → fallback
//    last_rendering_slot = slot;
//    cv_renderable_.notify_one();  // разбудить рендер (новый fallback доступен)
//    cv_free_.notify_one();        // если какой-то поток ждёт свободный слот
//}
//
//void SlotController::SetSlotState(uint8_t slot, SlotState new_state)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL) return;
//    switch (new_state) {
//    case FREE:      HandleFree(slot);      break;
//    case PREPARED:  HandlePrepared(slot);  break;
//    case UPLOADED:  HandleUploaded(slot);  break;
//    case RENDERING: HandleRendering(slot); break;
//    case RENDERED:  HandleRendered(slot);  break;
//    case PREPARING:
//    case UPLOADING: break;
//    }
//}
//
//// ====================== Остальное ====================================
//
//void SlotController::RemoveFlag(uint8_t slot, uint8_t flag)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL) return;
//    std::lock_guard<std::mutex> lock(mutex_);
//    uint8_t before = slots_data[slot].flags;
//    slots_data[slot].flags = static_cast<uint8_t>(before & ~flag);
//
//    if ((flag & SLOT_FLAG_HAS_PREPARED) && (before & SLOT_FLAG_HAS_PREPARED))
//        cv_free_.notify_all();
//    if (flag & (SLOT_FLAG_HAS_UPLOADED | SLOT_FLAG_IS_RENDERING))
//        cv_renderable_.notify_one();
//}
//
//uint8_t SlotController::GetLastRenderingSlotIndex()
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    return last_rendering_slot;
//}
//
//void SlotController::SetLastRenderedSlot(uint8_t slot)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL) return;
//    std::lock_guard<std::mutex> lock(mutex_);
//    last_rendering_slot = slot;
//}
//
//void SlotController::SetSlotFence(uint8_t slot, SDL_GPUFence* fence)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL) return;
//    std::lock_guard<std::mutex> lock(mutex_);
//    slots_data[slot].fence = fence;
//}
//
//void SlotController::SetSlotFence(uint8_t slot, std::chrono::steady_clock::time_point t)
//{
//    if (slot == INVALID_SLOT || slot >= BUFFERING_LEVEL) return;
//    uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
//        t.time_since_epoch()).count();
//    std::lock_guard<std::mutex> lock(mutex_);
//    slots_data[slot].fence_time_ns = ns;
//}
//
//SlotController::~SlotController() = default;
//
//void SlotController::DebugDump(const char* tag)
//{
//    std::lock_guard<std::mutex> lock(mutex_);
//    SDL_Log("==== SlotController::DebugDump [%s] ====", tag ? tag : "");
//    SDL_Log("  last_rendering_slot=%u  pq=[%u..%u]  uq=[%u..%u]",
//        last_rendering_slot, prepared_tail, prepared_head,
//        uploaded_tail, uploaded_head);
//
//    for (uint8_t i = 0; i < BUFFERING_LEVEL; ++i) {
//        const auto& s = slots_data[i];
//        uint8_t f = s.flags;
//        const char* state =
//            (f & SLOT_FLAG_IS_RENDERING) ? "RENDERING" :
//            (f & SLOT_FLAG_HAS_UPLOADED) ? "UPLOADED" :
//            (f & SLOT_FLAG_HAS_PREPARED) ? "PREPARED" :
//            (f & SLOT_FLAG_RESERVED) ? "PREPARING" :
//            (i == last_rendering_slot) ? "RENDERED" : "FREE";
//
//        SDL_Log("  slot %u: %-10s  flags=0x%02X  frame=%u  fence=%p",
//            i, state, f, s.frame_id, (void*)s.fence);
//    }
//    SDL_Log("==========================================");
//}