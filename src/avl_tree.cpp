#include <stdlib.h>

#include "avl_tree.h"


// utils for absolute value
static uint32_t _abs(uint32_t n){
    return (n > 0) ? n : -n;
}

// utils for rotations


void avl_init(AVLNode *node){
    node->height = 0;
    node->cnt = 1;
    node->left = node->right = node->parent = NULL;
}




// user APIs to the tree

/**
 * @brief detach a node from the tree
 * 
 * @param node node to detach
 * @return AVLNode* new root of tree after deletion
 */
AVLNode *avl_del(AVLNode *node){
    if (node->right == NULL){ // no right child, set left child as new root
        AVLNode *parent = node->parent;
        // reassign parent and child relation
        if(node->left){node->left->parent = parent;}
        if(parent){
            (parent->left == node ? parent->left : parent->right) = node->left;
            return avl_fix(parent); // AVL-specific
        } else {
            return node->left;
        }
    } else {
        // find the victim to swap with: the `smallest` elem in right tree
        AVLNode *victim = node->right;
        while (victim->left) {victim = victim->left;}
        // detach the victim
        AVLNode *root = avl_del(victim);
        // swap target node with victim node
        *victim = *node;
        // reassign parent and child relation
        if(victim->left){
            victim->left->parent = victim;
        }
        if(victim->right){
            victim->right->parent = victim;
        }
        // check if root is removed
        if(AVLNode *parent = node->parent){
            (parent->left == node ? parent->left : parent->right) = victim;
            return root; // no use
        } else {
            return victim;
        }
    }
}



/** 
 * @brief rebalance the tree after insertion / deletion, from bottom to top (important!)
 * 
 * @param node root of subtree to be balanced
 * @return AVLNode* root of balanced tree
 */
AVLNode *avl_fix(AVLNode *node){ // fix imbalanced nodes and maintain invariants, until reach the root
    while(true){
        // assume children are already AVL balanced
        avl_update(node);
        uint32_t lh = avl_height(node->left);
        uint32_t rh = avl_height(node->right);
        AVLNode **from = NULL;
        if (AVLNode *p = node->parent){ // has parent, get data slot for node (in its parent)
            from = (p->left == node) ? &p->left : &p->right;
        }
        // balance this node
        if(lh - rh > 1){ // heavier left tree
            node = avl_fix_left(node);
        }else if(lh - rh < -1){
            node = avl_fix_right(node);
        }
        // process the parent
        if(!from) return node; // already reach root\

        *from = node; // reassign balanced root as parent's child (with second-order pointer)
        node = node->parent;
    }
}


AVLNode *avl_fix_recursive(AVLNode *node){
    if (!node) return NULL;
    
    // fix two subtrees 
    node->left = avl_fix(node->left);
    node->right = avl_fix(node->right);
    avl_update(node); // necessary since children are updated

    // check whether two subtree violate balanced rules
    // if violated, rotate the tree to "lift" the "heavier" side
    if (_abs(node->left->height - node->right->height) <= 1 ) { // already balanced
        return node;
    }
    else if(node->left->height > node->right->height){ // left heavier
        return avl_fix_left(node);
    }
    else{ // right heavier
        return avl_fix_right(node);
    }
}


/**
 * @brief particularly fix the heavier left tree
 * 
 * @param node root
 * @return AVLNode* root of balanced tree
 */
AVLNode *avl_fix_left(AVLNode *node){
    if(avl_height(node->left->right) > avl_height(node->left->left)){ // `LR` form
        node->left = rot_left(node->left); // transform to `LL` form
    }
    return rot_right(node); // balance the `LL` form
}

/**
 * @brief particularly fix the heavier right tree
 * 
 * @param node root
 * @return AVLNode* root of balanced tree
 */
AVLNode *avl_fix_right(AVLNode *node){
    if(avl_height(node->right->left) > avl_height(node->right->right)){ // `RL` form
        node->right = rot_right(node->right); // tranform to `RR` form
    }
    return rot_left(node); // balance the `RR` form
}





// left and right rotations of tree (private)
/**
 * @brief perform left rotation on a subtree (for balancing). Assume that all subtree 1-4 are AVL tree themselves
 * 
 * @param node root node of subtree to balance
 * @return AVLNode* root node of balanced new subtree 
 */
AVLNode *rot_left(AVLNode *node){
    // children to replace
    AVLNode *new_root = node->right;
    AVLNode *sbtree2 = new_root->left;
    // reassign children
    new_root->left = node;
    node->right = sbtree2;
    // reassign parents
    new_root->parent = node->parent;
    node->parent = new_root;
    if(sbtree2){
        sbtree2->parent = node;
    }
    // update the cnt for roots with new children (must update lower root (`node` here) first)
    avl_update(node);
    avl_update(new_root);

    return new_root;
}

/**
 * @brief perform right rotation on a subtree (for balancing)
 * 
 * @param node root node of subtree to balance
 * @return AVLNode* root node of balanced new subtree 
 */
AVLNode *rot_right(AVLNode *node){
    // children to replace
    AVLNode *new_root = node->left;
    AVLNode *sbtree2 = new_root->right;
    // reassign children
    new_root->right = node;
    node->left = sbtree2;
    // reassign parents
    new_root->parent = node->parent;
    node->parent = new_root;
    if(sbtree2){
        sbtree2->parent = node;
    }
    // update the cnt for roots with new children (must update lower root (`node` here) first)
    avl_update(node);
    avl_update(new_root);

    return new_root;
}











// helper functions for state retrieval and update
/**
 * @brief get height of node
 * 
 * @param node an AVLNode
 * @return uint32_t height of node; return -1 if node is NULL
 */
uint32_t avl_height(AVLNode *node){
    return node ? node->height : -1;
}

/**
 * @brief get cnt of subtree
 * 
 * @param node root of subtree
 * @return uint32_t cnt of subtree; return 0 if node is NULL
 */
uint32_t avl_cnt(AVLNode *node){
    return node ? node->cnt : 0;
}

/**
 * @brief update height and cnt of a node
 * 
 * @param node root node of subtree
 */
void avl_update(AVLNode *node){
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}


// util function
/**
 * @brief pick larger value of two int numericals
 * 
 * @param lhs 
 * @param rhs 
 * @return uint32_t 
 */
uint32_t max(uint32_t lhs, uint32_t rhs){
    return (lhs > rhs) ? lhs : rhs;
}