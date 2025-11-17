/*****************************************************************//**
 * @file	CSVLoader.h
 * @brief	CSVファイル読み込み機能の定義
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

#ifndef ___CSV_LOADER_H___
#define ___CSV_LOADER_H___

// ===== インクルード =====
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>

namespace Utility
{
	/**
	 * @class	CSVLoader
	 * @brief	CSVファイルからデータを読み込み、行と列に分解ユーティリティクラス。
	 */
	class CSVLoader
	{
	public:
		using Row = std::vector<std::string>;
		using Data = std::vector<Row>;

		/**
		 * [Data - Load]
		 * @brief	指定されたファイルパスからCSVデータを読み込む。
		 * 
		 * @param	[in] filePath 読み込むCSVのファイルパス
		 * @return	CSVデータ（行のベクター、各行は文字列のベクター）
		 */
		static Data Load(const std::string& filePath)
		{
			Data data;
			std::ifstream file(filePath);

			if (!file.is_open())
			{
				// エラー処理：ファイルが開けなかった場合
				std::string errorMessage = "Error: CSVファイルが読み込めませんでした。: " + filePath;
				throw std::runtime_error(errorMessage);
			}

			std::string line;
			while (std::getline(file, line))
			{
				if (data.empty() && line.size() > 2 && line.substr(0, 3) == "\xEF\xBB\xBF")
				{
					line.erase(0, 3);
				}

				if (line.empty()) continue;

				data.push_back(ParseLine(line));
			}

			return data;
		}

	private:
		/**
		 * [Row - ParseLine]
		 * @brief	一行の文字列をカンマで分割し、フィールドのベクターに変換する。
		 * 
		 * @param	[in] line 処理する一行の文字列
		 * @return	分割されたフィールドのベクター
		 */
		static Row ParseLine(const std::string& line)
		{
			Row row;
			std::stringstream ss(line);
			std::string field;

			// シンプルなカンマ区切り処理（フィールド内のカンマやクォートには非対応）
			// Excel出力のシンプルなCSVを前提
			while (std::getline(ss, field, ','))
			{
				row.push_back(field);
			}

			// 末尾にカンマがある場合、からのフィールドを追加
			if (line.back() == ',')
			{
				row.push_back("");
			}

			return row;
		}
	};
}

#endif // !___CSV_LOADER_H___