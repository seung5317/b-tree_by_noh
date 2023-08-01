#include <vector>
#include <sys/time.h>
#include "bptree.h"



struct timeval
cur_time(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t;
}

void
print_tree_core(NODE *n)
{
	printf("["); 
	for (int i = 0; i < n->nkey; i++) {
		if (!n->isLeaf) print_tree_core(n->chi[i]); 
		printf("%d", n->key[i]); 
		if (i != n->nkey-1 && n->isLeaf) putchar(' ');
	}
	if (!n->isLeaf) print_tree_core(n->chi[n->nkey]);
	printf("]");
}

void
print_tree(NODE *node)
{
	print_tree_core(node);
	printf("\n"); fflush(stdout);
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




NODE *
insert_in_parent(NODE *node, int key, NODE *right)
{
    if (node->parent == NULL) {
        NODE *root = alloc_leaf(NULL);
        root->isLeaf = false;
        root->key[0] = key;
        root->chi[0] = node;
        root->chi[1] = right;
        root->nkey = 1;
        node->parent = root;
        right->parent = root;
        return root;
    }

    NODE *parent = node->parent;
    int new_key_index;
    for (new_key_index = 0; new_key_index < parent->nkey; new_key_index++) {
        if (key < parent->key[new_key_index]) break;
    }

    for (int i = parent->nkey; i > new_key_index; i--) {
        parent->key[i] = parent->key[i-1];
        parent->chi[i+1] = parent->chi[i];
    }
    parent->key[new_key_index] = key;
    parent->chi[new_key_index+1] = right;
    parent->nkey++;

    if (parent->nkey < N) return parent;

    NODE *new_node = alloc_leaf(parent->parent);
    new_node->isLeaf = false;
    int mid = parent->nkey / 2;
    new_node->nkey = parent->nkey - mid - 1;
    parent->nkey = mid; 

    for (int i = 0; i < new_node->nkey; i++) {
        new_node->key[i] = parent->key[i+mid+1];
        new_node->chi[i] = parent->chi[i+mid+1];
        new_node->chi[i]->parent = new_node;
    }
    new_node->chi[new_node->nkey] = parent->chi[parent->nkey+1];
    new_node->chi[new_node->nkey]->parent = new_node;

    return insert_in_parent(parent->parent, parent->key[mid], new_node);
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

        // Sort the keys in the temporary structure
        for (int i = 0; i < temp.nkey - 1; i++) {
            for (int j = i + 1; j < temp.nkey; j++) {
                if (temp.key[i] > temp.key[j]) {
                    std::swap(temp.key[i], temp.key[j]);
                    std::swap(temp.chi[i], temp.chi[j]);
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

        Root = insert_in_parent(leaf, new_leaf->key[0], new_leaf);
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