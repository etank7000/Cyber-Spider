#define _CRT_SECURE_NO_DEPRECATE
#include <string>
#include <cstring>
#include <functional>
#include "DiskMultiMap.h"
#include "MultiMapTuple.h"
#include "BinaryFile.h"
#include <iostream>
using namespace std;


// DiskMultiMap::Iterator

DiskMultiMap::Iterator::Iterator()
{
	m_value = NULL_OFFSET;
}

DiskMultiMap::Iterator::Iterator(BinaryFile* bfptr, BinaryFile::Offset begin)
{
	m_bfptr = bfptr;
	m_value = begin;
}

bool DiskMultiMap::Iterator::isValid() const
{
	return m_value != NULL_OFFSET;
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()	// CHANGE LATER
{
	if (!isValid())
		return *this;
	Node node;
	m_bfptr->read(node, m_value);
	char key[MAX_CHARS + 1];
	strcpy(key, node.key);
	do
	{
		m_value = node.next;
		if (m_value != NULL_OFFSET)
		{
			m_bfptr->read(node, m_value);
			if (strcmp(key, node.key) == 0)
				break;
		}
	} while (m_value != NULL_OFFSET);		

	return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	MultiMapTuple m;
	if (isValid())
	{
		Node node;
		m_bfptr->read(node, m_value);
		m.key = node.key;
		m.value = node.value;
		m.context = node.context;
	}
	return m;
}

// DiskMultiMap 

DiskMultiMap::DiskMultiMap()
{

}

DiskMultiMap::~DiskMultiMap()
{
	m_bf.close();
}

bool DiskMultiMap::createNew(const string& filename, unsigned int numBuckets)
{
	m_bf.close();
	if (!m_bf.createNew(filename))
		return false;
	m_header.numBuckets = numBuckets;
	m_header.freeList = NULL_OFFSET;		// Nothing "deleted" in the beginning
	if (!m_bf.write(m_header, 0))	// Header is at the beginning of the disk file
		return false;
	// The "array" of buckets
	for (int i = 0; i < numBuckets; i++)
	{
		Bucket b;
		b.headOffset = NULL_OFFSET;	// Bucket starts out empty
		if (!m_bf.write(b, getbucketOffset(i)))
			return false;
	}
	//cout << m_bf.fileLength() << endl;
	return true;
}

bool DiskMultiMap::openExisting(const string& filename)
{
	if (m_bf.isOpen())
		m_bf.close();
	if (!m_bf.openExisting(filename))
		return false;
	m_bf.read(m_header, 0);
	return true;
}

void DiskMultiMap::close()
{
	m_bf.close();
}

bool DiskMultiMap::insert(const string& key, const string& value, const string& context)
{
	if (key.size() > MAX_CHARS || value.size() > MAX_CHARS || context.size() > MAX_CHARS)
		return false;
	int bucketNum = hashFunction(key);
	BinaryFile::Offset bucketOffset = getbucketOffset(bucketNum);
	Bucket b;
	m_bf.read(b, bucketOffset);
	BinaryFile::Offset toInsert;

	if (m_header.freeList != NULL_OFFSET)				// Reusing a deleted node's spot to insert
	{
		toInsert = m_header.freeList;		// First empty spot's offset
		Node toReplace;				
		m_bf.read(toReplace, toInsert);		// Get the "deleted node" at the empty spot's offset
		m_header.freeList = toReplace.next;
		m_bf.write(m_header, 0);
	}
	else
	{
		toInsert = m_bf.fileLength();		// Expand the disk size
	}
	Node n = createNode(key, value, context, b.headOffset);
	m_bf.write(n, toInsert);
	b.headOffset = toInsert;
	m_bf.write(b, bucketOffset);
	//cout << m_bf.fileLength() << endl;
	return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const string& key)
{
	int bucketNum = hashFunction(key);
	BinaryFile::Offset bucketOffset = getbucketOffset(bucketNum);
	Bucket b;
	m_bf.read(b, bucketOffset);

	Node node;
	BinaryFile::Offset curr = b.headOffset;
	while (curr != NULL_OFFSET)
	{
		m_bf.read(node, curr);
		if (strcmp(key.c_str(), node.key) == 0)
			return Iterator(&m_bf, curr);
		curr = node.next;
	}

	return Iterator();
}

int DiskMultiMap::erase(const string& key, const string& value, const string& context)
{
	int numErased = 0;

	int bucketNum = hashFunction(key);
	BinaryFile::Offset bucketOffset = getbucketOffset(bucketNum);
	Bucket b;
	m_bf.read(b, bucketOffset);

	BinaryFile::Offset curr = b.headOffset;
	BinaryFile::Offset prev = NULL_OFFSET;
	Node node;
	while (curr != NULL_OFFSET)
	{
		m_bf.read(node, curr);
		BinaryFile::Offset next = node.next;	// Current node's next offset
		if (strcmp(key.c_str(), node.key) == 0 && strcmp(value.c_str(), node.value) == 0 && strcmp(context.c_str(), node.context) == 0)
		{
			numErased++;
			node.next = m_header.freeList;
			m_header.freeList = curr;
			m_bf.write(node, curr);
			m_bf.write(m_header, 0);
			if (prev == NULL_OFFSET)
			{
				b.headOffset = next;
				m_bf.write(b, bucketOffset);
			}
			else
			{
				m_bf.read(node, prev);
				node.next = next;
				m_bf.write(node, prev);
			}
		}
		else
			prev = curr;
		curr = next;
	}
	//cout << m_bf.fileLength() << endl;
	return numErased;
}

int DiskMultiMap::hashFunction(const string& key) const
{
	hash<string> str_hash;
	return str_hash(key) % m_header.numBuckets;
}

BinaryFile::Offset DiskMultiMap::getbucketOffset(const int bucketNum) const
{
	return sizeof(m_header) + bucketNum * sizeof(Bucket);
}

DiskMultiMap::Node DiskMultiMap::createNode(const string& key, const string& value, const string& context, BinaryFile::Offset next)
{
	Node n;
	strcpy(n.key, key.c_str());
	strcpy(n.value, value.c_str());
	strcpy(n.context, context.c_str());
	n.next = next;
	return n;
}