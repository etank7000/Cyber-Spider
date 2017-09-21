#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <string>
#include <vector>

const double LOAD_FACTOR = 0.70;

class IntelWeb
{
public:
	IntelWeb();
	~IntelWeb();
	bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
	bool openExisting(const std::string& filePrefix);
	void close();
	bool ingest(const std::string& telemetryFile);
	unsigned int crawl(const std::vector<std::string>& indicators,
		unsigned int minPrevalenceToBeGood,
		std::vector<std::string>& badEntitiesFound,
		std::vector<InteractionTuple>& badInteractions
		);
	bool purge(const std::string& entity);

private:
	// Your private member declarations will go here
	int getPrevelance(const string& entity, unsigned int minPrevalenceToBeGood);

	DiskMultiMap m_forward;
	DiskMultiMap m_reverse;
};

#endif // INTELWEB_H_


