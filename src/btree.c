#include <raylib.h>

unsigned int max(unsigned int x, unsigned int y) {
    return x > y ? x : y;
}

/// NODE ///

/*
 * A node containing data for the BTree.
 */
typedef struct {
    int data;
    int parentIndex;
    int leftIndex;
    int rightIndex;
} IntBTreeNode;

/*
 * Fallback node for accidental retrievals.
 */
static IntBTreeNode NIL_NODE = {
    -1,
    -1,
    -1,
    -1,
};

/*
 * Create a new BTreeNode holding `data`, not connected to any other nodes.

 */
IntBTreeNode newIntBTreeNode(int data) {
    return (IntBTreeNode){
        .data        = data,
        .parentIndex = -1,
        .leftIndex   = -1,
        .rightIndex  = -1,
    };
}

/// ARRAY ///

/*
 * Array of BTreeNodes.
 * 
 * Keeps the nodes tightly-packed in memory for
 * efficiency.
 */
typedef struct {
    unsigned int  length;
    unsigned int  capacity;
    IntBTreeNode *nodes;
} IntBTreeNodeArray;

/*
 * Create a new array of BTreeNodes
 */
IntBTreeNodeArray newIntBTreeNodeArray() {
    return (IntBTreeNodeArray){
        .length   = 0,
        .capacity = 8,
        .nodes    = (IntBTreeNode *)MemAlloc(8 * sizeof(IntBTreeNode)),
    };
}

/*
 * Remove the nodes from an array.
 */
void clearIntBTreeNodeArray(IntBTreeNodeArray *array) {
    array->length = 0;
}

/*
 * Free the dynamic memory held by an array, destroying it.
 */
void freeIntBTreeNodeArray(IntBTreeNodeArray *array) {
    array->length   = 0;
    array->capacity = 0;
    MemFree(array->nodes);
}

/*
 * Push a node onto the back of the array, resizing its allocated memory if
 * needed.
 */
void pushIntBTreeNodeArray(IntBTreeNodeArray *array, IntBTreeNode node) {
    if (array->length == array->capacity) {
        array->capacity *= 2;
        array->nodes     = (IntBTreeNode *)MemRealloc(
            array->nodes, sizeof(IntBTreeNode) * array->capacity);
    }

    array->nodes[array->length] = node;
    array->length++;
}

/*
 * Set the value of an element in the array.
 */
void setIntBTreeNodeArray(IntBTreeNodeArray *array,
                          unsigned int       index,
                          IntBTreeNode       node) {
    if (index >= array->length) {
        return;
    }

    array->nodes[index] = node;
}

/*
 * Get a copy of a node in the array.
 */
IntBTreeNode getIntBTreeNodeArray(IntBTreeNodeArray *array,
                                  unsigned int       index) {
    if (index >= array->length) {
        NIL_NODE = (IntBTreeNode){
            -1,
            -1,
            -1,
            -1,
        };
        return NIL_NODE;
    }

    return array->nodes[index];
}

/*
 * Get a reference to a node in the array.
 */
IntBTreeNode *getRefIntBTreeNodeArray(IntBTreeNodeArray *array,
                                      unsigned int       index) {
    if (index >= array->length) {
        NIL_NODE = (IntBTreeNode){
            -1,
            -1,
            -1,
            -1,
        };
        return &NIL_NODE;
    }

    return &array->nodes[index];
}

/*
 * Swap the node at `index` with the last node in the array and then drop
 * the
 * last element.
 *
 * Automatically re-connects the swapped node to its
 * parent and children.
 */
void swapDeleteBTreeNodeArray(IntBTreeNodeArray *array, unsigned int index) {
    if (index >= array->length) {
        return;
    }

    unsigned int endIndex = array->length - 1;

    IntBTreeNode endNode    = getIntBTreeNodeArray(array, endIndex);
    IntBTreeNode deleteNode = getIntBTreeNodeArray(array, index);

    setIntBTreeNodeArray(array, index, endNode);
    setIntBTreeNodeArray(array, endIndex, deleteNode);
    array->length--;

    if (endNode.parentIndex != -1) {
        IntBTreeNode *parentNode =
            getRefIntBTreeNodeArray(array, endNode.parentIndex);
        if (parentNode->leftIndex == endIndex) {
            parentNode->leftIndex = index;
        } else {
            parentNode->rightIndex = index;
        }
    }

    if (endNode.rightIndex != -1) {
        getRefIntBTreeNodeArray(array, endNode.rightIndex)->parentIndex = index;
    }

    if (endNode.leftIndex != -1) {
        getRefIntBTreeNodeArray(array, endNode.leftIndex)->parentIndex = index;
    }
}

/// BTREE ///

/*
 * A self balancing BTree set for storing integers.
 */
typedef struct {
    unsigned int      depth;
    unsigned int      splitLength;
    int               rootIndex;
    IntBTreeNodeArray nodes;
} IntBTree;

/*
 * Create a new, empty BTree
 */
IntBTree newIntBTree() {
    return (IntBTree){
        .depth       = 0,
        .splitLength = 1,
        .rootIndex   = -1,
        .nodes       = newIntBTreeNodeArray(),
    };
}

/*
 * Free the dynamic memory held by the BTree, destroying it.
 */
void freeIntBTree(IntBTree *btree) {
    btree->splitLength = 0;
    btree->rootIndex   = 0;
    btree->depth       = 0;
    freeIntBTreeNodeArray(&btree->nodes);
}

/*
 * Clear all nodes from the BTree.
 */
void resetIntBTree(IntBTree *btree) {
    btree->splitLength = 1;
    btree->rootIndex   = -1;
    clearIntBTreeNodeArray(&btree->nodes);
}

/*
 * Find the index of the node in `btree->nodes` holding `value`.
 *
 *
 * Returns `-1` if no matching node found.
 */
int findIntBTree(IntBTree *btree, int value) {
    if (btree->rootIndex == -1) {
        return -1;
    }

    unsigned int  index = btree->rootIndex;
    IntBTreeNode *node;

    do {
        node = getRefIntBTreeNodeArray(&btree->nodes, index);
        if (node->data == value) {
            break;
        }

        if (value < node->data && node->leftIndex != -1) {
            index = node->leftIndex;
            continue;
        }

        if (value > node->data && node->rightIndex != -1) {
            index = node->rightIndex;
            continue;
        }

        index = -1;
    } while (index != -1);
    return index;
}

/*
 * Readjust the book-keeping data of the BTree used for rebalancing.
 */
void adjustDepthIntBTree(IntBTree *btree) {
    if (btree->nodes.length >= btree->splitLength) {
        btree->depth++;
        btree->splitLength = btree->splitLength << 1;
    } else if (btree->nodes.length < btree->splitLength / 2) {
        btree->splitLength = btree->splitLength >> 1;
        btree->depth--;
        if (btree->splitLength == 0) {
            btree->splitLength = 1;
            btree->depth       = 0;
        }
    }
}

/*
 * Get the depth of the node holding `value`.
 *
 * Returns `-1` if no node
 * was found.
 */
int getNodeDepth(IntBTree *btree, int value) {
    int index = findIntBTree(btree, value);
    if (index < 0) {
        return index;
    }

    int           depth = 0;
    IntBTreeNode *node;
    do {
        depth++;
        node  = getRefIntBTreeNodeArray(&btree->nodes, index);
        index = node->parentIndex;
    } while (index != -1);

    return depth;
}

/*
 * Perform a left rotation on node at index `xIndex`.
 */
void rotateLeftIntBTree(IntBTree *btree, unsigned int xIndex) {
    IntBTreeNode *xNode, *yNode, *xParentNode;
    xNode      = getRefIntBTreeNodeArray(&btree->nodes, xIndex);
    int yIndex = xNode->rightIndex;
    if (yIndex == -1) {
        return;
    }

    yNode             = getRefIntBTreeNodeArray(&btree->nodes, yIndex);
    xNode->rightIndex = yNode->leftIndex;
    if (yNode->leftIndex != -1) {
        getRefIntBTreeNodeArray(&btree->nodes, yNode->leftIndex)->parentIndex =
            xIndex;
    }
    yNode->parentIndex = xNode->parentIndex;
    if (xNode->parentIndex == -1) {
        btree->rootIndex = yIndex;
    } else {
        xParentNode =
            getRefIntBTreeNodeArray(&btree->nodes, xNode->parentIndex);
        if (xIndex == xParentNode->leftIndex) {
            xParentNode->leftIndex = yIndex;
        } else {
            xParentNode->rightIndex = yIndex;
        }
    }

    yNode->leftIndex   = xIndex;
    xNode->parentIndex = yIndex;
}

/*
 * Perform a right rotation on node at index `xIndex`.
 */
void rotateRightIntBtree(IntBTree *btree, unsigned int yIndex) {
    IntBTreeNode *yNode, *xNode, *yParentNode;
    yNode      = getRefIntBTreeNodeArray(&btree->nodes, yIndex);
    int xIndex = yNode->leftIndex;
    if (xIndex == -1) {
        return;
    }

    xNode            = getRefIntBTreeNodeArray(&btree->nodes, xIndex);
    yNode->leftIndex = xNode->rightIndex;
    if (xNode->rightIndex != -1) {
        getRefIntBTreeNodeArray(&btree->nodes, xNode->rightIndex)->parentIndex =
            yIndex;
    }
    xNode->parentIndex = yNode->parentIndex;
    if (yNode->parentIndex == -1) {
        btree->rootIndex = xIndex;
    } else {
        yParentNode =
            getRefIntBTreeNodeArray(&btree->nodes, yNode->parentIndex);
        if (yIndex == yParentNode->rightIndex) {
            yParentNode->rightIndex = xIndex;
        } else {
            yParentNode->leftIndex = xIndex;
        }
    }

    xNode->rightIndex  = yIndex;
    yNode->parentIndex = xIndex;
}

/*
 * Find the node in branch starting at node `index` with the lowest value.

 */
int minimumIntBTree(IntBTree *btree, unsigned int index) {
    IntBTreeNode *node;
    while ((node = getRefIntBTreeNodeArray(&btree->nodes, index))->leftIndex !=
           -1) {
        index = node->leftIndex;
    }
    return index;
}

/*
 * Replace node `u` with node `v`.
 */
void transplantIntBTree(IntBTree    *btree,
                        unsigned int uIndex,
                        unsigned int vIndex) {
    IntBTreeNode *uNode, *uParent, *vNode, *vParent;
    uNode = getRefIntBTreeNodeArray(&btree->nodes, uIndex);
    vNode = getRefIntBTreeNodeArray(&btree->nodes, vIndex);

    if (uNode->parentIndex == -1) {
        btree->rootIndex = vIndex;
    } else {
        uParent = getRefIntBTreeNodeArray(&btree->nodes, uNode->parentIndex);
        if (uIndex == uParent->leftIndex) {
            uParent->leftIndex = vIndex;
        } else {
            uParent->rightIndex = vIndex;
        }
    }
    if (vNode->parentIndex != -1) {
        vParent = getRefIntBTreeNodeArray(&btree->nodes, vNode->parentIndex);
        if (vIndex == vParent->leftIndex) {
            vParent->leftIndex = -1;
        } else {
            vParent->rightIndex = -1;
        }
    }
    vNode->parentIndex = uNode->parentIndex;
}

/*
 * Get the maximum depth of branch starting at node `index`.
 */
unsigned int getBranchDepth(IntBTree *btree, int index) {
    if (index == -1) {
        return 0;
    }

    IntBTreeNode *node = getRefIntBTreeNodeArray(&btree->nodes, index);
    return max(getBranchDepth(btree, node->leftIndex),
               getBranchDepth(btree, node->rightIndex)) +
           1;
}

/*
 * Traverse up the BTree from node `index` to the root, rotating away from
 * the
 * deepest branch at each step.
 */
void rebalanceIntBTree(IntBTree *btree, unsigned int index) {
    IntBTreeNode *node;
    unsigned int  leftDepth, rightDepth;
    do {
        node       = getRefIntBTreeNodeArray(&btree->nodes, index);
        leftDepth  = getBranchDepth(btree, node->leftIndex);
        rightDepth = getBranchDepth(btree, node->rightIndex);

        if (leftDepth > rightDepth) {
            rotateRightIntBtree(btree, index);
            index = getRefIntBTreeNodeArray(&btree->nodes, node->parentIndex)
                        ->parentIndex;
        } else if (leftDepth < rightDepth) {
            rotateLeftIntBTree(btree, index);
            index = getRefIntBTreeNodeArray(&btree->nodes, node->parentIndex)
                        ->parentIndex;
        } else {
            index = node->parentIndex;
        }
    } while (index != -1);
}

/*
 * Insert `data` into the BTree, returning early if it already exists in the
 * set.
 */
void insertIntBTree(IntBTree *btree, int data) {
    if (btree->rootIndex == -1) {
        btree->rootIndex = btree->nodes.length;
        pushIntBTreeNodeArray(&btree->nodes, newIntBTreeNode(data));
        adjustDepthIntBTree(btree);
        return;
    }
    unsigned int  opDepth   = 0;
    unsigned int  nodeIndex = btree->rootIndex;
    IntBTreeNode *node, newNode;
    newNode = newIntBTreeNode(data);

    do {
        node = getRefIntBTreeNodeArray(&btree->nodes, nodeIndex);

        if (node->data == data) {
            return;
        }

        opDepth++;

        if (data < node->data) {
            if (node->leftIndex != -1) {
                nodeIndex = node->leftIndex;
                continue;
            }
            newNode.parentIndex = nodeIndex;
            node->leftIndex     = btree->nodes.length;
            pushIntBTreeNodeArray(&btree->nodes, newNode);
            break;
        } else {
            if (node->rightIndex != -1) {
                nodeIndex = node->rightIndex;
                continue;
            }
            newNode.parentIndex = nodeIndex;
            node->rightIndex    = btree->nodes.length;
            pushIntBTreeNodeArray(&btree->nodes, newNode);
            break;
        }
    } while (1);

    if (btree->nodes.length > 2 && opDepth > btree->depth) {
        rebalanceIntBTree(btree, node->parentIndex);
    }
    adjustDepthIntBTree(btree);
}

/*
 * Remove `value` from the set, if it is in there.
 */
void removeIntBtree(IntBTree *btree, int value) {
    int zIndex = findIntBTree(btree, value);
    if (zIndex == -1) {
        return;
    }

    int           xIndex;
    IntBTreeNode *zNode;
    zNode = getRefIntBTreeNodeArray(&btree->nodes, zIndex);

    if (zNode->rightIndex != -1 && zNode->leftIndex != -1) {
        xIndex = minimumIntBTree(btree, zNode->rightIndex);
        transplantIntBTree(btree, zIndex, xIndex);
    } else if (zNode->leftIndex == -1 && zNode->rightIndex != -1) {
        xIndex = zNode->rightIndex;
        transplantIntBTree(btree, zIndex, xIndex);
    } else if (zNode->leftIndex != -1 && zNode->rightIndex == -1) {
        xIndex = zNode->leftIndex;
        transplantIntBTree(btree, zIndex, xIndex);
    } else if (zNode->parentIndex != -1) {
        IntBTreeNode *parentNode =
            getRefIntBTreeNodeArray(&btree->nodes, zNode->parentIndex);
        if (zIndex == parentNode->leftIndex) {
            parentNode->leftIndex = -1;
        } else {
            parentNode->rightIndex = -1;
        }
    }

    swapDeleteBTreeNodeArray(&btree->nodes, zIndex);
}
