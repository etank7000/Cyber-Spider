#define _CRT_SECURE_NO_DEPRECATE
#include <string>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <sstream>

#include "IntelWeb.h"
#include "DiskMultiMap.h"
#include "InteractionTuple.h"
using namespace std;

// Comparison operators for InteractionTuple
bool operator<(const InteractionTuple& a, const InteractionTuple& b)
{
	if (a.context == b.context)
	{
		if (a.from == b.from)
		{
			return a.to < b.to;
		}
		return a.from < b.from;
	}
	return a.context < b.context;
}

bool operator==(const InteractionTuple& a, const InteractionTuple& b)
{
	return a.context == b.context && a.from == b.from && a.to == b.to;
}

IntelWeb::IntelWeb()
{

}

IntelWeb::~IntelWeb()
{
	close();
}

bool IntelWeb::createNew(const string& filePrefix, unsigned int maxDataItems)
{
	close();
	string fileName1 = filePrefix + "forward.dat";
	string fileName2 = filePrefix + "reverse.dat";
	int numBuckets = maxDataItems / LOAD_FACTOR;
	if (!m_forward.createNew(fileName1, numBuckets))
		return false;
	if (!m_reverse.createNew(fileName2, numBuckets))
	{
		close();
		return false;
	}
	return true;
}

bool IntelWeb::openExisting(const string& filePrefix)
{
	close();
	string fileName1 = filePrefix + "forward.dat";
	string fileName2 = filePrefix + "reverse.dat";
	if (!m_forward.openExisting(fileName1))
		return false;
	if (!m_reverse.openExisting(fileName2))
	{
		close();
		return false;
	}
	return true;
}

void IntelWeb::close()
{
	m_forward.close();
	m_reverse.close();
}

bool IntelWeb::ingest(const string& telemetryFile)
{
	ifstream inf(telemetryFile);
	if (!inf)
		return false;

	string line;
	while (getline(inf, line))
	{
		istringstream iss(line);
		string context;
		string first;
		string second;
		if (!(iss >> context >> first >> second))
			continue;
		m_forward.insert(first, second, context);
		m_reverse.insert(second, first, context);
	}
	return true;
}

unsigned int IntelWeb::crawl(const vector<string>& indicators,
	unsigned int minPrevalenceToBeGood,
	vector<string>& badEntitiesFound,
	vector<InteractionTuple>& badInteractions
	)
{

	// Assume sizes of vectors are much less than number of telemetry logs
	badEntitiesFound.clear();
	badInteractions.clear();

	unordered_set<string> malicious;
	unordered_map<string, int> prevalences;

	queue<string> bad;
	for (vector<string>::const_iterator it = indicators.begin(); it != indicators.end(); it++)
	{
		bad.push(*it);
		//cout << *it << endl;
	}
	while (!bad.empty())
	{
		//cout << "loop " << ++counter << endl;
		string frontKey = bad.front();
		//cout << "popped " << frontKey << endl;
		bad.pop();
		
		DiskMultiMap::Iterator iter = m_forward.search(frontKey);
		DiskMultiMap::Iterator iter2 = m_reverse.search(frontKey);
		if (iter.isValid() || iter2.isValid())
		{
			if (malicious.find(frontKey) == malicious.end())
			{
				badEntitiesFound.push_back(frontKey);
				malicious.insert(frontKey);
			}

			// Do the forward search
			while (iter.isValid())
			{
				string value = (*iter).value;
				int valuePrevelance;
				unordered_map<string, int>::const_iterator previter = prevalences.find(value);
				if (previter == prevalences.end())
				{
					valuePrevelance = getPrevelance(value, minPrevalenceToBeGood);
					prevalences[value] = valuePrevelance;
				}
				else
					valuePrevelance = previter->second;
				if (malicious.find(value) == malicious.end() && valuePrevelance < minPrevalenceToBeGood)
				{
					bad.push(value);
					//cout << "pushed " << value << endl;
					malicious.insert(value);
					badEntitiesFound.push_back(value);
				}
				++iter;
				//cout << "increment iter" << endl;
			}

			while (iter2.isValid())
			{
				//cout << "start of reverse search" << endl;
				string value = (*iter2).value;
				int valuePrevelance;
				unordered_map<string, int>::const_iterator previter = prevalences.find(value);
				if (previter == prevalences.end())
				{
					valuePrevelance = getPrevelance(value, minPrevalenceToBeGood);
					prevalences[value] = valuePrevelance;
				}
				else
					valuePrevelance = previter->second;
				if (malicious.find(value) == malicious.end() && valuePrevelance < minPrevalenceToBeGood)
				{
					bad.push(value);
					//cout << "pushed " << value << endl;
					malicious.insert(value);
					badEntitiesFound.push_back(value);
				}
				++iter2;
				//out << "increment iter2" << endl;
			}
		}
	}

	//cout << "done with crawling" << endl;
	for (vector<string>::iterator it = badEntitiesFound.begin(); it != badEntitiesFound.end(); it++)
	{
		DiskMultiMap::Iterator iter = m_forward.search(*it);
		while (iter.isValid())
		{
			InteractionTuple t((*iter).key, (*iter).value, (*iter).context);
			badInteractions.push_back(t);
			++iter;
		}
		DiskMultiMap::Iterator iter2 = m_reverse.search(*it);
		while (iter2.isValid())
		{
			InteractionTuple t((*iter2).value, (*iter2).key, (*iter2).context);
			badInteractions.push_back(t);
			++iter2;
		}
	}
	sort(badEntitiesFound.begin(), badEntitiesFound.end());
	sort(badInteractions.begin(), badInteractions.end());
	badInteractions.erase(unique(badInteractions.begin(), badInteractions.end()), badInteractions.end());

	return badEntitiesFound.size();
}

bool IntelWeb::purge(const string& entity)
{
	int numErased = 0;
	DiskMultiMap::Iterator iter = m_forward.search(entity);
	while (iter.isValid())
	{
		MultiMapTuple mmtuple = *iter;
		numErased += m_forward.erase(mmtuple.key, mmtuple.value, mmtuple.context);
		numErased += m_reverse.erase(mmtuple.value, mmtuple.key, mmtuple.context);		// By definition, the reverse associations exists
		iter = m_forward.search(entity);		// Iterator was invalidated by erase
	}

	iter = m_reverse.search(entity);

	while (iter.isValid())
	{
		MultiMapTuple mmtuple = *iter;
		numErased += m_reverse.erase(mmtuple.key, mmtuple.value, mmtuple.context);
		numErased += m_forward.erase(mmtuple.value, mmtuple.key, mmtuple.context);		// By definition, the reverse associations exists
		iter = m_reverse.search(entity);		// Iterator was invalidated by erase
	}
	return numErased > 0;
}

int IntelWeb::getPrevelance(const string& entity, unsigned int minPrevalenceToBeGood)
{
	int count = 0;
	DiskMultiMap::Iterator iter = m_forward.search(entity);
	while (iter.isValid())
	{
		count++;
		if (count == minPrevalenceToBeGood)
			return count;
		++iter;
	}
	iter = m_reverse.search(entity);
	while (iter.isValid())
	{
		count++;
		if (count == minPrevalenceToBeGood)
			return count;
		++iter;
	}
	return count;
}