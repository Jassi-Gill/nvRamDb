#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char *key;
    char **strtNvmAddr;
    int *size;
    char *ACL;
    int repCnt;
    char *repStore;
    int version;
} strInode;

// An AVL tree node
struct Node
{
    strInode *inode;
    struct Node *left;
    struct Node *right;
    int height;
};

// Height of the tree
int height(struct Node *N)
{
    if (N == NULL)
        return 0;
    return N->height;
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

/* Allocates a new node with the key and NULL left and right pointers. */
struct Node *newNode(char *key)
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));

    node->inode->key = (char)malloc(sizeof(char) * 32);
    strcpy(node->inode->key, key);
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return (node);
}

// Rright rotate subtree rooted with y
struct Node *rightRotate(struct Node *y)
{
    struct Node *x = y->left;
    struct Node *T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights
    y->height = max(height(y->left),
                    height(y->right)) +
                1;
    x->height = max(height(x->left),
                    height(x->right)) +
                1;

    // Return new root
    return x;
}

// Left rotate subtree rooted with x
struct Node *leftRotate(struct Node *x)
{
    struct Node *y = x->right;
    struct Node *T2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = T2;

    // Update heights
    x->height = max(height(x->left),
                    height(x->right)) +
                1;
    y->height = max(height(y->left),
                    height(y->right)) +
                1;

    // Return new root
    return y;
}

// Balance factor of node N
int getBalance(struct Node *N)
{
    if (N == NULL)
        return 0;
    return height(N->left) - height(N->right);
}

// Recursive function to insert a key in the subtree rooted
// with node and returns the new root of the subtree.
struct Node *insert(struct Node *node, char *key)
{
    /* 1.  Perform the normal BST insertion */
    if (node == NULL)
        return (newNode(key));

    if (key < node->inode->key)
        node->left = insert(node->left, key);
    else if (key > node->inode->key)
        node->right = insert(node->right, key);
    else // Equal keys are not allowed in BST
        return node;

    /* 2. Update height of this ancestor node */
    node->height = 1 + max(height(node->left), height(node->right));

    /* 3. Get the balance factor of this ancestor node to check whether this node became unbalanced */
    int balance = getBalance(node);

    // If this node becomes unbalanced, then there are 4 cases

    // Left Left Case
    if (balance > 1 && key < node->left->inode->key)
        return rightRotate(node);

    // Right Right Case
    if (balance < -1 && key > node->right->inode->key)
        return leftRotate(node);

    // Left Right Case
    if (balance > 1 && key > node->left->inode->key)
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left Case
    if (balance < -1 && key < node->right->inode->key)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    /* Return the (unchanged) node pointer */
    return node;
}

// Print preorder traversal & height of every node of the tree.
void preOrder(struct Node *root)
{
    if (root != NULL)
    {
        printf("%d ", root->inode->key);
        preOrder(root->left);
        preOrder(root->right);
    }
}

int main()
{
    struct Node *root = NULL;

    root = insert(root, "10");
    root = insert(root, "20");
    root = insert(root, "30");
    root = insert(root, "40");
    root = insert(root, "50");
    root = insert(root, "25");

    printf("Preorder traversal : \n");
    preOrder(root);

    return 0;
}