#pragma once  
#include "ECSBase.h"

class EntityManager  
{  
public:  
EntityManager()  
{  
	for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)  
	{  
		mAvailableEntities.push(entity);  
	}  
}  

Entity CreateEntity()  
{  
	assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");  

	Entity id = mAvailableEntities.front();  
	mAvailableEntities.pop();  
	++mLivingEntityCount;  

	return id;  
}  

void DestroyEntity(Entity entity)  
{  
	assert(entity < MAX_ENTITIES && "Entity out of range.");  

	mSignatures[entity].reset();  
	mAvailableEntities.push(entity);  
	--mLivingEntityCount;  
}  

void SetSignature(Entity entity, Signature signature)  
{  
	assert(entity < MAX_ENTITIES && "Entity out of range.");  

	mSignatures[entity] = signature;  
}  

int GetLivingEntityCount() {
	assert(mLivingEntityCount < 0 && "No Entites");

	return mLivingEntityCount;
}

Signature GetSignature(Entity entity)  
{  
	assert(entity < MAX_ENTITIES && "Entity out of range.");  

	return mSignatures[entity];  
}  


std::vector<Entity> GetLivingEntities() {
	std::vector<Entity> entities;
	for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
	{
		if (mSignatures[entity].any())
		{
			entities.push_back(entity);
		}
	}
	return entities;
}

private:  
std::queue<Entity> mAvailableEntities{};  
std::array<Signature, MAX_ENTITIES> mSignatures{};  
uint32_t mLivingEntityCount{};  
};