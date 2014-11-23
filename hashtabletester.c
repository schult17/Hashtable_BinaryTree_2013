#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HashTable.h"

#define GET_COLLIDE 1
#define GET_EXISTS 2
#define GET_NONE 0

#define NOT_A_HASH_TABLE -1

/*
struct HashTableObjectTag
{
    int sentinel;
    struct BucketObject *table;
    unsigned int bucketCount;
    int dynamicBehaviour; 
    float expandUseFactor; 
    float contractUseFactor;
};
*/

void printHashTable(HashTablePTR table) {
    if (table == NULL) return;
    char** keys = NULL;
    unsigned int keyCount;
    int status = GetKeys(table, &keys, &keyCount);

    if (status == NOT_A_HASH_TABLE) return;
    for (unsigned int i = 0; i < keyCount; i++) {
        char* dat;
        char* key = *((keys) + i);
        FindEntry(table, key, (void**) &dat);
        printf("%s:%s\n", key, dat);
        free(key);
    }
        
    free(keys);
    }


void freeHashTableContents(HashTablePTR table) {
char** keys;
unsigned int keyCount;
GetKeys(table, (char***) &keys, &keyCount);
for (unsigned int i = 0; i < keyCount; i++) {
char* dat;
char* key = *(keys + i);
FindEntry(table, key, (void**) &dat);
free(dat);
free(key);
}
free(keys);
}


#define VER(res, name) if (res != NOT_A_HASH_TABLE) printf(name " did not check for sentinel\n")

void sentinelTest() {
    struct HashTableObjectTag* table = malloc(sizeof(int));
    table->sentinel = 0x1fee1bad; // I feel bad 
    VER(InsertEntry(table, NULL, NULL, NULL), "Insert");
    VER(DeleteEntry(table, NULL, NULL), "Delete");
    VER(FindEntry(table, NULL, NULL), "Find");
    VER(GetKeys(table, NULL, NULL), "Get");
    //VER(GetLoadFactor(table, NULL), "Load");
    VER(DestroyHashTable(&table), "Destroy");
    free(table);
}


#undef VER


int main() {
HashTablePTR table = NULL;
struct HashTableInfoTag* info = malloc(sizeof(struct HashTableInfoTag));
for(;;) {
    char cmd[80];
    printf("Command: ");
    int retval = scanf("%79s", cmd);
    float expandUseFactor = 0;
    float contractUseFactor = 0;
    int dynamicBehaviour = 0;

    if (retval <= 0) break;

    if (strcmp(cmd, "create") == 0) {
        printf("Size: ");
        int size;
        scanf("%d", &size);
/* note: this is a signed int because unsigned int
* conversion in scanf is drunk and return 0xffffffff
* for -1.
*/
    if (size < 0) {
    printf("Size cannot be negative, clamping to 0.\n");
    size = 0;
    }
    if (table != NULL) DestroyHashTable(&table);
    CreateHashTable(&table, (unsigned int) size);

    if (table == NULL) {
        printf("create returned NULL\n");
    } else {
        printf("create returned non-NULL\n");
        if (table->sentinel != (int) 0xDEADBEEF) {
            printf("FAIL: sentinel is WRONG, is %X", table->sentinel);
        }
    }
    }
    else if(strcmp(cmd, "get") == 0){
        if(GetHashTableInfo(table, info))
        {
            printf("Check your GethashTableInfo();\n");
        }
        else
        {
            printf("Bucket Count: %d\nLoadFactor: %f\nuseFactor: %f\nlargestBucketSize: %d\ndynamicBehaviour: %d\n", info->bucketCount, info->loadFactor,\
                    info->useFactor, info->largestBucketSize, info->dynamicBehaviour);
            printf("expandUseFactor: %f\ncontractUseFactor: %f\n", info->expandUseFactor, info->contractUseFactor);
        }
    }
    else if(strcmp(cmd, "change") == 0){
        
        printf("dynamicBehaviour[int] expandUseFactor[float] contractUseFactor[float]: ");
        scanf("%d %f %f", &dynamicBehaviour, &expandUseFactor, &contractUseFactor);

        if(SetResizeBehaviour(table, dynamicBehaviour, expandUseFactor, contractUseFactor ))
        {
            printf("Something went wrong\n");
        }
        else
        {
            printf("Behaviour Changed\n");
        }
    }else if (strcmp(cmd, "destroy") == 0) {
    freeHashTableContents(table);
    DestroyHashTable(&table);
        if (table != NULL) {
            printf("FAIL: destroy did not set NULL or deleting to repoint\n");
        }
    } else if (strcmp(cmd, "print") == 0) {
        printHashTable(table);
    } else if (strcmp(cmd, "set") == 0) {
    printf("Key: ");
    char key[81];
    scanf(" %80s", key);
    char* value = malloc(81 * sizeof(char));
    scanf(" %80s", value);
    char* existingData = NULL;
    int success = InsertEntry(table, key, value, (void**) &existingData);

    switch(success){
    case GET_NONE:
        printf("Inserted into blank space\n");
    break;
    case GET_COLLIDE:
        printf("Inserted after resolving hash collision\n");
    break;
    case GET_EXISTS:
        printf("Exists: %s\nInserted after removing existing data: %s\n", existingData, existingData);
        free(existingData);
    break;
    default:
        printf("InsertEntry failed\n");
        free(value);
    break;
    }
    } else if (strcmp(cmd, "read") == 0) {
        printf("Key: ");
        char key[81];
        scanf(" %80s", key);
        char* value;
        int success = FindEntry(table, key, (void**) &value);
    if (success == GET_NONE) {
        printf("%s\n", value);
    } else {
        printf("readPosition returned failure\n");
    }
    } else if (strcmp(cmd, "delete") == 0) {
        printf("Key: ");
        char key[81];
        scanf(" %80s", key);
        char* value;
        int success = DeleteEntry(table, key, (void**) &value);
    if (success == GET_NONE) {
        printf("Deleted (was %s)\n", value);
        free(value);
    } else {
        printf("DeleteValue returned failure\n");
    }
    }
    else if(strcmp(cmd, "resize") == 0){
        printf("cats\n");
    }else if (strcmp(cmd, "sentinel") == 0) {
        sentinelTest();
    } else if (strcmp(cmd, "quit") == 0) {
        break;
    } else {
        printf("Invalid command\n");
    }
}
    if (table != NULL) {
        freeHashTableContents(table);
        DestroyHashTable(&table);
    }

    free(info);
    return 0;
}
