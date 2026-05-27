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

static const char *BLUE   = "\x1b[1;34m";
static const char *GREEN  = "\x1b[0;32m";
static const char *RED    = "\x1b[0;31m";
static const char *CYAN   = "\x1b[0;36m";
static const char *NORMAL = "\x1b[0m";

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


/*
int main (int argc, char** argv){
    const Node *HEAD = NULL;
    Node *n1 = malloc(sizeof(Node));
    Node *n2 = malloc(sizeof(Node));
    Node *n3 = malloc(sizeof(Node));
    n1->directory = "hello ";
    n2->directory = "hello ";
    n3->directory = "hello ";
    n1->next = n2;
    n2->next = n3;
    n3->next = NULL;
    HEAD = n1;
    int i = 0;
    while(HEAD != NULL && i < 3){
        printf("%s",HEAD->directory);
        i++;
        HEAD = HEAD->next;
    }
    free(n1);
    free(n2);
    free(n3);
    return 0;
}
*/
