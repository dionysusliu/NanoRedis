#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct AVLNode {
    uint32_t height = 0; // height of node
    uint32_t cnt = 0; // count of nodes in subtree
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    AVLNode *parent = NULL;
} AVLNode;

/**
 * @brief reset as a singular node
 * 
 * @param node root node
 */
void avl_init(AVLNode *node);




// user APIs to the tree

/**
 * @brief detach a node from the tree
 * 
 * @param node node to detach
 * @return AVLNode* new root of tree after deletion
 */
AVLNode *avl_del(AVLNode *node);

/** 
 * @brief rebalance the tree after insertion / deletion
 * 
 * @param node root of subtree to be balanced
 * @return AVLNode* root of balanced tree
 */
AVLNode *avl_fix(AVLNode *node);
AVLNode *avl_fix_recursive(AVLNode *node);

/**
 * @brief particularly fix the heavier left tree
 * 
 * @param node root
 * @return AVLNode* root of balanced tree
 */
AVLNode *avl_fix_left(AVLNode *node);

/**
 * @brief particularly fix the heavier right tree
 * 
 * @param node root
 * @return AVLNode* root of balanced tree
 */
AVLNode *avl_fix_right(AVLNode *node);



// left and right rotations of tree (private)
/**
 * @brief perform left rotation on a subtree (for balancing)
 * 
 * @param node root node of subtree to balance
 * @return AVLNode* root node of balanced new subtree 
 */
AVLNode *rot_left(AVLNode *node);

/**
 * @brief perform right rotation on a subtree (for balancing)
 * 
 * @param node root node of subtree to balance
 * @return AVLNode* root node of balanced new subtree 
 */
AVLNode *rot_right(AVLNode *node);











// helper functions for state retrieval and update
/**
 * @brief get height of node
 * 
 * @param node an AVLNode
 * @return uint32_t height of node; return 0 if node is NULL
 */
uint32_t avl_height(AVLNode *node);

/**
 * @brief get cnt of subtree
 * 
 * @param node root of subtree
 * @return uint32_t cnt of subtree; return 0 if node is NULL
 */
uint32_t avl_cnt(AVLNode *node);

/**
 * @brief update height and cnt of a node
 * 
 * @param node root node of subtree
 */
void avl_update(AVLNode *node);

/**
 * @brief find the node at given offset ranking
 * 
 * @param node 
 * @param offset 
 * @return AVLNode* 
 */
AVLNode *avl_offset(AVLNode *node, int64_t offset);


// util function
/**
 * @brief pick larger value of two int numericals
 * 
 * @param lhs 
 * @param rhs 
 * @return uint32_t 
 */
uint32_t max(uint32_t lhs, uint32_t rhs);