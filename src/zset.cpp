#include "zset.h"
#include <memory.h>


/**
 * @brief allocate a new node for (score, name) pair
 * 
 * @param name buffer containing name string
 * @param len length of name string
 * @param score score
 * @return ZNode* znode on heap
 */
ZNode *znode_new(const char *name, size_t len, double score){
    ZNode *new_node = (ZNode *)malloc(sizeof(ZNode) + len);
    // data
    new_node->score = score;
    memcpy(&new_node->name[0], name, len);
    new_node->len = len;
    // hash index
    new_node->hmap.hcode = str_hash((const uint8_t *)name, len);
    new_node->hmap.next = NULL;
    // avl tree index
    avl_init(&new_node->tree);

    return new_node;
}

/**
 * @brief lookup a pair by name in a ZSet (so, using hmap index)
 * 
 * @param zset container 
 * @param name buffer containing target name string
 * @param len length of name string
 * @return ZNode* if exists, return pointer to that Znode; else return NULL
 */
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len){
    if (!zset->tree) return NULL; // fast way to check whether sorted set is empty
    HKey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;
    HNode *found = hm_lookup(&zset->hmap, &key.node, hcmp);
    return found ? container_of(found, ZNode, hmap) : NULL;
}

/**
 * @brief add a new (score, name) pair into the Sorted Set
 * 
 * @param zset target sorted set
 * @param name buffer containing name string
 * @param len len of name string
 * @param score score of pair
 * @return bool true if successfully insert, false if name already exists 
 */
bool zset_add(ZSet *zset, const char *name, size_t len, double score){
    ZNode *found = zset_lookup(zset, name, len);
    if (found){
        zset_update(zset, found, score);
        return false;
    }
    // allocate new znode on heap
    ZNode *node = znode_new(name, len, score);
    // link znode to hashtable index
    hm_insert(&zset->hmap, &node->hmap);
    // link znode to avl tree index
    tree_add(zset, node);
    return true;
}


/**
 * @brief update score for a Znode in Zset
 * 
 * @param zset target zset
 * @param node node to update
 * @param score new score
 */
void zset_update(ZSet *zset, ZNode *node, double score){
    if (!zset) return;
    if (node->score == score) return;
    // detaching and re-inserting the AVL node would fix the order if score changed
    zset->tree = avl_del(&node->tree);
    node->score = score;
    avl_init(&node->tree); // reset the tree node (discard child info)
    tree_add(zset, node);
}


/**
 * @brief lookup and detach a node by name
 * 
 * @param zset target sorted set
 * @param name buffer containing name to lookup
 * @param len length of name
 * @return ZNode* detached node
 */
ZNode *zset_pop(ZSet *zset, const char *name, size_t len){
    if (!zset) return;
    // lookup and detach from hashset
    HKey key;
    key.len = len;
    key.name = name;
    key.node.hcode = str_hash((uint8_t *)name, len);
    HNode *hnode = hm_pop(&zset->hmap, &key.node, hcmp);
    if(!hnode) return NULL; // not found

    ZNode *found = container_of(hnode, ZNode, hmap);
    // detach from avl tree
    zset->tree = avl_del(&found->tree);
    
    return found;
}


/**
 * @brief deallocate the node
 * 
 * @param node node to deallocate
 */
void znode_del(ZNode *node){
    free(node);
}






/**
 * @brief insert new znode into AVL_tree index
 * 
 * @param zset target zset
 * @param node znode to insert
 */
void tree_add(ZSet *zset, ZNode *node){
    AVLNode *cur = NULL;
    AVLNode **from = &zset->tree; // incoming pointer of next node
    while(*from){
        cur = *from;
        from = &(zless(&node->tree, cur) ? cur->left : cur->right);
    }
    *from = &node->tree;
    node->tree.parent = cur;
    zset->tree = avl_fix(&node->tree);
}




bool hcmp(HNode *node, HNode *key){
    ZNode *znode = container_of(node, ZNode, hmap);
    HKey *hkey = container_of(key, HKey, node);
    if(znode->len != hkey->len) {
        return false;
    }
    return 0 == memcmp(znode->name, hkey->name, znode->len);
}

bool zless(AVLNode *lhs, AVLNode *rhs){
    ZNode *ls = container_of(lhs, ZNode, tree);
    ZNode *rs = container_of(rhs, ZNode, tree);
    if (ls->score != rs->score) return ls->score < rs->score;
    return strcmp(ls->name, rs->name) < 0;
}