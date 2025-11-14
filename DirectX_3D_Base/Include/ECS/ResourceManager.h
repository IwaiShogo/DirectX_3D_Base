#ifndef  ___RESOURCE_MANAGER_H___
#define ___RESOURCE_MANAGER_H___

#include<string>
#include <unordered_map>
#include<memory>
#include<cstdint>
#include"Systems/DirectX/Texture.h"

class ResourceManager

{
private:
	static std::unordered_map < uint32_t, std::unique_ptr<Texture>> m_textures;


public:

	ResourceManager() = delete;

	static void Init();

	static void Uninit();

	static bool LoadTexture(uint32_t id, const std::string& filePath);

	static Texture* GetTexture(uint32_t id);
private:

};

#endif //!___RESOURCE_MANAGER_H___