// Registry.h
#pragma once // <--- CRITICAL: Prevents "class type redefinition"

#include <unordered_map>
#include <map>
#include <typeindex>
#include <any>
#include <vector>

using EntityID = int;

template <typename T, typename U>
struct View {
    std::vector<EntityID> entities;
    std::map<EntityID, T>& poolT;
    std::map<EntityID, U>& poolU;
};

class Registry {
private:
    // The magical map that holds ALL components of ALL types
    std::unordered_map<std::type_index, std::any> pools;

public:
    // 0 reserved for global entity i.e server
    EntityID nextId = 1;
    Registry() = default; // Fixes "cannot define compiler-generated function"
    ~Registry() = default;

    // 1. Add Component (Works for Data AND Empty Flags)
    template <typename T, typename... Args>
    T& AddComponent(EntityID id, Args&&... args) {
        // Create pool if it doesn't exist
        if (pools.find(typeid(T)) == pools.end()) {
            pools[typeid(T)] = std::map<EntityID, T>();
        }

        auto& specificPool = std::any_cast<std::map<EntityID, T>&>(pools[typeid(T)]);

        // Construct the component (works for empty structs too!)
        specificPool[id] = T(std::forward<Args>(args)...);

        return specificPool[id];
    }

    // 2. Get Single Component
    template <typename T>
    T* GetComponent(EntityID id) {
        if (pools.find(typeid(T)) == pools.end()) return nullptr;

        auto& specificPool = std::any_cast<std::map<EntityID, T>&>(pools[typeid(T)]);

        auto it = specificPool.find(id);
        if (it == specificPool.end()) return nullptr;

        return &it->second;
    }
    template <typename T>
    void AddComponent(EntityID id, T componentInstance) {
        // 1. Check if pool exists
        if (pools.find(typeid(T)) == pools.end()) {
            pools[typeid(T)] = std::map<EntityID, T>();
        }

        // 2. Get the pool
        auto& specificPool = std::any_cast<std::map<EntityID, T>&>(pools[typeid(T)]);

        // 3. Insert the object you passed in
        specificPool[id] = componentInstance;
    }


    // 3. Remove Component
    template <typename T>
    void RemoveComponent(EntityID id) {
        if (pools.find(typeid(T)) != pools.end()) {
            auto& specificPool = std::any_cast<std::map<EntityID, T>&>(pools[typeid(T)]);
            specificPool.erase(id);
        }
    }

    // 4. Get All Components (Fixes 'GetAllComponents is not a member')
    template <typename T>
    std::map<EntityID, T>& GetAllComponents() {
        // Check if pool exists, if not, create an empty one so we don't crash
        if (pools.find(typeid(T)) == pools.end()) {
            pools[typeid(T)] = std::map<EntityID, T>();
        }
        return std::any_cast<std::map<EntityID, T>&>(pools[typeid(T)]);
    }

    template <typename T, typename U>
    View<T, U> view() {
        std::vector<EntityID> matches;
        auto& mapT = GetAllComponents<T>();
        auto& mapU = GetAllComponents<U>();

        if (mapT.size() < mapU.size()) {
            for (auto const& [id, component] : mapT) {
                if (mapU.count(id)) {
                    matches.push_back(id);
                }
            }
        }
        else {
            for (auto const& [id, component] : mapU) {
                if (mapT.count(id)) {
                    matches.push_back(id);
                }
            }
        }

        return { matches, mapT, mapU };
    }

    int CreateEntity() {
        return nextId++;
    }
};