#pragma once

#include "hashtable.h"
#include "avl_tree.h"
#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <cstring>

typedef struct ZSet { // put two indexes together
    AVLNode *tree = NULL;
    HMap hmap;
} ZSet;

typedef struct ZNode {
    AVLNode tree;
    HNode hmap;
    double score = 0.0;
    size_t len = 0; // length of name
    char name[0]; // array for name string (variable length, determine at init)
} ZNode;

/**
 * @brief allocate a new node for (score, name) pair
 * 
 * @param name buffer containing name string
 * @param len length of name string
 * @param score score
 * @return ZNode* znode on heap
 */
ZNode *znode_new(const char *name, size_t len, double score);

/**
 * @brief lookup a pair by name in a ZSet (so, using hmap index)
 * 
 * @param zset container 
 * @param name buffer containing target name string
 * @param len length of name string
 * @return ZNode* if exists, return pointer to that Znode; else return NULL
 */
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);

/**
 * @brief add a new (score, name) pair into the Sorted Set
 * 
 * @param zset target sorted set
 * @param name buffer containing name string
 * @param len len of name string
 * @param score score of pair
 * @return bool true if successfully insert, false if name already exists 
 */
bool zset_add(ZSet *zset, const char *name, size_t len, double score);


/**
 * @brief update score for a Znode in Zset
 * 
 * @param zset target zset
 * @param node node to update
 * @param score new score
 */
void zset_update(ZSet *zset, ZNode *node, double score);

/**
 * @brief lookup and detach a node by name
 * 
 * @param zset target sorted set
 * @param name buffer containing name to lookup
 * @param len length of name
 * @return ZNode* detached node
 */
ZNode *zset_pop(ZSet *zset, const char *name, size_t len);

/**
 * @brief deallocate the node
 * 
 * @param node node to deallocate
 */
void znode_del(ZNode *node);


/**
 * Sorted Set Range Query
 * 
 */



/**
 * @brief insert new znode into AVL_tree index
 * 
 * @param zset target zset
 * @param node znode to insert
 */
void tree_add(ZSet *zset, ZNode *node);

/**
 * @brief callback for HNode comparison
 * 
 * @param node hnode by compared
 * @param key hnode with target key
 * @return true equal hash value
 * @return false unequal hash value
 */
bool hcmp(HNode *node, HNode *key);

/**
 * @brief predicate lhs < rhs
 * 
 */
bool zless(AVLNode *lhs, AVLNode *rhs);
