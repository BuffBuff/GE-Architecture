#pragma once

#include "ZipPacked.h"

#include <IResourceFile.h>

namespace GENA
{
	class ResourceZipFile : public IResourceFile
	{
	private:
		std::string filepath;
		ZipPacked pack;

	public:
		ResourceZipFile(std::string filepath);

		void open() override;
		uint64_t getRawResourceSize(ResId res) override;
		void getRawResource(ResId res, char* buffer) override;
		uint32_t getNumResources() const override;
		ResId getResourceId(uint32_t num) const override;
		std::string getResourceName(ResId res) const override;
		std::string getResourceType(ResId res) const override;
	};
}