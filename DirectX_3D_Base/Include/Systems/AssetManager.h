/*****************************************************************//**
 * @file	AssetManager.h
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

#ifndef ___ASSET_MANAGER_H___
#define ___ASSET_MANAGER_H___

// ===== インクルード =====
#include <iostream>
#include <string>
#include <map>
#include <stdexcept>
#include "Utility/CSVLoader.h"
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"
#include "Systems/XAudio2/SoundEffect.h"

namespace Asset
{
	/**
	 * @struct	AssetType
	 * @brief	アセットの種類を定義する列挙型。
	 */
	enum class AssetType
	{
		Model,
		Texture,
		Sound,
		Animation,
		Unknown	// 不明な種類
	};

	/**
	 * @struct	AssetInfo
	 * @brief	CSVから読み込んだアセットの情報を保持する構造体。
	 */
	struct AssetInfo
	{
		std::string assetID;	// CSVの1行目（ID）
		std::string filePath;	// CSVの3行目（ファイルパス）
		AssetType type = AssetType::Unknown;

		// ロード済みリソースをキャッシュするためのポインタ
		void* pResource = nullptr;
	};

	/**
	 * @class	AssetManager
	 * @brief	アセットのパス情報（CSV）を一元管理し、リソースのロード・解放を仲介するシングルトンクラス。
	 */
	class AssetManager
	{
	public:
		static AssetManager* s_instance;

		// --------------------------------------------------
		// CSVから読み込んだアセット情報のマップ
		// AssetID (string) -> AssetInfo
		// --------------------------------------------------
		std::map<std::string, AssetInfo> m_modelMap;
		std::map<std::string, AssetInfo> m_textureMap;
		std::map<std::string, AssetInfo> m_soundMap;
		std::map<std::string, AssetInfo> m_animationMap;

	private:
		// 外部からのインスタンス化を禁止
		AssetManager() = default;
		// コピー、ムーブを禁止
		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;
		AssetManager(AssetManager&&) = delete;
		AssetManager& operator=(AssetManager&&) = delete;

	public:
		/**
		 * [AssetManager & - GetInstance]
		 * @brief	シングルトンインスタンスを取得する。
		 * 
		 * @return	AssetManagerインスタンスへのポインタ
		 */
		static AssetManager& GetInstance()
		{
			if (s_instance == nullptr)
			{
				s_instance = new AssetManager();
			}
			return *s_instance;
		}

		/**
		 * [void - ReleaseInstance]
		 * @brief	シングルトンインスタンスを解放する。
		 */
		static void ReleaseInstance()
		{
			if (s_instance != nullptr)
			{
				delete s_instance;
				s_instance = nullptr;
			}
		}

		// ----------------------------------------
		// ヘルパー関数
		// ----------------------------------------
		bool LoadAssetListInternal(const std::string& csvPath, std::map<std::string, AssetInfo>& targetMap, AssetType type);

		// ----------------------------------------
		// CSV読み込み関連のインターフェース
		// ----------------------------------------
		bool LoadModelList(const std::string& csvPath);
		bool LoadTextureList(const std::string& csvPath);
		bool LoadSoundList(const std::string& csvPath);
		bool LoadAnimationList(const std::string& csvPath);

		// ----------------------------------------
		// アセットパス取得インターフェス
		// ----------------------------------------
		std::string GetModelPath(const std::string& assetID) const;
		std::string GetTexturePath(const std::string& assetID) const;
		std::string GetSoundPath(const std::string& assetID) const;
		std::string GetAnimationPath(const std::string& assetID) const;

		// ----------------------------------------
		// リソースロードインタフェース
		// ----------------------------------------
		// ロード関数は AssetInfo* を返し、内部でキャッシュの有無をチェックする
		AssetInfo* LoadModel(const std::string& assetID, float scale, Model::Flip flip);
		AssetInfo* LoadTexture(const std::string& assetID);
		AssetInfo* LoadSound(const std::string& assetID);

		// キャッシュ解放関数 (後のステップで実装)
		void UnloadAll();
	};
}

#endif // !___ASSET_MANAGER_H___