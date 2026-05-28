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

void printSLL(Node* head) {
    Node* temp = head;
    while (temp != NULL) {
        printf("%s\n", temp->directory);
        temp = temp->next;
    }
}


void freeSLL(Node *head){
    Node* temp = head;
    while (head != NULL) {
        free(head->directory);
        temp = head;
        head = head->next;
        free(temp);
    }
}