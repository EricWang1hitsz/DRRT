/* jlist.cpp
 * Corin Sandford
 * Fall 2016
 */

#include <DRRT/jlist.h>
#include <DRRT/kdtreenode.h>

void JList::JlistPush( std::shared_ptr<KDTreeNode> t )
{
    JListNode* newNode = new JListNode( t );
    newNode->parent = front->parent;
    newNode->child = front;

    if( length == 0 ) {
        back = newNode;
    } else {
        front->parent = newNode;
    }

    front = newNode;
    length += 1;
}

void JList::JlistPush( Edge* e )
{
    JListNode* newNode = new JListNode( e );
    newNode->parent = front->parent;
    newNode->child = front;

    if( length == 0 ) {
        back = newNode;
    } else {
        front->parent = newNode;
    }

    front = newNode;
    length += 1;
}

void JList::JlistPush( std::shared_ptr<KDTreeNode> t, double k )
{
    JListNode* newNode = new JListNode( t );
    newNode->parent = front->parent;
    newNode->child = front;

    if( length == 0 ) {
        back = newNode;
    } else {
        front->parent = newNode;
    }

    newNode->key = k;
    front = newNode;
    length += 1;
}

void JList::JlistPush( Edge* e, double k )
{
    JListNode* newNode = new JListNode( e );
    newNode->parent = front->parent;
    newNode->child = front;

    if( length == 0 ) {
        back = newNode;
    } else {
        front->parent = newNode;
    }

    newNode->key = k;
    front = newNode;
    length += 1;
}

void JList::JlistTop( std::shared_ptr<KDTreeNode> t )
{
    if( length == 0 ) {
        // Jlist is empty
        t = std::make_shared<KDTreeNode>();
    }
    t = front->node;
}

void JList::JlistTop( Edge* e ) {
    if( length == 0 ) {
        // Jlist is empty
        e = new Edge();
    }
    e = front->edge;
}

void JList::JlistTopKey( std::shared_ptr<KDTreeNode> t, double &k )
{
    if( length == 0 ) {
        // Jlist is empty
        t = std::make_shared<KDTreeNode>();
        k = -1.0;
    }
    t = front->node;
    k = front->key;
}

void JList::JlistTopKey( Edge* e, double &k )
{
    if( length == 0 ) {
        // Jlist is empty
        e = new Edge();
        k = -1.0;
    }
    e = front->edge;
    k = front->key;
}

void JList::JlistPop( std::shared_ptr<KDTreeNode> t )
{
    if( length == 0 ) {
        // Jlist is empty
        t = std::make_shared<KDTreeNode>();
    }

    JListNode* oldTop = front;
    if( length > 1 ) {
        front->child->parent = front->parent;
        front = front->child;
    } else if( length == 1 ) {
        back = bound;
        front = bound;
    }

    length -= 1;

    oldTop->child = oldTop; // added in case Jlist nodes hang around after this
    oldTop->parent = oldTop;

    t = oldTop->node;
}

void JList::JlistPop( Edge* e )
{
    if( length == 0 ) {
        // Jlist is empty
        e = new Edge();
    }

    JListNode* oldTop = front;
    if( length > 1 ) {
        front->child->parent = front->parent;
        front = front->child;
    } else if( length == 1 ) {
        back = bound;
        front = bound;
    }

    length -= 1;

    oldTop->child = oldTop; // added in case Jlist nodes hang around after this
    oldTop->parent = oldTop;

    e = oldTop->edge;
}

void JList::JlistPopKey( std::shared_ptr<KDTreeNode> n,
                         std::shared_ptr<double> k)
{

    if( length == 0 ) {
        // Jlist is empty
        n = std::make_shared<KDTreeNode>();
        *k = -1.0;
    }

    JListNode* oldTop = front;
    if( length > 1 ) {
        front->child->parent = front->parent;
        front = front->child;
    } else if( length == 1 ) {
        back = bound;
        front = bound;
    }

    length -= 1;

    oldTop->child = oldTop;
    oldTop->parent = oldTop;

    n = oldTop->node;
    *k = oldTop->key;
}

void JList::JlistPopKey(Edge* e, std::shared_ptr<double> k)
{
    if( length == 0 ) {
        // Jlist is empty
        e = new Edge();
        *k = -1.0;
    }

    JListNode* oldTop = front;
    if( length > 1 ) {
        front->child->parent = front->parent;
        front = front->child;
    } else if( length == 1 ) {
        back = bound;
        front = bound;
    }

    length -= 1;

    oldTop->child = oldTop;
    oldTop->parent = oldTop;

    e = oldTop->edge;
    *k = oldTop->key;
}

// Removes node from the list
bool JList::JlistRemove( JListNode* node )
{
    if( length == 0 ) {
        // Node not in Jlist
        return true;
    }

    if( front == node ) {
        front = node->child;
    }
    if( back == node ) {
        back = node->parent;
    }

    JListNode* nextNode = node->child;
    JListNode* previousNode = node->parent;

    if( length > 1 && previousNode != previousNode->child ) {
        previousNode->child = nextNode;
    }
    if( length > 1 && nextNode != nextNode->parent ) {
        nextNode->parent = previousNode;
    }

    length -= 1;

    if( length == 0 ) {
        back = bound; // dummy node
        front = bound; // dummy node
    }

    node->parent = node;
    node->child = node;

    return true;
}

void JList::JlistPrint()
{
    JListNode* ptr = front;
    while( ptr != ptr->child ) {
        std::cout << ptr->node->dist << std::endl;
        ptr = ptr->child;
    }
}

Eigen::Matrix<double,Eigen::Dynamic,2> JList::JlistAsMatrix()
{
    Eigen::Matrix<double,Eigen::Dynamic,2> matrix(length,2);
    JListNode* ptr = front;
    int row_count = 0;
    while( ptr != ptr->child ) {
        matrix.row(row_count) = ptr->node->position.head(2);
        row_count++;
        ptr = ptr->child;
    }
    return matrix;
}

void JList::JlistEmpty()
{
    std::shared_ptr<KDTreeNode> temp
            = std::make_shared<KDTreeNode>();
    JlistPop(temp);
    while( temp->dist != -1.0 ) JlistPop(temp);
}

/* Test case
int main()
{
    JList L = JList();

    std::shared_ptr<KDTreeNode> a = new KDTreeNode(1);
    std::shared_ptr<KDTreeNode> b = new KDTreeNode(2);
    std::shared_ptr<KDTreeNode> c = new KDTreeNode(3);
    std::shared_ptr<KDTreeNode> d = new KDTreeNode(4);

    std::cout << "Pushing three nodes onto list" << std::endl;
    L.JlistPush(a);
    L.JlistPush(b);
    L.JlistPush(c);

    std::cout << "Printing list" << std::endl;
    L.JlistPrint();

    std::cout << "Emptying list" << std::endl;
    L.JlistEmpty();

    std::cout << "-" << std::endl;

    std::cout << "Printing list" << std::endl;
    L.JlistPrint();

    std::cout << "-" << std::endl;

    std::cout << "Pushing five nodes onto list" << std::endl;
    L.JlistPush(a);
    L.JlistPush(b);
    L.JlistPush(c);
    L.JlistPush(b);
    L.JlistPush(a);

    JListNode* cc = L.front;

    std::cout << "Printing list" << std::endl;
    L.JlistPrint();

    std::cout << "-" << std::endl;

    std::cout << "Removing front node from list" << std::endl;
    L.JlistRemove(cc);

    std::cout << "Pushing node onto list" << std::endl;
    L.JlistPush(d);

    std::cout << "Printing list" << std::endl;
    L.JlistPrint();
    return 0;
}*/
