#pragma once

#include <string>
#include <stdint.h>
#include <stddef.h>

struct HNode {
    HNode *next = NULL;
    size_t hcode = 0;
};

struct HTab {
    HNode **tab = NULL; // array of `HNode *`
    size_t mask; // 2^n - 1
    size_t size;
};

struct HMap { // use two tables for progressive resizing
    HTab tb1;
    HTab tb2;
    size_t resizing_pos = 0;
};

struct HKey { // helper struct for key comparison (less memory alloc)
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};


/***
 * interface of HTab (utils for HMap) 
*/

/**
 * @brief initialize a hashtable
 * 
 * @param htab pointer to a valid hashtable struct
 * @param n initial size of the hashtable, must be 2^n
 */
void h_init(HTab *htab, size_t n);

// hashtable insertion
void h_insert(HTab *htab, HNode *new_node);

// hashtable lookup, return the incoming pointer of a `pointer to HNode`

/**
 * @brief hashtable lookup
 * 
 * @param htab pointer to a hashtable struct
 * @param key key node to lookup
 * @param eq callback decide whether keys of two nodes are the same
 * @return HNode** incoming pointer of a `pointer to HNode`
 */
HNode** h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *));

/**
 * @brief detach a node from original hashtable
 * 
 * @param htab the original hashtable
 * @param from lookup results 
 * @return HNode* the pointer to detached node
 */
HNode* h_detach(HTab *htab, HNode **from);

/**
 * @brief scan all nodes in hashtable
 * 
 * @param tab target hash table
 * @param f callback to call on each node
 * @param arg argument to callback
 */
void h_scan(HTab *tab, void (*f)(HNode *, void *), void *arg);




/**
 * interfaces for HMap (API open to users)
*/

const size_t k_max_load_factor = 8;
const size_t k_resizing_work = 128;

/**
 * @brief get number of nodes of a HashMap struct
 * 
 * @param hmap target hashmap
 * @return uint32_t number of nodes
 */
uint32_t hm_size(HMap *hmap);

/**
 * @brief insert a HNode into a Hmap, should progressively resize the HTabs underneath
 * 
 * @param hmap pointer to hashmap
 * @param node pointet to node to be inserted
 * 
 */
void hm_insert(HMap *hmap, HNode *node);

/**
 * @brief swap two hashtables, double size for the empty one and reset resizing pos
 * 
 * @param hmap target hashmap
 */
void hm_start_resizing(HMap *hmap); 


/**
 * @brief helper function for progressive resizing
 * 
 * @param hmap target hashmap
 */
void hm_help_resizing(HMap *hmap);

/**
 * @brief insert a node in hashmap
 * 
 * @param hmap target hashmap
 * @param key node with keys to lookup
 * @param eq callback checking whether key equality
 * @return HNode* valid node in hashmap with provided key, or NULL if not found
 */
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));

/**
 * @brief delete a node in hashmap
 * 
 * @param hmap target hashmap
 * @param key node with keys to lookup
 * @param eq callback checking whether key equality
 * @return HNode* the deleted node
 */
HNode *hm_pop(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));

