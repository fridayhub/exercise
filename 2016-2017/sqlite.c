#include <stdio.h>   
#include <stdlib.h>
#include <string.h>   
#include <sqlite3.h>   

int watersql[16][6];
char watertype[16][18];
int row = 0; //record row
  
/* callback函数中： 
 * void*,  Data provided in the 4th argument of sqlite3_exec()
 * int,    The number of columns in row 
 * char**, An array of strings representing fields in the row
 * char**  An array of strings representing column names
 */  
int callback(void *data, int nr, char **values, char **names)  
{  
    int i;  
//    fprintf(stderr, "%s:\n", (const char*)data);
    for(i = 0; i < nr; i++){
        //printf("%s = %s\n", names[i], values[i] ? values[i] : "NULL");
        printf("%s = %s", names[i], values[i] ? values[i] : "NULL");
        watersql[row][i] = atoi(values[i]);
    }
    row++;
    //printf("\n");
    return 0;  //callback函数正常返回0   
}  
  
  
int main()  
{  
    char *CrateSql, *InsertSql, *SelectSql;  
    sqlite3 *db;  
    int rc;
    char *zErrMsg = 0;
    const char* data = "Callback function called";

    int i = 0, j = 0;
  
    rc = sqlite3_open("/data/.watersys.db", &db);  //打开（或新建）一个数据库   
    if( rc ){
        fprintf(stderr, "Can't open databases: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        fprintf(stdout, "Opened database successfully\n");
    }
    
    /* Crate SQL statement */
    CrateSql = "CREATE TABLE watersys(" \
        "channel     INTEGER PRIMARY KEY NOT NULL, "\
        "type        INTEGER NOT NULL," \
        "switch      INTEGER NOT NULL DEFAULT 0," \
        "range       TEXT," \
        "uplimite    TEXT," \
        "lowlimite   TEXT," \
        "rate        TEXT," \
        "startvalue  TEXT DEFAULT '4000'," \
        "location    varchar(100))";
    rc = sqlite3_exec(db, CrateSql, callback, 0, &zErrMsg);  
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL crate error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table crate successfully\n");
    }

    InsertSql = "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(1, 1, 1, 50, 49, 0, 7, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(2, 2, 0, 16000, 15999, 12000, 6, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(3, 1, 1, 50, 49, 1, 4, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(4, 2, 0, 17000, 14000, 10000, 7, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(5, 1, 1, 50, 49, 0, 5, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(6, 2, 0, 16000, 15999, 12000, 6, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(7, 1, 1, 50, 49, 1, 4, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(8, 2, 0, 17000, 14000, 10000, 7, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(9, 1, 1, 50, 49, 0, 5, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(10, 2, 0, 16000, 15999, 12000, 6, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(11, 1, 1, 50, 49, 1, 4, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(12, 2, 0, 17000, 14000, 10000, 7, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(13, 1, 1, 50, 49, 0, 5, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(14, 2, 0, 16000, 15999, 12000, 6, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(15, 1, 1, 50, 49, 1, 4, '1');" \
                 "INSERT INTO watersys(channel, type, switch, range, uplimite, lowlimite, rate, location)" \
                 "VALUES(16, 2, 0, 17000, 14000, 10000, 7, '1');";

    rc = sqlite3_exec(db, InsertSql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL insert error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Records crate successfully\n");
    }

    /* select sql statement*/
    SelectSql = "SELECT * from watersys";
    rc = sqlite3_exec(db, SelectSql, callback, (void*)data, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL select error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Select successfully\n");
    }
#if 0

    for(i = 0; i < 16; i++){
        for(j = 0; j < 7; j++){
            printf("%d ", watersql[i][j]);
        }
        printf("\n");
    }

#endif
  
    sqlite3_close(db); //关闭数据库   
    return 0;  
}  
