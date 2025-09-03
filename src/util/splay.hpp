#pragma once
#include <iostream>

class SplayTree
{
private:
    struct Node
    {
        int x, y;
        Node *left;
        Node *right;
        Node *parent;

        Node(int xx, int yy)
            : x(xx), y(yy), left(nullptr), right(nullptr), parent(nullptr) {}
    };

    Node *root = nullptr;

    static bool isLeftChild(Node *node)
    {
        return node->parent && node->parent->left == node;
    }

    static bool isRightChild(Node *node)
    {
        return node->parent && node->parent->right == node;
    }

    void rotate(Node *node)
    {
        Node *p = node->parent;
        if (!p)
            return;

        Node *g = p->parent;
        bool nodeIsLeftChild = (node == p->left);

        if (nodeIsLeftChild)
        {
            p->left = node->right;
            if (node->right)
            {
                node->right->parent = p;
            }
            node->right = p;
            p->parent = node;
        }
        else
        {
            p->right = node->left;
            if (node->left)
            {
                node->left->parent = p;
            }
            node->left = p;
            p->parent = node;
        }

        node->parent = g;
        if (g)
        {
            if (p == g->left)
                g->left = node;
            else
                g->right = node;
        }
    }

    void splay(Node *node)
    {
        if (!node)
            return;
        while (node->parent)
        {
            Node *p = node->parent;
            Node *g = p->parent;
            if (!g)
            {
                rotate(node);
            }
            else
            {
                bool nodeIsLeft = (node == p->left);
                bool pIsLeft = (p == g->left);
                if (nodeIsLeft == pIsLeft)
                {
                    rotate(p);
                    rotate(node);
                }
                else
                {
                    rotate(node);
                    rotate(node);
                }
            }
        }
        root = node;
    }

    Node *insertBST(int x, int y)
    {
        if (!root)
        {
            root = new Node(x, y);
            return root;
        }

        Node *cur = root;
        Node *parent = nullptr;

        while (cur)
        {
            parent = cur;
            if (x < cur->x)
            {
                cur = cur->left;
            }
            else if (x > cur->x)
            {
                cur = cur->right;
            }
            else
            {
                return cur;
            }
        }

        Node *newNode = new Node(x, y);
        newNode->parent = parent;
        if (x < parent->x)
            parent->left = newNode;
        else
            parent->right = newNode;

        splay(newNode);
        return newNode;
    }

    Node *searchBSTByX(int x)
    {
        Node *cur = root;
        Node *lastAccess = nullptr;
        while (cur)
        {
            if (cur->x < x)
            {
                cur = cur->right;
            }
            else
            {
                lastAccess = cur;
                cur = cur->left;
            }
        }
        splay(lastAccess);
        return lastAccess;
    }

    Node *searchBSTByY(int y)
    {
        Node *cur = root;
        Node *lastAccess = nullptr;
        while (cur)
        {
            if (cur->y > y)
            {
                cur = cur->left;
            }
            else
            {
                lastAccess = cur;
                cur = cur->right;
            }
        }
        splay(lastAccess);
        return lastAccess;
    }

    void rangeInorderTraversal(Node *cur, int low, int high, vector<pair<int, int>> &ret)
    {
        if (!cur)
            return;

        if (cur->x >= low)
        {
            rangeInorderTraversal(cur->left, low, high, ret);
        }

        if (cur->x >= low && cur->x <= high)
        {
            ret.push_back(make_pair(cur->x, cur->y));
        }

        if (cur->x <= high)
        {
            rangeInorderTraversal(cur->right, low, high, ret);
        }
    }

    Node *removeRoot(Node *oldRoot)
    {
        if (!oldRoot)
            return nullptr;

        Node *leftSub = oldRoot->left;
        Node *rightSub = oldRoot->right;
        if (leftSub)
            leftSub->parent = nullptr;
        if (rightSub)
            rightSub->parent = nullptr;
        delete oldRoot;

        if (!leftSub)
            return rightSub;

        Node *maxNode = leftSub;
        while (maxNode->right)
        {
            maxNode = maxNode->right;
        }
        splay(maxNode);
        maxNode->right = rightSub;
        if (rightSub)
            rightSub->parent = maxNode;
        return maxNode;
    }

    Node *findExactByX(int x)
    {
        Node *cur = root;
        Node *lastAccess = nullptr;

        while (cur)
        {
            lastAccess = cur; // record last access
            if (x == cur->x)
            {
                break;
            }
            else if (x < cur->x)
            {
                cur = cur->left;
            }
            else
            {
                cur = cur->right;
            }
        }

        if (cur)
        {
            splay(cur);
            return cur;
        }
        else
        {
            if (lastAccess)
                splay(lastAccess);
            return nullptr;
        }
    }

    void inorder(Node *cur) const
    {
        if (!cur)
            return;
        inorder(cur->left);
        std::cout << "(" << cur->x << "," << cur->y << ") ";
        inorder(cur->right);
    }

    void destroySubtree(Node *node)
    {
        if (!node)
            return;
        destroySubtree(node->left);
        destroySubtree(node->right);
        delete node;
    }

public:
    SplayTree() : root(nullptr) {}

    ~SplayTree()
    {
        destroySubtree(root);
    }

    void insert(int x, int y)
    {
        insertBST(x, y);
    }

    int searchByX(int x)
    {
        Node *found = searchBSTByX(x);
        return found->x;
    }

    int searchByY(int y)
    {
        Node *found = searchBSTByY(y);
        return found->x;
    }

    void getRange(int low, int high, vector<pair<int, int>> &ret)
    {
        rangeInorderTraversal(root, low, high, ret);
    }

    bool remove(int x)
    {
        Node *target = findExactByX(x);
        if (!target || root->x != x)
        {
            return false;
        }
        root = removeRoot(root);
        return true;
    }

    void inorderPrint() const
    {
        inorder(root);
        std::cout << std::endl;
    }
};
