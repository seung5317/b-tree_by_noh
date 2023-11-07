#include <vector>
#include <sys/time.h>
#include "bptree.h"
#include <queue>



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
    node = (NODE *)calloc(1, sizeof(NODE));
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
        Root = root;  
        return root;
    }
    NODE *parent = left->parent;
    TEMP temp;
    int i, j;
    for (i = 0; i < parent->nkey; i++) {
        if (left == parent->chi[i]) break;
    }

    for (j = 0; j < i; j++) {
        temp.key[j] = parent->key[j];
        temp.chi[j] = parent->chi[j];
    }
    temp.key[j] = key;
    temp.chi[j] = left; 
    temp.chi[j + 1] = right;
    for (++j; j <= parent->nkey; j++) {
        temp.key[j] = parent->key[j - 1];
        temp.chi[j + 1] = parent->chi[j];
    }
    temp.nkey = parent->nkey + 1;

    if (temp.nkey < N) {
        for (j = 0; j < temp.nkey; j++) {
            parent->key[j] = temp.key[j];
            parent->chi[j] = temp.chi[j];
        }
        parent->chi[temp.nkey] = temp.chi[temp.nkey];
        parent->nkey = temp.nkey;
        for (j = 0; j <= parent->nkey; j++) {
            if (parent->chi[j]) {
                parent->chi[j]->parent = parent;
            }
        }
        return NULL;  
    } else {
        int midpoint = temp.nkey / 2;
        int promote_key = temp.key[midpoint];

        NODE *new_parent = alloc_leaf(NULL);
        new_parent->isLeaf = false;
        int k = 0;
        for (i = midpoint + 1; i < temp.nkey; i++) {
            new_parent->key[k] = temp.key[i];
            new_parent->chi[k] = temp.chi[i];
            new_parent->chi[k]->parent = new_parent; 
            k++;
        }
        new_parent->chi[k] = temp.chi[temp.nkey];
        new_parent->chi[k]->parent = new_parent; 
        new_parent->nkey = k;

        for (i = 0; i < midpoint; i++) {
            parent->key[i] = temp.key[i];
            parent->chi[i] = temp.chi[i];
            parent->chi[i]->parent = parent; 
        }
        parent->nkey = i;
        parent->chi[i] = temp.chi[midpoint];
        parent->chi[i]->parent = parent; 

        return insert_in_parent(parent, promote_key, new_parent);
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

        insert_in_parent(leaf, new_leaf->key[0], new_leaf);
      }
    return Root;
}

void shuffle(int *array, long n) {
    if (n > 1) {
        long i;
        for (i = 0; i < n - 1; i++) {
          long j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
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

	begin = cur_time();
    int total_num = 200;
    int num = 100;
    int numbers[total_num];
    for (int i = 0; i < total_num; i++) {
        numbers[i] = i + 1;
    }

    srand((unsigned)time(NULL));
    shuffle(numbers, total_num);

    for (int i = 0; i < num; i++) {
        insert(numbers[i], NULL);
    }
    print_tree(Root);

	end = cur_time();

	return 0;
} 