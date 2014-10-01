#pragma once

#include "BinPacked.h"

#include <IResourceFile.h>

namespace GENA
{
	class ResourceBinFile : public IResourceFile
	{
	private:
		std::string filepath;
		BinPacked pack;

	public:
		ResourceBinFile(std::string filepath);

		void open() override;
		uint64_t getRawResourceSize(ResId res) override;
		void getRawResource(ResId res, char* buffer) override;
		uint32_t getNumResources() const override;
		ResId getResourceId(uint32_t num) const override;
		std::string getResourceName(ResId res) const override;
		std::string getResourceType(ResId res) const override;
	};
}
