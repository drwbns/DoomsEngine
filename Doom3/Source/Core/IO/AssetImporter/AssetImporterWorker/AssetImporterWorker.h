#pragma once

#include <Core.h>

#include <filesystem>
#include <atomic>

#include <Asset/eAssetType.h>

namespace dooms
{
	namespace asset
	{
		class Asset;
	}

	namespace assetImporter
	{
		class DOOM_API AssetImporterWorker : public DObject
		{
			DOBJECT_ABSTRACT_CLASS_BODY(AssetImporterWorker);
			DOBJECT_CLASS_BASE_CHAIN(DObject)

		protected:

			bool IsInitialized = false;
			static std::atomic<bool> IsInitializedStatic;

		public:

			AssetImporterWorker();
			AssetImporterWorker(const AssetImporterWorker&);
			AssetImporterWorker(AssetImporterWorker&&) noexcept;
			AssetImporterWorker& operator=(const AssetImporterWorker&);
			AssetImporterWorker& operator=(AssetImporterWorker&&) noexcept;
			virtual ~AssetImporterWorker();

			virtual bool ImportSpecificAsset(const std::filesystem::path& path, dooms::asset::Asset* asset) = 0;
			virtual dooms::asset::eAssetType GetEAssetType() const = 0;
		};
	}
}