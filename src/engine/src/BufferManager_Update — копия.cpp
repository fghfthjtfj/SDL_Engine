#include "PCH.h"
#include "BufferManager.h"

UpdateInstruction* BufferManager::CreateUpdateInstruction(BufferData& buffer_data, std::function<void(BufferData&)> fn)
{
    UpdateInstruction instr;
    instr.buffer_data = &buffer_data;
    instr.updater = std::move(fn);
    //instr.size_fn = std::move(size_fn);
    update_instructions.push_back(std::move(instr));

    return &update_instructions.back();
}

UpdateInstruction* BufferManager::CreateUpdateInstruction(const std::string& name, std::function<void(BufferData&)> fn)
{
    BufferData* bd = (*this)[name];
    return CreateUpdateInstruction(*bd, std::move(fn));
}

void BufferManager::uploadToGPUBuffer(const std::string& name, const void* data, Uint32 size)
{
    auto it = buffers_data.find(name);
    if (it == buffers_data.end()) {
        SDL_Log("Buffer '%s' not found", name.c_str());
        return;
    }

    _uploadToGPUBufferInternal(it->second.get(), data, size);
}

void BufferManager::uploadToGPUBuffer(BufferData* buffer_data, const void* data, Uint32 size)
{
    if (!buffer_data) {
        SDL_Log("uploadToGPUBuffer: null buffer_data");
        return;
    }

    _uploadToGPUBufferInternal(buffer_data, data, size);
}

inline void BufferManager::_uploadToGPUBufferInternal(BufferData* buffer_data, const void* data, Uint32 size)
{
    uint8_t li = logic_index.load();

    UploadTask task;
    task.dst_buffer_data = buffer_data;

    // Расчёт смещений
    switch (buffer_data->type)
    {
    case BufferDataType::Static: {
        task.dst_offset = buffer_data->Static.current_dst_offset;
        buffer_data->Static.current_dst_offset += size;

        Uint32 end = task.dst_offset + size;
        if (end > buffer_data->Static.required_size)
            buffer_data->Static.required_size = end;
        break;
    }
    case BufferDataType::Dynamic: {
        task.dst_offset = buffer_data->Dynamic.current_dst_offset[li];
        buffer_data->Dynamic.current_dst_offset[li] += size;

        Uint32 end = task.dst_offset + size;
        if (end > buffer_data->Dynamic.required_size[li])
            buffer_data->Dynamic.required_size[li] = end;
        break;
    }
    default:
        SDL_Log("Unknown buffer type in _uploadToGPUBufferInternal");
        return;
    }

    // Добавляем данные и задачу
    task.tb_offset = upload_tasks_batch[li].current_tb_offset;
    task.size = size;

    auto& batch = upload_tasks_batch[li];

    batch.upload_data.insert(
        batch.upload_data.end(),
        (uint8_t*)data, (uint8_t*)data + size
    );
    batch.upload_tasks.push_back(task);
    batch.current_tb_offset += size;
}

void BufferManager::ExecuteUploadTasks(SDL_GPUCopyPass* cp, uint8_t idx)
{
    auto& batch = upload_tasks_batch[idx];
    if (batch.upload_tasks.empty())
        return;

    FillTransferBuffer(safe_u32(batch.upload_data.size()), batch.upload_data.data());

    for (const auto& task : batch.upload_tasks) {
        BufferData* buffer_data = task.dst_buffer_data;

        SDL_GPUBuffer* target_buffer = _GetGPUBufferForFrame(buffer_data, idx);

        SDL_GPUTransferBufferLocation src = { tb, task.tb_offset };
        SDL_GPUBufferRegion dstReg = { target_buffer, task.dst_offset, task.size };
        SDL_UploadToGPUBuffer(cp, &src, &dstReg, false);

        switch (buffer_data->type)
        {
        case BufferDataType::Static:
            buffer_data->Static.required_size = 0;
            break;
        case BufferDataType::Dynamic:
            buffer_data->Dynamic.required_size[idx] = 0;
            break;
        }
    }

    for (auto& [name, data] : buffers_data) {
        switch (data->type)
        {
        case BufferDataType::Static:
            data->Static.current_dst_offset = 0;
            break;
        case BufferDataType::Dynamic:
            data->Dynamic.current_dst_offset[idx] = 0;
            break;
        }
    }

    batch.upload_tasks.clear();
    batch.upload_data.clear();
    batch.current_tb_offset = 0;
}

void BufferManager::BeginWrite(BufferData* buffer)
{
    uint8_t li = logic_index.load();
    auto& p = pending[li];
    p.buffer = buffer;
    p.dst_offset = buffer->Dynamic.current_dst_offset[li];
    p.tb_offset = upload_tasks_batch[li].current_tb_offset;
    p.total_size = 0;
    p.active = true;
}

void BufferManager::AppendWrite(const void* data, Uint32 size)
{
    uint8_t li = logic_index.load();
    auto& p = pending[li];

    upload_tasks_batch[li].upload_data.insert(
        upload_tasks_batch[li].upload_data.end(),
        (uint8_t*)data, (uint8_t*)data + size
    );

    upload_tasks_batch[li].current_tb_offset += size;
    p.total_size += size;
}

void BufferManager::EndWrite()
{
    uint8_t li = logic_index.load();
    auto& p = pending[li];

    UploadTask task;
    task.dst_buffer_data = p.buffer;
    task.dst_offset = p.dst_offset;
    task.tb_offset = p.tb_offset;
    task.size = p.total_size;

    upload_tasks_batch[li].upload_tasks.push_back(task);
    Uint32 end = p.dst_offset + p.total_size;

    // обновляем buffer_data offsets
    BufferData* data = p.buffer;

    switch (data->type)
    {
    case BufferDataType::Static:
        data->Static.current_dst_offset = end;
        data->Static.required_size = end;
        break;
    case BufferDataType::Dynamic:
        data->Dynamic.current_dst_offset[li] = end;
        data->Dynamic.required_size[li] = end;
        break;
    default:
        SDL_Log("Unknown buffer type in EndWrite");
        return;
    };

    p.active = false;
}

//UpdateInstruction* BufferManager::CreateUpdateInstruction(const std::string& name, std::function<void(BufferData&)> fn, std::function<Uint32(BufferData&)> size_fn)
//{
//    BufferData* bd = (*this)[name];
//    return CreateUpdateInstruction(*bd, std::move(fn), std::move(size_fn));
//}

void BufferManager::ExecuteUpdateInstructions()
{
    for (auto& instr : update_instructions) {
        if (instr.updater && instr.buffer_data) {
            instr.updater(*(instr.buffer_data));
        }
        else {
            SDL_Log("Invalid update instruction in ExecuteUpdateInstructions");
        }
    }
}

void BufferManager::FinilizeUploadTasks()
{
    uint8_t li = logic_index.load();
    auto& batch = upload_tasks_batch[li];

    for (const auto& task : batch.upload_tasks) {
        BufferData* buffer_data = task.dst_buffer_data;
        Uint32 required = 0;

        switch (buffer_data->type)
        {
        case BufferDataType::Static:
            required = buffer_data->Static.required_size;
            break;
        case BufferDataType::Dynamic:
            required = buffer_data->Dynamic.required_size[li];
            break;
        default:
            continue;
        }


        EnsureBufferCapacity(buffer_data, required, li);
    }
    Uint32 total_required = safe_u32(batch.upload_data.size());
    EnsureTransferBufferCapacity(total_required);
}

