#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
    char *directory;
    struct node *next;
}Node;


typedef struct node_text {
    char **directories;
    struct node_text *next;
}Node_text;


/*
 * Allocate a new linked-list node and copy `s` into owned memory.
 *
 * The returned node must eventually be released with freeSLL().
 */
Node *createNode(char *s){
    Node* first = malloc(sizeof(Node));
    if (first == NULL){
        perror("Failed to create a node");
        exit(1);
    }
    first->directory = strdup(s);
    first->next = NULL;
    return first;
}

/*
 * Append a new node containing `s` to the end of an existing linked list.
 *
 * `ssl` must point to the first node of a non-empty list. The string is copied
 * by createNode(), so callers keep ownership of the original `s`.
 */
void insertAtEnd(Node *ssl,char *s){
    Node *HEAD = ssl;
    if(ssl == NULL){
        perror("SLL is null");
        exit(1);
    }
    while(HEAD->next != NULL){
        HEAD = HEAD->next;
    }
    HEAD->next = createNode(s);
}

/*
 * Print each stored directory path, one per line.
 */
void printSLL(Node* head) {
    Node* temp = head;
    while (temp != NULL) {
        printf("%s\n", temp->directory);
        temp = temp->next;
    }
}


/*
 * Free every node in the linked list, including each node's copied directory
 * string.
 */
void freeSLL(Node *head){
    Node* temp = head;
    while (head != NULL) {
        free(head->directory);
        temp = head;
        head = head->next;
        free(temp);
    }
}
