#pragma once
#include "BaseComponents.h"
#include <map>
#include <set>
#include <string>
#include "BufferManager.h"
#include "SceneData.h"
#include "MaterialManager.h"

struct PassBatchesResult;

template<typename SoA>
struct SoAElement {
    SoA* soa = nullptr;
    size_t index = 0;

    SoA& container() { return *soa; }
    const SoA& container() const { return *soa; }
    size_t i() const { return index; }

    operator SoA& () { return *soa; }
    operator const SoA& () const { return *soa; }
};

template<typename T>
using foreach_arg_t = std::conditional_t<is_soa<T>::value, SoAElement<T>, T&>;

struct BatchKey
{
    ModelData* model = nullptr;
    TextureData* albedo = nullptr;
    TextureData* normal = nullptr;

    bool operator==(const BatchKey& o) const noexcept
    {
        return model == o.model && albedo == o.albedo && normal == o.normal;
    }
};

// Без отдельного hasher-struct: используем функцию и указатель на неё.
static size_t HashBatchKey(const BatchKey& k) noexcept
{
    auto h = std::hash<void*>{};
    size_t r = h(k.model);
    // классический combine
    r ^= (h(k.albedo) + 0x9e3779b97f4a7c15ULL + (r << 6) + (r >> 2));
    r ^= (h(k.normal) + 0x9e3779b97f4a7c15ULL + (r << 6) + (r >> 2));
    return r;
}

class ObjectManager {
public:
    template<typename... Components>
    Entity create(const std::string& scene_name, Components&&... comps);

    template<typename ...Ts, typename Fn>
    void ForEachArchetype(SceneData* scene, Fn&& fn);

    template<typename ...Ts, typename Fn>
    void ForEach(SceneData* scene, Fn&& fn);

    template<typename T>
    foreach_arg_t<T> Get(SceneData* scene, Entity e);

    template<typename Component>
    bool Has(SceneData* scene, Entity e) const;

    SceneData* CreateScene(const std::string& name);
	void SetSceneState(const std::string& scene_name, bool is_active);
    SceneData* GetActiveScene();
    PassBatchesResult BuildPassBatches(SceneData* scene, const RenderPassName& rp_name, ShaderProgram* sp);
    bool CheckDirty() { return dirty; };
    //PassBatchesResult BuildPassBatches(SceneData* scene, RenderPassType pass);

    SceneData* operator[](const std::string& name);


private:
    template<typename... Components>
    void add_components(Archetype& arch, Components&&... comps);

    std::unordered_map<std::string, std::unique_ptr<SceneData>> scenes_data;
    bool dirty = true;
};

#include "ObjectManager.inl"