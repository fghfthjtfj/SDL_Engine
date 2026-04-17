#include "PCH.h"
#include "PIB_DataModule.h"
#include "ObjectManager.h"
#include "BufferManager.h"
#include "RenderCommandData.h"
#include "RenderManager.h"
#include "BatchBuilder.h"

PIB_DataModule::PIB_DataModule() {}

uint32_t PIB_DataModule::CalculatePIBSizes(BatchBuilder* bb, ObjectManager* om, PassManager* rm)
{
    if (!bb->CheckPIBNeedUpload()) {
        return 0;
    }
    uint32_t count = 0;

    for (RenderPassStep* rp : rm->GetOrderedRenderPasses())
    {
        for (const auto& [_, sb] : rp->shader_batches)
        {
            for (const auto& [_, ab] : sb.atlases_batches)
            {
                for (const auto& [_, tb] : ab.texture_batches)
                {
                    for (const auto& [_, mb] : tb.model_batches) {
                        count += mb.instanceCount;
                    }
                };
            }
        }
    }

    total_elements = count;
    dirty = false;

    bb->SetPIBNeedUpload(false);

    return total_elements * sizeof(uint32_t);
}

void PIB_DataModule::StorePIB(BufferManager* bm, PassManager* rm, UploadTask* task)
{
    std::vector<uint32_t> combined;
    for (RenderPassStep* rp : rm->GetOrderedRenderPasses())
        for (const auto& [_, sb] : rp->shader_batches)
            for (const auto& [_, ab] : sb.atlases_batches)
                for (const auto& [_, tb] : ab.texture_batches)
                    for (const auto& [_, mb] : tb.model_batches)
                        for (uint32_t id : mb.pib_sub_buffer)
                            combined.push_back(id);

    //SDL_Log("PIB combined size=%zu: ", combined.size());
    /*std::string s;
    for (auto v : combined) s += std::to_string(v) + " ";
    SDL_Log("%s", s.c_str());*/

    bm->UploadToPrePassTransferBuffer(task, safe_u32(combined.size()) * sizeof(uint32_t), combined.data());
}

uint32_t PIB_DataModule::CalcuteEntityToBatch(BatchBuilder* bb, ObjectManager* om, PassManager* pm)
{
    return CalculatePIBSizes(bb, om, pm);
}

void PIB_DataModule::StoreEntityToBatch(BufferManager* bm, PassManager* pm, UploadTask* task)
{
    uint32_t cmd_idx = 0;
    for (RenderPassStep* rp : pm->GetOrderedRenderPasses()){
        for (const auto& [_, sb] : rp->shader_batches){
            for (const auto& [_, ab] : sb.atlases_batches){
                for (const auto& [_, tb] : ab.texture_batches){
                    for (const auto& [_, mb] : tb.model_batches){
                        for (uint32_t id : mb.pib_sub_buffer) {
                            bm->UploadToPrePassTransferBuffer(task, sizeof(uint32_t), &cmd_idx);
                        }
                        cmd_idx++;
                    }
                }
            }
        }
    }
}
