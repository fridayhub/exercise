#include <stdio.h>
#include <stdlib.h>
#include "list.h"

//===============================================
//将item拷贝进节点，此函数可根据Item的不同进行修改
//===============================================
static void CopyItem(Item item, struct node* pnode)
{
    pnode->item = item;
}

//=========================================================
//判断item是否和节点中的item相等,此函数可根据Item的不同修改  
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

//======================
//初始化链表，带哨兵元素
//======================
byte ListInit(List *plist)
{
    struct node * pnew;
    pnew = (struct node*)malloc(sizeof(struct node));
    if(pnew == 0)
        return 0; //创建失败

    pnew->next = pnew;
    *plist = pnew;
    return 1;
}

//=================
//删除链表
//=================
void ListDel(List *plist)
{
    struct node* pscan;
    struct node* pdel;
    pscan = *plist;
    while(pscan->next != *plist) //不是哨兵元素
    {
        pdel = pscan;
        pscan = pscan->next;  //psacn  下移
        free(pdel);
    }
    free(pscan); //删除哨兵元素
}

//========================
//添加元素到链表尾
//成功，返回1；失败，返回0
//========================
byte ListAddItem(Item item, List *plist)
{
    struct node *pnew;
    struct node *pscan = *plist;
    pnew = (struct node *)malloc(sizeof(struct node));
    if(pnew == 0)
        return 0;

    CopyItem(item, pnew);
    pnew->next = *plist;

    while(pscan->next != *plist)
        pscan = pscan->next;

    pscan->next = pnew;

    return 1;
}
