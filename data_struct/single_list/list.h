#ifndef LIST_H_
#define LIST_H_
typedef unsigned int Item;
typedef unsigned char byte;
struct node
{
    Item item;  //数据
    struct node *next;
};

typedef struct node * List;   //List 是struct node * 型
byte ListInit(List *plist); //plist是指向struct node *型的指针（指针的指针）
void ListDel(List *plist);  //删除链表
byte ListAddItem(Item item, List *plist);
byte LIstDelItem(Item item, List *plist);
struct node* ListSearchItem(Item item, List *plist);

#endif
