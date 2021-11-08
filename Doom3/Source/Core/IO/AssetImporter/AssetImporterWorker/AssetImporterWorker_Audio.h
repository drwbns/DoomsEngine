#pragma once

#include "AssetImporterWorker.h"


namespace dooms
{
	namespace assetImporter
	{
		class DOOM_API D_CLASS AssetImporterWorker_Audio : public AssetImporterWorker
		{
			DOBJECT_CLASS_BODY(AssetImporterWorker_Audio);
			DOBJECT_CLASS_BASE_CHAIN(AssetImporterWorker)

		public:

			virtual bool ImportSpecificAsset(const std::filesystem::path& path, dooms::asset::Asset* asset) override;
			virtual dooms::asset::eAssetType GetEAssetType() const final;

			static void InitializeAssetImporterWorkerStatic() {}
		};
		
	}
}