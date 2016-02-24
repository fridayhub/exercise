#ifndef D_LIST_H
#define D_LIST_H

typedef unsigned int word;
typedef unsigned char byte;
typedef int Item;

struct node
{
    Item item;
    struct node *prev;
    struct node *next;
};

struct list
{
    struct node* head;  //链表头
    word count;         //链表元素个数
    word max;           //链表允许的最大元素个数
};

typedef struct list List;
byte ListInit(List *plist, word max);  // plist是指向struct node *型的指针（指针的指针）
void ListDel(List *plist);                     //删除链表
byte ListAddItem(Item item, List *plist);       //添加元素
byte ListDelItem(Item item, List *plist);       //删除元素
struct node* ListSearchItem(Item item, List *plist);  // 查找元素

#endif
