/*****************************************************************//**
 * @file	AssetManager.cpp
 * @brief	AssetManagerクラスの定義と骨子の作成
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/18	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Systems/AssetManager.h"

// シングルトンインスタンスの初期化
Asset::AssetManager* Asset::AssetManager::s_instance = nullptr;

namespace Asset
{
	// ----------------------------------------
	// ヘルパー関数
	// ----------------------------------------
	/**
	 * [bool - LoadAssetListInternal]
	 * @brief	汎用的なCSV読み込みとAssetInfoへの変換、マップへの格納を行うヘルパー関数。
	 * 
	 * @param	[in] csvPath 読み込むCSVファイルのパス
	 * @param	[in] targetMap 格納対象のマップ（m_modelMapなど）
	 * @param	[in] type アセットの種類（AssetType::Modelなど）
	 * @return	true.成功 false.失敗
	 * @note	（省略可）
	 */
	bool AssetManager::LoadAssetListInternal(
		const std::string& csvPath,
		std::map<std::string, AssetInfo>& targetMap,
		AssetType type
	)
	{
		std::cout << "Loading Asset List from: " << csvPath << std::endl;

		try
		{
			// CSVLoader::Loadを使ってデータを取得
			Utility::CSVLoader::Data csvData = Utility::CSVLoader::Load(csvPath);

			// ヘッダー行をスキップ（1行目）
			for (size_t i = 1; i < csvData.size(); ++i)
			{
				const auto& row = csvData[i];

				// 必要なカラム数（AssetID, AssetType, FilePath）が揃っているか確認
				if (row.size() < 3)
				{
					std::cerr << "Warning: Skipping row " << i + 1 << " in " << csvPath
						<< ". Insufficient columns." << std::endl;
					continue;
				}

				AssetInfo info;
				info.assetID = row[0];
				info.filePath = row[2]; // 3列目 (インデックス2) がファイルパス
				info.type = type;       // 呼び出し側で指定されたタイプを設定

				// IDが既に登録されていないか確認（重複防止）
				if (targetMap.count(info.assetID))
				{
					std::cerr << "Warning: Duplicate Asset ID found: " << info.assetID
						<< " in " << csvPath << ". Skipping." << std::endl;
					continue;
				}

				targetMap[info.assetID] = info;
			}
			std::cout << "Successfully loaded " << targetMap.size() << " assets of type "
				<< csvPath << " into manager." << std::endl;
			return true;
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << e.what() << std::endl;
			return false;
		}
		catch (...)
		{
			std::cerr << "An unknown error occurred while loading CSV: " << csvPath << std::endl;
			return false;
		}
	}

	// ----------------------------------------
	// CSV読み込み関連のインターフェース
	// ----------------------------------------
	bool AssetManager::LoadModelList(const std::string& csvPath)
	{
		return LoadAssetListInternal(csvPath, m_modelMap, AssetType::Model);
	}

	bool AssetManager::LoadTextureList(const std::string& csvPath)
	{
		return LoadAssetListInternal(csvPath, m_textureMap, AssetType::Texture);
	}

	bool AssetManager::LoadSoundList(const std::string& csvPath)
	{
		return LoadAssetListInternal(csvPath, m_soundMap, AssetType::Sound);
	}

	// ----------------------------------------
	// アセットパス取得インターフェス
	// ----------------------------------------
	std::string AssetManager::GetModelPath(const std::string& assetID) const
	{
		auto it = m_modelMap.find(assetID);
		if (it != m_modelMap.end())
		{
			return it->second.filePath;
		}
		std::cerr << "Error: Model Asset ID '" << assetID << "' not found." << std::endl;
		return "";
	}

	std::string AssetManager::GetTexturePath(const std::string& assetID) const
	{
		auto it = m_textureMap.find(assetID);
		if (it != m_textureMap.end())
		{
			return it->second.filePath;
		}
		std::cerr << "Error: Model Asset ID '" << assetID << "' not found." << std::endl;
		return "";
	}

	std::string AssetManager::GetSoundPath(const std::string& assetID) const
	{
		auto it = m_soundMap.find(assetID);
		if (it != m_soundMap.end())
		{
			return it->second.filePath;
		}
		std::cerr << "Error: Model Asset ID '" << assetID << "' not found." << std::endl;
		return "";
	}
	
	// ----------------------------------------
	// リソースロードインタフェース
	// ----------------------------------------
	AssetInfo* AssetManager::LoadModel(const std::string& assetID, float scale, Model::Flip flip)
	{
		auto it = m_modelMap.find(assetID);
		if (it == m_modelMap.end())
		{
			std::cerr << "Error: Model Asset ID '" << assetID << "' not registered in CSV." << std::endl;
			return nullptr;
		}

		AssetInfo& info = it->second;

		// 【キャッシュチェック】: 既にロード済みならそれを返す
		if (info.pResource != nullptr)
		{
			// std::cout << "Model '" << assetID << "' already loaded. Returning cached resource." << std::endl;
			return &info;
		}

		// 【新規ロード】: ファイルパスを取得し、ロードする
		const std::string& filePath = info.filePath;

		Model* newModel = new Model();

		if (newModel->Load(filePath.c_str(), scale, flip))
		{
			info.pResource = newModel; // 成功したらポインタをキャッシュ
			std::cout << "Model '" << assetID << "' loaded successfully from " << filePath << std::endl;
			return &info;
		}
		else
		{
			std::cerr << "Error: Failed to load model file: " << filePath << std::endl;
			delete newModel; // ロード失敗時はインスタンスを解放
			return nullptr;
		}
	}

	AssetInfo* AssetManager::LoadTexture(const std::string& assetID)
	{
		auto it = m_textureMap.find(assetID);
		if (it == m_textureMap.end())
		{
			std::cerr << "Error: Texture Asset ID '" << assetID << "' not registered in CSV." << std::endl;
			return nullptr;
		}

		AssetInfo& info = it->second;

		// 【キャッシュチェック】: 既にロード済みならそれを返す
		if (info.pResource != nullptr)
		{
			return &info;
		}

		// 【新規ロード】: ファイルパスを取得し、Textureをロードする
		const std::string& filePath = info.filePath;

		// Texture はヒープに確保し、pResource に格納する
		// Textureクラスが、リソース解放をデストラクタで担うことを前提とします。
		Texture* newTexture = new Texture();

		// Texture::Load() のシグネチャを仮定 (Texture.hの構造に依存)
		HRESULT hr = newTexture->Create(filePath.c_str());
		if (!hr)
		{
			info.pResource = newTexture; // 成功したらポインタをキャッシュ (void*)
			std::cout << "Texture '" << assetID << "' loaded successfully from " << filePath << std::endl;
			return &info;
		}
		else
		{
			std::cerr << "Error: Failed to load texture file: " << filePath << std::endl;
			delete newTexture; // ロード失敗時はインスタンスを解放
			return nullptr;
		}
	}

	AssetInfo* AssetManager::LoadSound(const std::string& assetID)
	{
		auto it = m_soundMap.find(assetID);
		if (it == m_soundMap.end())
		{
			std::cerr << "Error: Sound Asset ID '" << assetID << "' not registered in CSV." << std::endl;
			return nullptr;
		}

		AssetInfo& info = it->second;

		// 【キャッシュチェック】：既にロード済みならそれを返す
		if (info.pResource != nullptr)
		{
			return &info;
		}

		// 【新規ロード】: ファイルパスを取得し、SoundEffectをロードする
		const std::string& filePath = info.filePath;

		// SoundEffect はヒープに確保し、pResource に格納する
		Audio::SoundEffect* newSound = new Audio::SoundEffect();

 		if (newSound->Load(filePath)) // SoundEffect::Load()を呼び出す
		{
			info.pResource = newSound; // 成功したらポインタをキャッシュ (void*)
			std::cout << "Sound '" << assetID << "' loaded successfully from " << filePath << std::endl;
			return &info;
		}
		else
		{
			std::cerr << "Error: Failed to load sound file: " << filePath << std::endl;
			delete newSound; // ロード失敗時はインスタンスを解放
			return nullptr;
		}
	}

	// ----------------------------------------
	// キャッシュ解放関数
	// ----------------------------------------
	void AssetManager::UnloadAll()
	{
		std::cout << "AssetManager: Starting resource unloading..." << std::endl;

		// ----------------------------------------------------
		// ヘルパーテンプレートを使用して解放処理を共通化
		// ----------------------------------------------------
		auto unloadMap = [this](auto& assetMap, const std::string& typeName) {
			size_t releasedCount = 0;
			for (auto& pair : assetMap)
			{
				AssetInfo& info = pair.second;
				if (info.pResource != nullptr)
				{
					// モデルは new Model() で確保されているため delete
					if (info.type == AssetType::Model)
					{
						delete static_cast<Model*>(info.pResource);
						releasedCount++;
					}
					else if (info.type == AssetType::Texture)
					{
						delete static_cast<Texture*>(info.pResource);
						releasedCount++;
					}
					else if (info.type == AssetType::Sound)
					{
						delete static_cast<Audio::SoundEffect*>(info.pResource);
						releasedCount++;
					}
					else
					{
						// ★危険なelseブロックを削除し、Unknownの場合はログを出力するだけに留める★
						std::cerr << "Warning: Skipping unload for asset '" << info.assetID
							<< "' due to unknown AssetType." << std::endl;
					}

					info.pResource = nullptr;
				}
			}
			std::cout << "AssetManager: Unloaded " << releasedCount << " " << typeName << " resources." << std::endl;
			assetMap.clear(); // マップから全てのエントリを削除
			};

		// 実際には型安全を確保する必要がありますが、Modelについては new/delete が確実なため実装します。
		unloadMap(m_modelMap, "Model");
		unloadMap(m_textureMap, "Texture");
		unloadMap(m_soundMap, "Sound");

		std::cout << "AssetManager: Resource unloading completed." << std::endl;
	}
}