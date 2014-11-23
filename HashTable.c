/*
 * Created by Tanner Young-Schultz
 * Started March 13, 2014
 * A .c file for Hash Table Project, part B
 */

#include "HashTable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//simple struct (pointed to by one bucket struct above), containts a string key (name),
//a void pointer (to the data of that key) and a next pointer for linking
struct BucketItem
{
    struct BucketItem *left;
    struct BucketItem *right;
    char *key;
    void *data;
};

//pointer to individual buckets, has a bucket range to hold size of seperate chain, 
//and pointer to head of a linked list (for collisions), having three structs was 
//a personal engineering design decision and I did it because it was clearer and cleaner to me
struct BucketObject
{
    unsigned int bucket_range;
    struct BucketItem *bucket_root;
};

//Old HashTable object, plys a few things I felt were neccessary. deleteHandle is a boolean for 
//deleting in the resize vs entire table, the rest is just for dynamic behaviour
struct HashTableObjectTag
{
    int sentinel;
    struct BucketObject *table;
    unsigned int bucketCount;
    int dynamicBehaviour; 
    int deleteHandle;
    float expandUseFactor; 
    float contractUseFactor;
};

//this function is still funny lol
static unsigned int Hashish( char *zard, unsigned int range )
{
    //easy hash function, may change later
    unsigned int key = 0;

    int index = 0;
    while(zard[index] != '\0')
    {
        key += (unsigned int)zard[index];
        index++;
    }

    key = key % range;

    return key;
}

//helper function that destroys a single bucket (tree)
static void destroyBucket(struct BucketItem* leaf)
{
    if(leaf != NULL)
    {
        destroyBucket(leaf->left);
        destroyBucket(leaf->right);
        free(leaf->key);
        free(leaf);
    }
}

//helper function that creates a new tree Node, with extra return statements for 
//case of not enough memory
static struct BucketItem* NewTreeNode(char* key, void* data)
{
    struct BucketItem* new_node = malloc(sizeof(struct BucketItem));
    if(new_node == NULL){return new_node;}

    new_node->key = malloc(strlen(key) + 1);
    if(new_node->key == NULL){return new_node;}

    strcpy(new_node->key, key);
    new_node->data = data;
    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

//helper function that finds smallest in right subtree(for deletetion of parent with 2 children)
static char* findSmallest(struct BucketItem* root)
{
    if(root->left != NULL)
    {
        return findSmallest(root->left);
    }
    else
    {
        return root->key;
    }
}


static void ResizeHashTable( HashTablePTR *hashTableHandle, HashTableInfo *currentInfo )
{
    //game time, new bucket size variable....initially set to old bucket size(will change later, alligator)
    unsigned int newBucketCount = 0;
    newBucketCount = currentInfo->bucketCount;

    //for loop that will count buckets with more than 1, for useFactor
    unsigned int bucketsWithOne = 0;
    for(int bucket = 0; bucket < currentInfo->bucketCount; bucket++)
    {
        if(((*hashTableHandle)->table[bucket]).bucket_range > 0)
        {
            bucketsWithOne += 1;
        }
    }

    //loops that will decrease or increase newBucket count until bucketsWithOne/newBucketCount (mock 'useFactor') is appropriate
    //in second for loop, the check of newBucketCount's size, is to make sure of atleast one bucket at all times!(clamping @ 1)
    while((float)bucketsWithOne/(float)newBucketCount > currentInfo->expandUseFactor)  {newBucketCount++;}
    while((float)bucketsWithOne/(float)newBucketCount < currentInfo->contractUseFactor && newBucketCount > 1)  {newBucketCount--;}

    //get all current keys
    char** keys = NULL;
    unsigned int* keyCount = malloc(sizeof(unsigned int));
    GetKeys(*hashTableHandle, &keys, keyCount);

    //get all current data, data[i] will correspond with keys[i], since keys[i] is being sent as search item
    void** data = malloc(sizeof(void*) * (*keyCount));
    for(int i = 0; i < *keyCount; i++)
    {
        FindEntry(*hashTableHandle, keys[i], &data[i]);
    }

    //delete hashTable with deleteHandle off, ie. don't free the handle (*hashTableHandle), just free until the table (buckets) are freed
    (*hashTableHandle)->deleteHandle = 0;
    DestroyHashTable(hashTableHandle);
    
    //'realloc', just re-malloc table to new size and change bucket size to new size and increase bucket size (variable in struct)
    //no need for memory check here, says foster, thats why I made this function void... also set bucketCount variable to new count
    ((*hashTableHandle)->table) = malloc(sizeof(struct BucketObject) * newBucketCount);
    (*hashTableHandle)->bucketCount = newBucketCount;

    //reinitialize buckets(like in create hashTable) to NULL and range to 0
    for(int i = 0; i < newBucketCount; i++)
    {
        ((*hashTableHandle)->table[i]).bucket_range = 0;
        ((*hashTableHandle)->table[i]).bucket_root = NULL;
    }

    //turn OFF dynamic behaviour for re-insert (so that we dont shrink our newly expanded table)
    SetResizeBehaviour(*hashTableHandle, 0, currentInfo->expandUseFactor, currentInfo->contractUseFactor);

    //re-insert key and data, dummy pointer is just to give InsertEntry all of its parameters (there will be no identical keys)
    void** existDummy = NULL;
    for(int i = 0; i < *keyCount; i++)
    {
        InsertEntry(*hashTableHandle, keys[i], data[i], existDummy);
    }

    //reset behaviour to old behaviour: turn back ON dynamicBehaviour as well as deleteHandle boolean
    SetResizeBehaviour(*hashTableHandle, 1, currentInfo->expandUseFactor, currentInfo->contractUseFactor);
    (*hashTableHandle)->deleteHandle = 1;

    //free other stuff used in function: keys in key array, the key array itself, and the data array (not individual data, just array)
    for(int i = 0; i < *keyCount; i++){free(keys[i]);}
    free(keys);
    free(keyCount);
    free(data);
    
    //veryyy nice
}

int CreateHashTable( HashTablePTR *hashTableHandle, unsigned int initialSize )
{
    //this creates the Table to the intial size given, the size may grow or shrink
    //if NULL pointer, allocation failed.
    (*hashTableHandle = malloc(sizeof(HashTableObject)));
    
    //memory check
    if ((*hashTableHandle) == NULL) {return -1;}
    
    //initialize pointer to entire hash table
    (*hashTableHandle)->sentinel = (int)0xDEADBEEF;
    (*hashTableHandle)->bucketCount = initialSize;
    (*hashTableHandle)->expandUseFactor = (float)0.7;
    (*hashTableHandle)->contractUseFactor = (float)0.2;
    (*hashTableHandle)->dynamicBehaviour = 1;  
    (*hashTableHandle)->deleteHandle = 1;

    //allocate space to initialSize times the size of a bucket, if not enough room
    //(malloc returns NULL), return -1 (fail)
    (*hashTableHandle)->table = malloc(sizeof(struct BucketObject) * initialSize);
    //memory check
    if((*hashTableHandle)->table == NULL) {return -1;}

    //for loop that goes through all buckets to initialize the buckets
    for(int i = 0; i < initialSize; i++)
    {
        //iterating through different buckets and initializing there range to 0 (nothing in it)
        //and the buckets values (item struct), to NULL (nothing in it yet)
        ((*hashTableHandle)->table[i]).bucket_range = 0;
        ((*hashTableHandle)->table[i]).bucket_root = NULL;
    }
    
    //great success!
    return 0;
}

int GetHashTableInfo( HashTablePTR hashTable, HashTableInfo *pHashTableInfo )
{
    //checks if hashTable is a valid hashTableObject
    if (hashTable == NULL || *((int *)hashTable) != (int)0xDEADBEEF) {return -1;}

    //big ol' for loop that gets largestBucketSize(useless?), start of useFactor variable and counts keys
    unsigned int count = 0;
    unsigned int largest = 0;
    float useFactor = 0;

    for(int bucket = 0; bucket < hashTable->bucketCount; bucket++)
    {
        if((hashTable->table[bucket]).bucket_range > largest)
        {
            largest = (hashTable->table[bucket]).bucket_range;
        }

        if((hashTable->table[bucket]).bucket_range > 0)
        {
            useFactor += 1;
        }

        count += (hashTable->table[bucket]).bucket_range;
    }

    //assign hashTable info to tag
    pHashTableInfo->bucketCount = hashTable->bucketCount;
    pHashTableInfo->loadFactor = (float)count / (float)hashTable->bucketCount;
    pHashTableInfo->useFactor = useFactor/(float)hashTable->bucketCount;
    pHashTableInfo->largestBucketSize = largest;
    pHashTableInfo->dynamicBehaviour = hashTable->dynamicBehaviour;
    pHashTableInfo->expandUseFactor = hashTable->expandUseFactor;
    pHashTableInfo->contractUseFactor = hashTable->contractUseFactor;

    //yeeha!
    return 0;
}

int SetResizeBehaviour( HashTablePTR hashTable, int dynamicBehaviour, float expandUseFactor, float contractUseFactor )
{
    //checks if hashTable is a valid hashTableObject
    if (hashTable == NULL || *((int *)hashTable) != (int)0xDEADBEEF) {return -1;}

    //contractFactor cannot be greater than expandFactor
    if(contractUseFactor >= expandUseFactor) {return 1;}

    //set behaviour given
    hashTable->dynamicBehaviour = dynamicBehaviour;
    hashTable->expandUseFactor = expandUseFactor;
    hashTable->contractUseFactor = contractUseFactor;

    return 0;
}

int DestroyHashTable( HashTablePTR *hashTableHandle )
{
    //checks if hashTable is a valid hashTableObject
    if (*hashTableHandle == NULL || *((int *)(*hashTableHandle)) != (int)0xDEADBEEF) {return -1;}

    //send each root pointer to deleteBucket to delete it...
    for(int bucket = 0; bucket < (*hashTableHandle)->bucketCount; bucket++)
    {
        destroyBucket(((*hashTableHandle)->table[bucket]).bucket_root);
    }

    //free the table (array of buckets)
    free((*hashTableHandle)->table);

    //conditionally free the entire table only if NOT deleteing for reinserting...
    if((*hashTableHandle)->deleteHandle)
    {
        free(*hashTableHandle);
        *hashTableHandle = NULL;
    }

    return 0;
}

//just a quick function for checking if resize is neccessary to make things cleaner, returns true(1) or false(0)
static int checkResize(HashTablePTR hashTable, struct HashTableInfoTag* currInfo, int isInsert)
{
    //put this in (+ the boolean parameter 'isInsert') to detect for expanding(insert) or contract(deletion)
    int checkResizeFactor = 0;   //either expand or contract factor

    if(isInsert){checkResizeFactor = currInfo->useFactor > currInfo->expandUseFactor;}
    else{checkResizeFactor = currInfo->useFactor < currInfo->contractUseFactor;}

    int checkDynamicBehaviour = currInfo->dynamicBehaviour;
    int checkBucketCount = hashTable->bucketCount > 0;

    int toResize = 0;
    if(checkDynamicBehaviour && checkBucketCount && checkResizeFactor){toResize = 1;}
    else{toResize = 0;}

    return toResize;
}

static int insertNode(struct BucketItem** root,  char *key, void *data, void **previousDataHandle )
{
    //this will return the correct return value to the main insert function of our program..
    if(*root == NULL)
    {
        //traverses until a NULL node and adds a new Node
        *root = NewTreeNode(key, data);
        //memory check
        if(*root == NULL || (*root)->key == NULL){return -2;}
        else{return 1;}
    }
    else if(strcmp(key, (*root)->key) < 0)
    {
        //recursive left
        return insertNode(&((*root)->left), key, data, previousDataHandle);
    }
    else if(strcmp(key, (*root)->key) > 0)
    {
        //recursive right
        return insertNode(&((*root)->right), key, data, previousDataHandle);
    }
    else
    {
        //case of strcmp = 0.. ie. matched data
        *previousDataHandle = (*root)->data;
        (*root)->data = data;
        return 2;
    }
}

int InsertEntry( HashTablePTR hashTable, char *key, void *data, void **previousDataHandle )
{
    //checks if hashTable is a valid hashTableObject
    if (hashTable == NULL || *((int*)hashTable) != (int)0xDEADBEEF) {return -1;}

    //gets index of bucket from hash
    unsigned int bucket_index = Hashish(key, hashTable->bucketCount);

    //variable to return
    int checkStatus = 0;
    //special case: no collision (making the root of the bucket)
    if((hashTable->table[bucket_index]).bucket_root == NULL)
    {
        //make a new treeNode, if block is a memory check, if there was enough memory
        struct BucketItem* new_root = NewTreeNode(key, data);
        if(new_root == NULL || new_root->key == NULL)
        {
            checkStatus = -2;
        }
        else
        {
            //point root pointer to new root and increase bucket range by one and set return variable to sucees(no collision)
            (hashTable->table[bucket_index]).bucket_root = new_root;
            (hashTable->table[bucket_index]).bucket_range = 1;
            checkStatus = 0;
        }
    }
    //collision
    else
    {
        //temp root handle to send to insertNode, for new node to be inserted in tree
        struct BucketItem** rootPTR = &((hashTable->table[bucket_index]).bucket_root);

        //attempt to insert and increase bucket range upon successful insert (1)
        checkStatus = insertNode(rootPTR, key, data, previousDataHandle);
        if(checkStatus == 1){(hashTable->table[bucket_index]).bucket_range++;}
    }

    //after inserting, check useFactor...
    struct HashTableInfoTag* currInfo = malloc(sizeof(struct HashTableInfoTag)); 
    GetHashTableInfo(hashTable, currInfo);

    //call checkResize(); function to check if resizing is needed 
    //(the '1' is a 'true' bool variable to indicate we are inserting)
    if(checkResize(hashTable, currInfo, 1)) {ResizeHashTable(&hashTable, currInfo);}

    //after checking resize (and possibly resizing) free info tag and NULL teh pointer
    free(currInfo);
    currInfo = NULL;

    //return status of insert
    return checkStatus;
}

static int SearchAndDestroy(struct BucketItem** root, char* key, void **dataHandle)
{
    //conditional statement to find match, if no match return -2, not found
    if (*root != NULL)
    {
        if(strcmp(key, (*root)->key) < 0)
        {
            //recursive left
            return SearchAndDestroy(&((*root)->left), key, dataHandle);
        }
        else if(strcmp(key, (*root)->key) > 0)        
        {
            //recursive right
            return SearchAndDestroy(&((*root)->right), key, dataHandle);
        }
        else
        {
            //theres a match!, temp pointer to root
            struct BucketItem* delPTR = *root;

            //no children case
            if((*root)->left == NULL && (*root)->right == NULL)
            {
                *dataHandle = delPTR->data;
                free(delPTR->key);
                free(delPTR);
                *root = NULL;
            }
            //one child right case
            else if((*root)->left == NULL && (*root)->right != NULL)
            {
                *dataHandle = delPTR->data;
                free(delPTR->key);
                *root = (*root)->right;
                free(delPTR);
            }
            //one child left case
            else if((*root)->left != NULL && (*root)->right == NULL)
            {
                *dataHandle = delPTR->data;
                free(delPTR->key);
                *root = (*root)->left;
                free(delPTR);
            }
            //two child case
            else
            {
                //replace root (match) with smallest is RIGHT subtree (use smallestRight)
                char* smallestRight = findSmallest((*root)->right);
                SearchAndDestroy(&((*root)->right), smallestRight, dataHandle);
                (*root)->data = smallestRight;
            }

            return 0;
        }
    }
    else{return -2;}

}

int DeleteEntry( HashTablePTR hashTable, char *key, void **dataHandle )
{
    //checks if hashTable is a valid hashTableObject
    if (hashTable == NULL || *((int*)hashTable) != (int)0xDEADBEEF) {return -1;}

    //gets index of bucket from hash
    unsigned int bucket_index = Hashish(key, hashTable->bucketCount);
    struct BucketItem** rootPTR = &((hashTable->table[bucket_index]).bucket_root); 

    //actually delete node, if successful (check == 0), decrease bucket size
    int check = SearchAndDestroy(rootPTR, key, dataHandle);
    if (!check) {(hashTable->table[bucket_index]).bucket_range--;}

    //use factor only depends on buckets with head node or not, therefore only neccessary to
    //check if a new root is being created (ie not below this if statement!)
    struct HashTableInfoTag* currInfo = malloc(sizeof(struct HashTableInfoTag)); 
    GetHashTableInfo(hashTable, currInfo);

    if(checkResize(hashTable, currInfo, 0)) {ResizeHashTable(&hashTable, currInfo);}

    free(currInfo);
    currInfo = NULL;

    return check;
}

static struct BucketItem* searchBucket(struct BucketItem *root, char* key)
{
    //simple recursive search
    if(root != NULL)
    {
        if(strcmp(key, root->key) < 0)
        {
            return searchBucket(root->left, key);
        }
        else if(strcmp(key, root->key) > 0)
        {
            return searchBucket(root->right, key);
        }
        else
        {
            return root;
        }
    }
    else{return NULL;}
}

int FindEntry( HashTablePTR hashTable, char *key, void **dataHandle )
{
    //checks validity of hashTable
    if (hashTable == NULL || *((int*)hashTable) != (int)0xDEADBEEF) {return -1;}

    //gets index of bucket from hash
    unsigned int bucket_index = Hashish(key, hashTable->bucketCount);

    //don't bother searching if root is NULL, time performance!
    if((hashTable->table[bucket_index]).bucket_root == NULL){return -2;}

    //sends this to search function to be search for
    struct BucketItem* found = searchBucket((hashTable->table[bucket_index]).bucket_root, key);

    //if found is NULL, nothing was found, if not null, set data handle to nodes data and return sucess
    if(found == NULL){return -2;}
    else{*dataHandle = found->data;}

    return 0;
}

static void getBucketKeys(struct BucketItem** rootPTR, char*** keysArrayHandle, int* index)
{
    if(*rootPTR != NULL)
    {
        //allocate and copy new string into array handle, increase index
        ((*keysArrayHandle)[*index]) = malloc(strlen((*rootPTR)->key) + 1);
        strcpy(((*keysArrayHandle)[*index]), (*rootPTR)->key);
        (*index)++;

        //recursive portion, similar to print treenode
        getBucketKeys(&((*rootPTR)->left), keysArrayHandle, index);
        getBucketKeys(&((*rootPTR)->right), keysArrayHandle, index);
    }
}

int GetKeys( HashTablePTR hashTable, char ***keysArrayHandle, unsigned int *keyCount )
{
    //checks validity of hashTable
    if (hashTable == NULL || *((int*)hashTable) != (int)0xDEADBEEF) {return -1;}

    //outer index for keysArrayHandle
    int index = 0;

    //temp variable for keyCount
    unsigned int count = 0;
    for(int bucket = 0; bucket < hashTable->bucketCount; bucket++)
    {
        count += (hashTable->table[bucket]).bucket_range;
    }
    *keyCount = count;

    //make space for arrayHandle (user will free)
    *keysArrayHandle = malloc(sizeof(char*) * count);

    //goes through each bucket and gets keys in entire bucket from getBucketKeysFunction
    for(int bucket = 0; bucket < hashTable->bucketCount; bucket++)
    {
        struct BucketItem** root = &((hashTable->table[bucket]).bucket_root);
        getBucketKeys(root, keysArrayHandle, &index);
    }

    return 0;
}
