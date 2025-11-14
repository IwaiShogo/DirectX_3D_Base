#include"Systems/RenderSystem.h"
#include<iostream>
#include "ResourceManager.h"
#include <iostream>

using namespace std;


std::unordered_map<uint32_t, std::unique_ptr<Texture>> ResourceManager::m_textures;

void ResourceManager::Init()
{
}

void ResourceManager::Uninit()
{
	m_textures.clear();
	std::cout << "ResourceManager: ALL textures cleared." << std::endl;
}

bool ResourceManager::LoadTexture(uint32_t id, const std::string& filePath)
{
	if (m_textures.count(id))
	{
		std::cerr << "Error: Texture ID" << id << "is already loaded." << std::endl;
		return false;
	}

	auto pTexture = std::make_unique<Texture>();

	if (FAILED(pTexture->Create(filePath.c_str())))
	{
		std::cerr << "Error: Failed to load texture: " << filePath << std::endl;
		return false;//ƒ[ƒhŽ¸”s
	}

	m_textures[id] = std::move(pTexture);
	cout << "ResourceManager: Load texture (ID: " << id << ") from" << filePath << std::endl;
	return true;
}

Texture* ResourceManager::GetTexture(uint32_t id)
{
	auto it = m_textures.find(id);

	if (it != m_textures.end())
	{
		return it->second.get();
	}

	return nullptr;
}
