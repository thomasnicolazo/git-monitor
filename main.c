#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.c"


void printDir(DIR *stream){
    if (stream ==  NULL){
        perror("there is no stream");
    }
    DIR *tmpStream = stream;
    struct dirent* tmp; 
    while ((tmp = readdir(tmpStream))){
        printf("%s\n", tmp->d_name);
    }
}

void storeDir(Node **head,char *directory){
    DIR *streamd = opendir(directory);
    struct stat stats;
    if (streamd == NULL){
        perror("can not open stream ");
        exit(1);
    } 
    struct dirent* tmp; 
    while ((tmp = readdir(streamd))){
        if(( 0 != strcmp("..",tmp->d_name)) && ( 0 != strcmp(".",tmp->d_name))){
            char *concatString = malloc(sizeof(char)*(strlen(tmp->d_name)+strlen(directory)+2));
            sprintf(concatString,"%s/%s",directory,tmp->d_name); 
            if( 0 == stat(concatString,&stats) && S_ISDIR(stats.st_mode)){
                if(*head == NULL){
                    Node* first = createNode(concatString);
                    *head = first;
                }
                else{
                    insertAtEnd(*head, concatString);
                }
            }
            free(concatString);
        }
    }
    closedir(streamd);
}


int main (int argc, char** argv){
    Node *HEAD = NULL;
    char *firstd = "/home/thomas/Documents/project";
    char *dirToSearch = ".git";
    int dirToSearchSize = strlen(dirToSearch);
    storeDir(&HEAD, firstd);
    Node *current = HEAD;
    printSLL(HEAD); 
    do{
        int dir_size = strlen(current->directory);
        if(dir_size != dirToSearchSize && (0 != strcmp(&(current->directory[dir_size - dirToSearchSize]),dirToSearch))){
            storeDir(&current,current->directory);
        }
        //storeDir(&current,current->directory);
        current =  current->next;
    }while(current != NULL);
    printSLL(HEAD); 
    freeSLL(HEAD);
    return 0;
}
