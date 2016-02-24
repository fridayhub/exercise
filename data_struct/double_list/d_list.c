#include <stdio.h>
#include <stdlib.h>
#include "d_list.h"

//===============================================
//将item拷贝进节点，此函数可根据Item的不同进行修改
//===============================================
static void CopyItem(Item item, struct node* pnode)
{
    pnode->item = item;
}

//=========================================================
//判断item是否和节点中的item相等，此函数可根据Item的不同修改
//=========================================================
static byte ItemEqual(Item item, struct node* pnode)
{
    byte result;
    if(pnode->item == item)
        result = 1;
    else 
        result = 0;
    return result;
}

//================================================
//初始化链表
//plist:指向链表的指针， num:链表允许的最大元素个数
//成功返回1，失败返回0
//=================================================
byte ListInit(List *plist, word max)
{
    struct node *pnew;
    pnew = (struct node *)malloc(sizeof(struct node));

    if(pnew == 0)
        return 0;

    pnew->next = pnew;
    pnew->prev = pnew;  //哨兵元素

    plist->max = max;
    plist->count = 0;
    plist->head = pnew;

    return 1;
}

//========================================
//添加元素到链表尾部
//返回值：0：链表已达到最大值,不能再添加元素
//        1：系统无可用内存,不能再添加元素
//        2：添加元素成功
//=========================================
byte ListAddItem(Item item, List *plist)
{
    struct node* pnew;
    (plist->count)++;
    if(plist->count > plist->max) //大于链表最大个数后不能再添加元素
    {
        (plist->count)--;
        return 0;
    }

    pnew = (struct node *)malloc(sizeof(struct node));
    if(pnew == 0)  //分配内存失败
    {
        (plist->count)--;
        return 1;
    }

    CopyItem(item, pnew);
    pnew->next = plist->head;   //指向哨兵节点
    pnew->prev = plist->head->prev; //指向末尾节点
    plist->head->prev->next = pnew; //末尾节点的下一个指向新建立的节点
    plist->head->prev = pnew; //哨兵元素的前趋指向新建立的节点

    return 2;
}

//======================================
//搜索元素
//链表中存在元素，则返回第一个节点的元素
//若不存在或者链表为空，则返回0
//======================================
struct node* ListSearchItem(Item item, List *plist)
{
    struct node *pscan;

    if(plist->count == 0)  //空链表
        return (struct node*)0;

    pscan = plist->head->next;

    while(pscan->next != plist->head) //不等于哨兵元素，即没到链表尾
    {
        pscan = pscan->next;
        if(ItemEqual(item, pscan))
            return pscan;
    }
    if(ItemEqual(item, pscan))  //最后一个节点
        return pscan;
    else
        return (struct node*)0;
}

//==========================================
//删除元素
//若链表为空，则返回0
//若元素不在链表中，则返回1
//若成功删除，则返回
//==========================================
byte ListDelItem(Item item, List *plist)
{
    struct node *pdel;

    if(plist->count == 0)
        return 0;

    pdel = ListSearchItem(item, plist);
    if(pdel == 0)
        return 1;

    //存在，删除
    (plist->count)--;
    pdel->prev->next = pdel->next;
    pdel->next->prev = pdel->prev;
    free(pdel);

    return 2;
}

//=========================================
//打印链表中元素
static int ListDispItem(List *plist)
{
    struct node *pdisp;

    if(plist->count == 0) //空链表
    {
        printf("list empted\n");
        return 0;
    }

    pdisp = plist->head->next;
    while(pdisp->next != plist->head) //不等于哨兵元素，即没到链表尾
    {
        printf("%d  ", pdisp->item);
        pdisp = pdisp->next;
    }

    printf("%d  ", pdisp->item);

    return 0;
}

int main()
{
    List mylist;
    struct node *psearch;
    int i;
    word max = 10;
    Item temp;
    byte add_flag;
    ListInit(&mylist, max); 
    printf("please input 10 numbers:\n");
    for(i = 0; i < 10; i++)
    {
       scanf("%d", &temp);
       printf("%d\n", i);
       add_flag = ListAddItem(temp, &mylist);
       if(add_flag == 0)
       {
           printf("List has been full!\n");
           exit(0);
       }else if(add_flag == 1)
       {
           printf("No enough memeory!\n");
           exit(0);
       }else if(add_flag == 2)
           printf("add successful!\n");
    }
    ListDispItem(&mylist);

    psearch = ListSearchItem(4, &mylist);
    if(psearch == 0)
        printf("can't find item\n");
    else
        printf("find item %d\n", psearch->item);

    return 0;
}
