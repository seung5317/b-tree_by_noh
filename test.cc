#include <vector>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <strings.h>
#include <math.h>
#include "debug.h"
#include <iostream>
#include <queue>
using namespace std;
#define MAX_OBJ (1000*1000)
#define N 4

typedef struct _DATA {
	int key;
	int val;
	struct _DATA *next;
} DATA;



typedef struct _NODE {
	bool isLeaf;
	struct _NODE *chi[N];
	int key[N-1]; 
	int nkey;
	struct _NODE *parent;
} NODE;

typedef struct _TEMP {
	bool isLeaf;
	NODE *chi[N+1]; // for internal split (for leaf, only N is enough)
	int key[N]; // for leaf split
	int nkey;
} TEMP;

DATA Head;
DATA *Tail;
NODE *Root = NULL;



struct timeval
cur_time(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t;
}

void print_tree(NODE *root) {
    if (!root) {
        printf("The tree is empty\n");
        return;
    }

    queue<NODE*> nodeQueue;
    nodeQueue.push(root);

    while (!nodeQueue.empty()) {
        int levelSize = nodeQueue.size(); 

        for (int i = 0; i < levelSize; i++) {
            NODE *current = nodeQueue.front();
            nodeQueue.pop();

            printf("[");
            for (int j = 0; j < current->nkey; j++) {
                printf("%d", current->key[j]);
                if (j != current->nkey - 1) {
                    printf(" ");
                }
            }
            printf("] ");
            if (!current->isLeaf) {
                for (int j = 0; j <= current->nkey; j++) {
                    if (current->chi[j]) {
                        nodeQueue.push(current->chi[j]);
                    }
                }
            }
        }
        printf("\n"); 
    }
    fflush(stdout);
}

NODE *
find_leaf(NODE *node, int key)
{
	int kid;

	if (node->isLeaf) return node;
	for (kid = 0; kid < node->nkey; kid++) {
		if (key < node->key[kid]) break;
	}

	return find_leaf(node->chi[kid], key);
}

NODE *
insert_in_leaf(NODE *leaf, int key, DATA *data)
{
	int i;
	if (key < leaf->key[0]) {
		for (i = leaf->nkey; i > 0; i--) {
			leaf->chi[i] = leaf->chi[i-1] ;
			leaf->key[i] = leaf->key[i-1] ;
		} 
		leaf->key[0] = key;
		leaf->chi[0] = (NODE *)data;
	}
	else {
		for (i = 0; i < leaf->nkey; i++) {
			if (key < leaf->key[i]) break;
		}
		for (int j = leaf->nkey; j > i; j--) {		
			leaf->chi[j] = leaf->chi[j-1] ;
			leaf->key[j] = leaf->key[j-1] ;
		} 
	leaf->key[i] = key;
	leaf->chi[i] = (NODE *)data;

	}
	leaf->nkey++;

	return leaf;
}

NODE *
alloc_leaf(NODE *parent)
{
	NODE *node;
	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->isLeaf = true;
	node->parent = parent;
	node->nkey = 0;

	return node;
}




NODE *insert_in_parent(NODE *left, int key, NODE *right) {
    if (left->parent == NULL) {
        NODE *root = alloc_leaf(NULL);
        root->isLeaf = false;
        root->key[0] = key;
        root->chi[0] = left;
        root->chi[1] = right;
        root->nkey = 1;
        left->parent = root;
        right->parent = root;
        return root;
    }

    NODE *parent = left->parent;
    int insert_index = 0;
    while (insert_index < parent->nkey && parent->key[insert_index] < key) {
        insert_index++;
    }

    if (parent->nkey < N - 1) {
        for (int i = parent->nkey; i > insert_index; i--) {
            parent->key[i] = parent->key[i - 1];
            parent->chi[i + 1] = parent->chi[i];
        }
        parent->key[insert_index] = key;
        parent->chi[insert_index + 1] = right;
        parent->nkey++;
        return NULL;
    }

    NODE *new_node = alloc_leaf(NULL);
    new_node->isLeaf = false;

    TEMP temp;
    for (int i = 0; i < parent->nkey; i++) {
        temp.key[i] = parent->key[i];
        temp.chi[i] = parent->chi[i];
    }
    temp.chi[parent->nkey] = parent->chi[parent->nkey];
    temp.nkey = parent->nkey;

    for (int i = temp.nkey; i > insert_index; i--) {
        temp.key[i] = temp.key[i - 1];
        temp.chi[i + 1] = temp.chi[i];
    }
    temp.key[insert_index] = key;
    temp.chi[insert_index + 1] = right;
    temp.nkey++;

    int midpoint = temp.nkey / 2;
    int promote_key = temp.key[midpoint];

    parent->nkey = 0;
    for (int i = 0; i < midpoint; i++) {
        parent->key[i] = temp.key[i];
        parent->chi[i] = temp.chi[i];
        parent->nkey++;
    }
    parent->chi[midpoint] = temp.chi[midpoint];

    new_node->nkey = 0;
    for (int i = midpoint + 1; i < temp.nkey; i++) {
        new_node->key[new_node->nkey] = temp.key[i];
        new_node->chi[new_node->nkey] = temp.chi[i];
        new_node->nkey++;
    }
    new_node->chi[new_node->nkey] = temp.chi[temp.nkey];

    for (int i = 0; i <= new_node->nkey; i++) {
        if (new_node->chi[i]) new_node->chi[i]->parent = new_node;
    }
    for (int i = 0; i <= midpoint; i++) {
        if (parent->chi[i]) parent->chi[i]->parent = parent;
    }


    if (parent->parent == NULL) {
        NODE *new_root = alloc_leaf(NULL);
        new_root->isLeaf = false;
        new_root->key[0] = promote_key;
        new_root->chi[0] = parent;
        new_root->chi[1] = new_node;
        new_root->nkey = 1;
        parent->parent = new_root;
        new_node->parent = new_root;
        return new_root;
    } else {
        return insert_in_parent(parent->parent, promote_key, new_node);
    }
}



NODE *
insert(int key, DATA *data)
{
    NODE *leaf;

    if (Root == NULL) {
        leaf = alloc_leaf(NULL);
        Root = leaf;
    } else {
        leaf = find_leaf(Root, key);
    }
    if (leaf->nkey < N-1) {
        insert_in_leaf(leaf, key, data);
    } else {
        TEMP temp;
        temp.isLeaf = true;
        for (int i = 0; i < leaf->nkey; i++) {
            temp.chi[i] = leaf->chi[i];
            temp.key[i] = leaf->key[i];
        }
        temp.chi[leaf->nkey] = (NODE*) data;
        temp.key[leaf->nkey] = key;
        temp.nkey = leaf->nkey + 1;
        
        for (int i = 0; i < temp.nkey - 1; i++) {
            for (int j = i + 1; j < temp.nkey; j++) {
                if (temp.key[i] > temp.key[j]) {
                    swap(temp.key[i], temp.key[j]);
                    swap(temp.chi[i], temp.chi[j]);
                }
            }
        }

        leaf->nkey = 0;
        int split = (temp.nkey + 1) / 2;

        NODE *new_leaf = alloc_leaf(leaf->parent);

        leaf->nkey = split;
        for (int i = 0; i < split; i++) {
            leaf->chi[i] = temp.chi[i];
            leaf->key[i] = temp.key[i];
        }

        new_leaf->nkey = temp.nkey - split;
        for (int i = split; i < temp.nkey; i++) {
            new_leaf->chi[i-split] = temp.chi[i];
            new_leaf->key[i-split] = temp.key[i];
        }

        if (leaf->parent == NULL) {
        Root = insert_in_parent(leaf, new_leaf->key[0], new_leaf);
       } else {
         insert_in_parent(leaf, new_leaf->key[0], new_leaf);
       }
      }
    return Root;
}




void
init_root(void)
{
	Root = NULL;
}

int 
interactive()
{
  int key;

  std::cout << "Key: ";
  std::cin >> key;

  return key;
}

int
main(int argc, char *argv[])
{
  struct timeval begin, end;

	init_root();

	printf("-----Insert-----\n");
	begin = cur_time();
  while (true) {
		insert(interactive(), NULL);
    print_tree(Root);
  }
	end = cur_time();

	return 0;
} 