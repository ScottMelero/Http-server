/* Queue data structure implementation. */
#include "myqueue.h"
#include <stdlib.h>

node_t* head = NULL;
node_t* tail = NULL;

// queues up a request 
void enqueue(int *client_sockd) {
    node_t *newnode = (node_t*) malloc(sizeof(node_t));
    newnode->client_sockd = client_sockd;
    newnode->next = NULL;
    if(tail == NULL){
        head = newnode;
    } else {
        tail->next = newnode;
    }
    tail = newnode; 
}

// returns NULL if the queue isempty
// removes the head from the queue 
int* dequeue(){
    if(head == NULL){
        return NULL;
    } else{
        int *result = head->client_sockd;
        node_t *temp = 
        head;
        head = head->next;
        if(head == NULL) {tail = NULL;}
        free(temp);
        return result;
    }
}