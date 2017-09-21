#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

const int MAX_CHARS = 120;
const BinaryFile::Offset NULL_OFFSET = -1;

class DiskMultiMap
{
public:

	class Iterator
	{
	public:
		Iterator();
		// You may add additional constructors
		Iterator(BinaryFile* bfptr, BinaryFile::Offset begin);
		bool isValid() const;
		Iterator& operator++();
		MultiMapTuple operator*();

	private:
		// Your private member declarations will go here
		BinaryFile* m_bfptr;
		BinaryFile::Offset m_value;
	};

	DiskMultiMap();
	~DiskMultiMap();
	bool createNew(const std::string& filename, unsigned int numBuckets);
	bool openExisting(const std::string& filename);
	void close();
	bool insert(const std::string& key, const std::string& value, const std::string& context);
	Iterator search(const std::string& key);
	int erase(const std::string& key, const std::string& value, const std::string& context);

private:
	struct Header
	{
		int numBuckets;
		BinaryFile::Offset freeList;	// Head offset for "trash nodes"
	};

	struct Bucket
	{
		BinaryFile::Offset headOffset;
	};

	struct Node
	{
		char key[MAX_CHARS + 1];
		char value[MAX_CHARS + 1];
		char context[MAX_CHARS + 1];
		BinaryFile::Offset next;	// deleted node's next will point to next deleted node
	};

	int hashFunction(const string& key) const;
	BinaryFile::Offset getbucketOffset(const int bucketNum) const;
	Node createNode(const string& key, const string& value, const string& context, BinaryFile::Offset next);
	Bucket getBucket(const string& key);

	BinaryFile m_bf;
	Header m_header;
};

#endif // DISKMULTIMAP_H_

