#include "DiskMultiMap.h"
#include "MultiMapTuple.h"
#include "IntelWeb.h"
#include <string>
#include <functional>
#include <set>
#include <algorithm>
#include <vector>
#include <iostream>

void printVector(const vector<string>& t)
{
	for (int i = 0; i < t.size(); i++)
		cout << t[i] << endl;
	cout << "--------------" << endl;
}

void printInteractionTuples(const vector<InteractionTuple>& t)
{
	for (int i = 0; i < t.size(); i++)
		cout << t[i].context << " " << t[i].from << " " << t[i].to << endl;
	cout << "--------------" << endl;
}

void testDMM()
{
	DiskMultiMap map;
	map.createNew("mytable.dat", 1);
	map.insert("Hello", "bye", "di");
	map.insert("GOODBYE", "blue", "night");
	map.insert("Hello", "green", "day");
	map.insert("Hello", "green", "day");
	map.close();
	map.openExisting("mytable.dat");
	DiskMultiMap::Iterator it = map.search("Hello");
	MultiMapTuple tuple = *it;
	++it;
	++it;
	cout << (*it).value << endl;
}

void testIW()
{
	IntelWeb web;
	web.createNew("Hedrick", 7);
	web.ingest("telemetryFile.dat");
	vector<string> indicators = { "a.exe", "raremaliciouswebsite.com" };
	int minPrevelanceToBeGood = 10;
	vector<string> badEntitiesFound;
	vector<InteractionTuple> badInteractions;
	//web.purge("b.exe");
	web.crawl(indicators, minPrevelanceToBeGood, badEntitiesFound, badInteractions);
	printVector(badEntitiesFound);
	printInteractionTuples(badInteractions);
}



//bool operator<(const InteractionTuple& a, const InteractionTuple& b)
//{
//	if (a.context == b.context)
//	{
//		if (a.from == b.from)
//		{
//			return a.to < b.to;
//		}
//		return a.from < b.from;
//	}
//	return a.context < b.context;
//}
//
//bool operator==(const InteractionTuple& a, const InteractionTuple& b)
//{
//	return a.context == b.context && a.from == b.from && a.to == b.to;
//}

int main()
{
	testIW();
	//testDMM();
	
	
	
	//vector<string> str;
	//str.push_back("apple");
	//str.push_back("boy");
	//printVector(str);
	//str.push_back("cat");
	//printVector(str);
	//str.push_back("cat");
	//str.push_back("cat");
	//str.push_back("cat");
	//str.push_back("cat");
	//printVector(str);
	//InteractionTuple t4("boy", "c", "bab");
	//InteractionTuple t1("boy", "cat", "apple");
	//InteractionTuple t2("boy", "cat", "apple");
	//InteractionTuple t3("boy", "cat", "app");
	//vector<InteractionTuple> tuples;
	//tuples.push_back(t4);
	//tuples.push_back(t1);
	//tuples.push_back(t2);
	//tuples.push_back(t3);
	//printVector(tuples);
	//sort(tuples.begin(), tuples.end());
	//printVector(tuples);
}

