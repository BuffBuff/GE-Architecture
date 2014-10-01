#pragma once

#include <cstdint>
#include <string>

namespace GENA
{
	class IResourceFile
	{
	public:
		typedef uint32_t ResId;

		virtual void open() = 0;
		virtual uint64_t getRawResourceSize(ResId res) = 0;
		virtual void getRawResource(ResId res, char* buffer) = 0;
		virtual uint32_t getNumResources() const = 0;
		virtual ResId getResourceId(uint32_t num) const = 0;
		virtual std::string getResourceName(ResId res) const = 0;
		virtual std::string getResourceType(ResId res) const = 0;
		virtual ~IResourceFile() {}
	};
}
