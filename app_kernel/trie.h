#ifndef TRIE_H_
#define TRIE_H_

#include <iostream>
#include <unordered_map>
#include <vector>
#include <cassert>
#include "../app_kernel/util/rwlock.h"

using namespace std;

// A Trie node
template <typename T>
class TrieNode
{

public:
	// true when node is a leaf node
	bool isLeaf;
	rwlock leaflock;

	// each node stores a map to its child nodes
	unordered_map<T, TrieNode<T> *> map;
	// map lock
	rwlock lock;
};

template <typename T>
class Trie
{
	TrieNode<T> *root;

public:
	// constructor
	Trie()
	{
		root = getNewTrieNode();
	}
	// destructor
	~Trie()
	{
		del_node(root);
	}

	// Function that returns a new Trie node
	TrieNode<T> *getNewTrieNode()
	{
		TrieNode<T> *node = new TrieNode<T>;
		node->isLeaf = false;
		return node;
	}

	// Iterative function to insert a string in Trie.
	bool insert(const vector<T> &seq)
	{
		// start from root node
		TrieNode<T> *curr = root;

		for (int i = 0; i < seq.size(); i++)
		{
			T key = seq[i];
			// lock.rdlock();
			// create a new node if path doesn't exists
			curr->lock.rdlock();
			TrieNode<T> *pa = curr; // parent node
			auto it = curr->map.find(key);
			if (it == curr->map.end())
			{
				// cout << i << endl; /////////////////////////////////////
				curr->lock.unlock();
				curr->lock.wrlock();
				// cout << "wrlock" << endl; /////////////////////////////////////
				it = curr->map.find(key);
				if (it == curr->map.end()) // add why we need this. !!!
				{
					curr = curr->map[key] = this->getNewTrieNode();
				}
				else curr = it->second; // go to next node
			}
			else curr = it->second; // go to next node
			pa->lock.unlock();
			if (i == seq.size() - 1) // !!!
			{
				// cout << "aaaa" << endl; /////////////////////////////////////
				// mark current node as leaf
				curr->leaflock.rdlock();
				// cout << "bbbb" << endl; /////////////////////////////////////
				if (curr->isLeaf == false)
				{
					curr->leaflock.unlock();
					curr->leaflock.wrlock();
					// cout << "cccc" << endl; /////////////////////////////////////
					if (curr->isLeaf == false) // !!!
					{
						curr->isLeaf = true;
						curr->leaflock.unlock();
						// cout << "unlock1" << endl; /////////////////////////////////////
						return true;
					}
					else
					{
						curr->leaflock.unlock();
						// cout << "unlock2" << endl; /////////////////////////////////////
						return false;
					}
				}
				else
				{
					curr->leaflock.unlock();
					return false;
				}
			}
			// cout << "unlock" << endl; /////////////////////////////////////
		}
		assert(false); // something wrong if program reaches here.
		return false;
		// cout << sizeof(this) << endl;
	}

	/*

	// Iterative function to search a string in Trie. It returns true
	// if the string is found in the Trie, else it returns false
	bool search(const vector<T> &seq) // not thread-safe !!!
	{
		// return false if Trie is empty
		// if (trie->root == nullptr)
		// 	return false;

		TrieNode<T> *curr = root;
		for (int i = 0; i < seq.size(); i++)
		{
			// go to next node
			curr = curr->map[seq[i]];
			// if string is invalid (reached end of path in Trie)
			if (curr == nullptr)
				return false;
		}

		// if current node is a leaf and we have reached the
		// end of the string, return true
		return curr->isLeaf;
	}

	*/

	void del_node(TrieNode<T> *node)
	{
		for (auto it = node->map.begin(); it != node->map.end(); ++it)
		{
			//cout << it->first << " => " << it->second << '\n';
			del_node(it->second);
		}

		// cout << node << " => " << "deleted!" << '\n';
		delete node;
	}

	void print_result(TrieNode<T> *node, vector<T> &path, size_t &count)
	{
		for (auto it = node->map.begin(); it != node->map.end(); ++it)
		{
			//cout << it->first << " => " << it->second << '\n';
			path.push_back(it->first);
			if (it->second->isLeaf)
			{
				for (int i = 0; i < path.size(); i++)
				{
					// cout << path[i];
				}
				// cout << endl;
				count++;
			}
			print_result(it->second, path, count);
			path.pop_back();
		}
	}

	size_t print_result()
	{
		vector<T> path;
		size_t count = 0;
		print_result(root, path, count);
		return count;
	}
};

#endif /* TRIE_H_ */
