#include <stdlib.h>
#include <cassert>

#include "hashtable.h"

void h_init(HTab *htab, size_t n){
    assert(n > 0 && ((n-1) & n == 0));
    htab->tab = static_cast<HNode **>(calloc(sizeof(struct HNode *), n));
    htab->mask = n - 1;
    htab->size = 0;
}

void h_insert(HTab *htab, HNode *new_node){
    // find the position
    size_t pos = new_node->hcode & htab->mask;

    HNode *next = htab->tab[pos];
    new_node->next = next;
    htab->tab[pos] = new_node;
    htab->size++;
}


HNode** h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *)){
    if(!htab){
        return NULL;
    }

    size_t pos = key->hcode & htab->mask;

    // lookup in one pass
    HNode **from = &htab->tab[pos];
    for (HNode *cur; (cur = *from)!=NULL; from = &(cur->next)){
        if(cur->hcode == key->hcode && eq(key, cur)) {
            return from;
        }
    }

    return NULL;
}



HNode* h_detach(HTab *htab, HNode **from){
    HNode *detached = *from;
    *from = detached->next;
    htab->size--;
    return detached;
}

/**
 * @brief insert a HNode into a Hmap, should progressively resize the HTabs underneath
 * 
 * @param hmap pointer to hashmap
 * @param node pointet to node to be inserted
 * 
 */
void hm_insert(HMap *hmap, HNode *node){
    if(!hmap->tb1.tab){
        h_init(&hmap->tb1, 4); // 1. init the table if empty
    }
    h_insert(&hmap->tb1, node); // 2. insert the new node

    // progressive resizing
    if(!hmap->tb2.tab){
        size_t load_factor = hmap->tb1.size / (hmap->tb1.mask + 1);
        if(load_factor > k_max_load_factor){
            hm_start_resizing(hmap); // 3. if overload, start to resize
        }
    }

    hm_help_resizing(hmap);
}


/**
 * @brief swap two hashtables, double size for the empty one and reset resizing pos
 * 
 * @param hmap target hashmap
 */
void hm_start_resizing(HMap *hmap){
    assert(!hmap->tb2.tab);
    hmap->tb2 = hmap->tb1;
    h_init(&hmap->tb1, 2 * (hmap->tb1.mask + 1));
    hmap->resizing_pos = 0;
}

/**
 * @brief helper function for progressive resizing
 * 
 * @param hmap target hashmap
 */
void hm_help_resizing(HMap *hmap){
    size_t n_work = 0;
    while(n_work < k_resizing_work && hmap->tb2.size > 0){ // move node from tb2 to tb1, one by one
        if(!hmap->tb2.tab[hmap->resizing_pos]){
            hmap->resizing_pos++;
        }
        HNode *to_move = h_detach(&hmap->tb2, &hmap->tb2.tab[hmap->resizing_pos]);
        h_insert(&hmap->tb1, to_move);
    }

    if(hmap->tb2.size == 0 && hmap->tb2.tab){ // tb2 is empty now
        free(hmap->tb2.tab);
        hmap->tb2 = HTab{}; // renew the whole table
    }
}

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)){
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->tb1, key, eq);
    from = from ? from : h_lookup(&hmap->tb2, key, eq);
    return from ? *from : NULL;
}

HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)){
    hm_help_resizing(hmap);
    // check first table
    HNode **from = h_lookup(&hmap->tb1, key, eq);
    if(from){
        HNode *deleted = h_detach(&hmap->tb1, from);
        return deleted;
    }
    // if not found, check second table
    from = h_lookup(&hmap->tb2, key, eq);
    if(from){
        HNode *deleted = h_detach(&hmap->tb2, from);
        return deleted;
    }
    return NULL;
}