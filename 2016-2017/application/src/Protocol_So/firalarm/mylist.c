#include "mylist.h"
extern  char *Catch_Path;
/*
	读取链表数据上传										读取控制器数据写入链表
		   |										Head			|
		   |		读取一个删除一个节点,头指针后移----->			|
		   ListHead-------------------------------------------------InsertDate


*/
/*Init a NULL list , return head point*/

application_info init_rxbuffer_data_list(void) 		
{
	application_info app;
    if ((app= (application_info)malloc(sizeof(listnode))) == NULL)
    {
        printf("malloc error\n");
        return NULL;
    }
    app->next = NULL;
 
    return app;
}
//销毁rx buffer 数据链表
void destory_rxbuffer_data_list(application_info head)
{
	application_info p = head;
	while (head->next)
	{
		p = head->next;
		head->next = p->next;
		free(p);
	}
	free(p);
    p=NULL;
}

//链表按顺序插入
void insert_rxbuffer_data(application_info head , struct thingWorxData data)
{
    application_info p, q;
    p = head;
    while (p -> next != NULL)
        p = p->next;

    q = (application_info)malloc(sizeof(listnode));
    if(NULL == q)
        return;
    q->data=data;
    p->next=q;
    q->next = NULL;
    return;
}

//插入链表头 (火警)
void insert_rxbuffer_data_head(application_info head, struct thingWorxData data)
{
    application_info temp, q;
    if(head == NULL){
        fprintf(stderr, "No list could use\n"); 
        return; 
    }

    temp = (application_info)malloc(sizeof(listnode));
    if(temp == NULL){
        fprintf(stderr, "malloc faild while insert fire to first node\n");
    }
    if(NULL == head->next)
    {
        q=head;
        temp->data=data;
        q->next=temp;
        temp->next=NULL;
    }
    else
    {
        q=head->next->next;
        temp->data=data;
        temp->next=q;
        head->next->next=temp;
    }
}


//查找链表头结点的next是否有数据 
application_info get_rxbuffer_data(application_info p)
{
	if(p->next!=NULL)
	{
		p=p->next;
		return p;
	}
	return NULL;
}

//链表删除的节点 将头指针指向head->next=head->next->next
void delete_rxbuffer_data(application_info head )
{
    application_info p;
    if (head->next != NULL)
    {
        p = head->next;
        head->next = head->next->next;
        free(p);
        p = NULL;
     }
}

#if 0
void get_catch_file(application_info head )
{
	twData ptr;
	char cmd[30]={0};
	FILE *ftmp=NULL;
	FILE *fp=fopen(Catch_Path , "r");
	if(NULL == fp)
		return ;
	if(!feof(fp))
	{
		if(!fread(&ptr , sizeof(ptr) , 1 , fp))
		{
			fclose(fp);
			TW_LOG(TW_ERROR,"catch read faild ptr=%d",sizeof(ptr));
			return ;
		}
		insert_rxbuffer_data(head , ptr);
	}
	else
		TW_LOG(TW_ERROR,"tmp file open faild");
	ftmp = fopen("./tmp" , "w");
	if(NULL == ftmp)
	{
		TW_LOG(TW_ERROR,"tmp file open faild");
		return;
	}
	while(!feof(fp))
	{
		if(!fread(&ptr , sizeof(ptr) , 1 , fp))
		{
			TW_LOG(TW_ERROR,"*catch file read faild");
			fclose(fp);
			fclose(ftmp);
			remove(Catch_Path);
			sprintf(cmd , "mv tmp %s" , Catch_Path);
			system(cmd);
			return ;
		}
		else 
		{
			if(!fwrite(&ptr , sizeof(twData) , 1, ftmp))	
			{
				TW_LOG(TW_ERROR,"tmp file write faild");
				fclose(fp);
				fclose(ftmp);
				return ;
			}
		}
	}
	remove(Catch_Path);
	sprintf(cmd , "mv tmp %s" , Catch_Path);
	system(cmd);
}
#endif
void get_catch_file(application_info head )
{
	twData ptr;
	FILE *fp=fopen(Catch_Path , "r");
	if(NULL == fp)
		return;
	while(!feof(fp))
	{
		if(fread(&ptr , sizeof(twData) , 1 , fp))
		{
			insert_rxbuffer_data(head , ptr);
		}
		else
		{
			break;
		}
	}
	fclose(fp);
	remove(Catch_Path);
}
