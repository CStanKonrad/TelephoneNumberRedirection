/** @file
 * Implementacja struktury reprezentującej
 * skompresowane drzewo TRIE
 *
 * @author Konrad Staniszewski
 * @copyright Konrad Staniszewski
 * @date 04.05.2018
 */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "radix_tree.h"
#include "stdfunc.h"


int radixTreeIsRoot(RadixTreeNode node) {
    return node->txt != NULL && strcmp(node->txt, RADIX_TREE_ROOT_TXT) == 0;
}

/**
 * @brief Inicjuje węzeł drzewa.
 * #### Złożoność
 * O(1)
 * @param[in] node  - wskaźnik na węzeł.
 */
static void radixTreeInitNode(RadixTreeNode node) {
    node->data = NULL;
    node->txt = NULL;

    node->father = NULL;

    size_t i;
    for (i = 0; i < RADIX_TREE_NUMBER_OF_SONS; i++) {
        node->sons[i] = NULL;
    }

}

static void radixTreeFreeNode(RadixTreeNode node) {
    if (!radixTreeIsRoot(node)) {
        assert(node->data == NULL);
        if (node->txt != NULL) {
            free((void *) node->txt);
            node->txt = NULL;
        }
    }
    free(node);
}


/**
 * @brief Inicjuje nowopowstałe drzewo.
 * #### Złożoność
 * O(1)
 * @param[in] tree  - wskaźnik na drzewo.
 */
static void radixTreeInitTree(RadixTree tree) {
    radixTreeInitNode(tree);
    tree->txt = RADIX_TREE_ROOT_TXT;
}


/**
 * @brief Tworzy węzeł drzewa i inicjalizuje go.
 * #### Złożoność
 * O(1)
 * @return Wskaźnik na stworzony węzeł, w przypadku
 *         problemów z pamięcią NULL.
 */
static RadixTreeNode radixTreeCreateNode() {
    RadixTreeNode result = malloc(sizeof(struct RadixTreeNode));
    if (result == NULL) {
        return NULL;
    } else {
        radixTreeInitNode(result);
        return result;
    }
}


/**
 * @brief Tworzy drzewo i inicjalizuje je.
 * #### Złożoność
 * O(1)
 * @return Wskaźnik na stworzone drzewo, w przypadku
 *         problemów z pamięcią NULL.
 */
RadixTree radixTreeCreate() {
    RadixTree result = malloc(sizeof(struct RadixTreeNode));
    if (result == NULL) {
        return NULL;
    } else {
        radixTreeInitTree(result);
        return result;
    }
}

static int radixTreeConvertCharToNumber(char sonId) {
    return sonId - '0';
}

/**
 * @brief Sprawdza czy węzeł @p node ma syna o numerze @p son.
 * @param node - wskaźnik na węzeł.
 * @param son - numer syna.
 * @return Niezerowa wartość jeżeli ma, zerowa w przeciwnym wypadku.
 */
static int radixTreeHasSon(RadixTreeNode node, char son) {
    return node->sons[radixTreeConvertCharToNumber(son)] != NULL;
}

static size_t radixTreeHowManySons(RadixTreeNode node) {
    size_t result = 0;
    size_t i;
    for (i = 0; i < RADIX_TREE_NUMBER_OF_SONS; i++) {
        if (node->sons[i] != NULL) {
            result++;
        }
    }
    return result;
}

static int radixTreeHasSons(RadixTreeNode node) {
    return radixTreeHowManySons(node) != 0;
}

static int radixTreeIsNodeRedundant(RadixTreeNode node) {
    return  (!radixTreeIsRoot(node)) &&
            (!radixTreeHasSons(node)) &&
            node->data == NULL;
}

static int radixTreeCanBeMergedWithSon(RadixTreeNode node) {
    return  (!radixTreeIsRoot(node)) &&
            radixTreeHowManySons(node) == 1 &&
            node->data == NULL;
}

static void radixTreeMoveToSon(RadixTreeNode *ptr, char son) {
    assert(radixTreeHasSon(*ptr, son));
    *ptr = (*ptr)->sons[radixTreeConvertCharToNumber(son)];
}

static void radixTreeChangeSon(RadixTreeNode node, char son, RadixTreeNode ch) {
    if (node != NULL) {
        node->sons[radixTreeConvertCharToNumber(son)] = ch;
    }
}

static int radixTreeMoveTxt(RadixTreeNode node, const char **txt,
                            const char **nodeTxtPtr) {
    const char *i = node->txt;

    if (radixTreeIsRoot(node)) {
        i = stringEnd(node->txt);
    } else {
        while (*i != '\0'
               && *(*txt) != '\0'
               && *i == *(*txt)) {
            i++;
            (*txt)++;
        }
    }

    *nodeTxtPtr = i;
    if (*i == '\0') {
        return RADIX_TREE_OPERATION_SUCCESS;
    } else {
        return RADIX_TREE_OPERATION_FAIL;
    }
}

static int radixTreeMove(RadixTreeNode *node, const char **txt,
                         const char **nodeTxtPtr) {
    assert(*(*txt) != '\0');
    if (!radixTreeHasSon(*node, *(*txt))) {
        return RADIX_TREE_OPERATION_FAIL;
    } else {
        radixTreeMoveToSon(node, *(*txt));
        return radixTreeMoveTxt(*node, txt, nodeTxtPtr);
    }

}

int radixTreeFind(RadixTree tree, const char *txt, RadixTreeNode *ptr,
                  const char **txtMatchPtr, const char **nodeMatchPtr) {
    *ptr = tree;
    *txtMatchPtr = txt;
    *nodeMatchPtr = stringEnd((*ptr)->txt);

    while (*(*txtMatchPtr) != '\0'
           && radixTreeMove(ptr, txtMatchPtr, nodeMatchPtr)
              == RADIX_TREE_OPERATION_SUCCESS);

    if (*(*nodeMatchPtr) == '\0' && *(*txtMatchPtr) == '\0') {
        return RADIX_TREE_FOUND;
    } else if (*(*txtMatchPtr) == '\0') {
        return RADIX_TREE_SUBSTR;
    } else {
        return RADIX_TREE_NOT_FOUND;
    }
}

size_t radixTreeHowManyCharsOffset(RadixTreeNode node, const char *txt) {
    return (txt - node->txt);
}

size_t radixTreeHowManyChars(RadixTreeNode node) {
    return strlen(node->txt);
}

static int radixTreeSplitNode(RadixTreeNode node, const char *splitPtr) {
    RadixTreeNode newNode = radixTreeCreateNode();

    if (newNode == NULL) {
        return RADIX_TREE_OPERATION_FAIL;
    } else {
        size_t matchingTextLength = splitPtr - node->txt;
        size_t textLeftLength = strlen(node->txt) - matchingTextLength;

        char *textA = malloc((matchingTextLength + (size_t) 1) * sizeof(char));
        if (textA == NULL) {
            radixTreeFreeNode(newNode);
            return RADIX_TREE_OPERATION_FAIL;
        } else {
            char *textB = malloc((textLeftLength + (size_t) 1) * sizeof(char));
            if (textB == NULL) {
                free(textA);
                radixTreeFreeNode(newNode);
                return RADIX_TREE_OPERATION_FAIL;
            } else {
                copyText(node->txt, textA, matchingTextLength);
                copyText(node->txt + matchingTextLength, textB, textLeftLength);

                free((void *) node->txt);

                node->txt = textB;
                newNode->txt = textA;

                newNode->father = node->father;
                radixTreeChangeSon(node->father, *newNode->txt, newNode);

                node->father = newNode;
                radixTreeChangeSon(newNode, *node->txt, node);
                return RADIX_TREE_OPERATION_SUCCESS;
            }
        }


    }
}

static RadixTreeNode radixTreeInsertLeaf(RadixTreeNode node, const char *txt) {
    size_t textLength = strlen(txt);
    char *copiedText = malloc((textLength + (size_t) 1) * sizeof(char));
    if (copiedText == NULL) {
        return NULL;
    } else {
        copyText(txt, copiedText, textLength);
        RadixTreeNode newNode = radixTreeCreateNode();
        if (newNode == NULL) {
            free(copiedText);
            return NULL;
        } else {
            newNode->txt = copiedText;

            newNode->father = node;
            assert(!radixTreeHasSon(node, *newNode->txt));
            radixTreeChangeSon(node, *newNode->txt, newNode);

            return newNode;
        }
    }
}

RadixTreeNode radixTreeInsert(RadixTree tree, const char *txt) {
    RadixTreeNode insertPtr;
    const char *matchPtr;
    const char *nodeMatchPtr;
    int findResult = radixTreeFind(tree, txt, &insertPtr, &matchPtr, &nodeMatchPtr);

    if (findResult == RADIX_TREE_FOUND) {
        return insertPtr;
    } else if (findResult == RADIX_TREE_SUBSTR) {
        int splitResult = radixTreeSplitNode(insertPtr, nodeMatchPtr);
        if (splitResult == RADIX_TREE_OPERATION_SUCCESS) {
            return insertPtr->father;
        } else {
            return NULL;
        }
    } else if (findResult == RADIX_TREE_NOT_FOUND) {
        if (*nodeMatchPtr != '\0') {
            int splitResult = radixTreeSplitNode(insertPtr, nodeMatchPtr);
            if (splitResult == RADIX_TREE_OPERATION_SUCCESS) {
                return radixTreeInsert(tree, txt);
            } else {
                return NULL;
            }
        } else {
            return radixTreeInsertLeaf(insertPtr, matchPtr);
        }
    } else {
        return NULL;
    }
}

void radixTreeDeleteSubTree(RadixTreeNode subTreeNode,
                            void (*f)(void *, void *),
                            void *fData) {
    RadixTreeNode pos = subTreeNode, tmp;
    pos->foldI = 0;

    while (!(pos == subTreeNode
             && pos->foldI == RADIX_TREE_NUMBER_OF_SONS)) {
        size_t *i = &pos->foldI;
        if (*i == RADIX_TREE_NUMBER_OF_SONS) {
            if (pos->data != NULL) {
                f(pos->data, fData);
                pos->data = NULL;
            }
            tmp = pos;
            pos = pos->father;
            radixTreeChangeSon(pos, *tmp->txt, NULL);
            radixTreeFreeNode(tmp);

        } else {
            if (pos->sons[*i] != NULL) {
                pos = pos->sons[*i];
                pos->foldI = 0;
            }
            (*i)++;
        }
    }

    if (subTreeNode->data != NULL) {
        f(subTreeNode->data, fData);
        subTreeNode->data = NULL;
    }
    if (!radixTreeIsRoot(subTreeNode)) {
        radixTreeChangeSon(subTreeNode->father, *subTreeNode->txt, NULL);
    }
    radixTreeFreeNode(subTreeNode);
}

void radixTreeDelete(RadixTree tree, void (*f)(void *, void *), void *fData) {
    radixTreeDeleteSubTree(tree, f, fData);
}

void radixTreeEmptyDelFunction(void *ptrA, void *ptrB) {

}

void *radixTreeGetNodeData(RadixTreeNode node) {
    return node->data;
}

RadixTreeNode radixTreeFather(RadixTreeNode node) {
    return node->father;
}

static RadixTreeNode radixTreeFristSon(RadixTreeNode node) {
    size_t i;
    for (i = 0; i < RADIX_TREE_NUMBER_OF_SONS; i++) {
        if (node->sons[i] != NULL) {
            return node->sons[i];
        }
    }
    return NULL;
}

static int radixTreeMerge(RadixTreeNode a, RadixTreeNode b) {

    size_t aTextLength = strlen(a->txt);
    size_t bTextLength = strlen(b->txt);
    size_t textLength =  aTextLength + bTextLength;
    char *txt = malloc(textLength + (size_t)1);

    if (txt == NULL) {
        return RADIX_TREE_OPERATION_FAIL;
    } else {
        copyText(a->txt, txt, aTextLength);
        copyText(b->txt, txt + aTextLength, bTextLength);

        free((void*)b->txt);

        b->txt = txt;

        b->father = a->father;
        radixTreeChangeSon(a->father, *b->txt, b);
        radixTreeFreeNode(a);
    }



}

void radixTreeBalance(RadixTreeNode node) {
    RadixTreeNode pos = node, tmp;
    size_t skipped = 0;
    const size_t canSkip = 5;

    while (!radixTreeIsRoot(pos)
            && skipped <= canSkip) {
        if (radixTreeIsNodeRedundant(pos)) {
            tmp = pos;
            pos = pos->father;
            radixTreeChangeSon(pos, *tmp->txt, NULL);
            radixTreeFreeNode(tmp);
        } else if (radixTreeCanBeMergedWithSon(pos)) {
            tmp = pos;
            pos = pos->father;
            int mergeResult =radixTreeMerge(tmp, radixTreeFristSon(tmp));
            if (mergeResult != RADIX_TREE_OPERATION_SUCCESS) {
                skipped++;
            }
        } else {
            pos = pos->father;
            skipped++;
        }
    }

}

void radixTreeSetData(RadixTreeNode node, void *ptr) {
    node->data = ptr;
}

int radixTreeFindLite(RadixTree tree, const char *txt, RadixTreeNode *ptr) {
    const char *unused1;
    const char *unused2;
    return radixTreeFind(tree, txt, ptr, &unused1, &unused2);
}

char *radixGetFullText(RadixTreeNode node) {
    RadixTreeNode pos = node;
    size_t length = 0;

    while (!radixTreeIsRoot(pos)) {
        length += radixTreeHowManyChars(pos);
        pos = radixTreeFather(pos);
    }

    char *result = malloc(length + (size_t) 1);
    if (result == NULL) {
        return NULL;
    } else {
        result[length] = '\0';
        pos = node;
        while (!radixTreeIsRoot(pos)) {
            size_t len = strlen(pos->txt);
            size_t i;
            length -= len;
            for (i = 0; i < len; i++) {
                result[length + i] = pos->txt[i];
            }
            pos = radixTreeFather(pos);
        }
        return result;
    }

}




