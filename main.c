#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>
#include "utils.c"


/*
 * Print every entry available from an already-open directory stream.
 *
 * This consumes the stream with readdir(), so callers should not expect the
 * stream position to stay unchanged after this function returns.
 */
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

/*
 * Read the direct children of `directory` and append every child directory to
 * the linked list pointed to by `head`.
 *
 * The function ignores "." and "..", builds a full path for each entry, and
 * uses stat() to keep only directories. Each inserted path is duplicated by
 * createNode(), so the temporary concatenated path can be freed immediately.
 */
void storeDir(Node **head,char *directory){
    /* compute the directory path and store directory path into the SSL */
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

/*
 * Traverse directories starting at `first_path_dir` and collect repositories.
 *
 * `directories` acts as a work list: storeDir() appends newly discovered
 * subdirectories to the same list while `current` walks through it. When a path
 * ends with `dir_to_search` (currently ".git"), the parent directory is added
 * to `git_repo`.
 */
void search_repos(Node **directories, Node **git_repo, char **first_path_dir,char **dir_to_search, int *dir_to_search_size){
    storeDir(directories, *first_path_dir);
    Node *current = *directories;
    while(current != NULL){
        int dir_size = strlen(current->directory);
        if(dir_size != *dir_to_search_size && (0 != strcmp(&(current->directory[dir_size - *dir_to_search_size]),*dir_to_search))){
            storeDir(&current,current->directory);
        }
        else{
            char* tmp_string = malloc(sizeof(char)*strlen(current->directory) +1);
            sprintf(tmp_string,"%.*s", dir_size-*dir_to_search_size, current->directory);
            if(*git_repo == NULL){
                *git_repo = createNode(tmp_string);
            }
            else{
                insertAtEnd(*git_repo,tmp_string);
            }
            free(tmp_string);
        }
        current =  current->next;
    }
}

/*
 * Program entry point.
 *
 * Expected usage:
 *   ./main /path/to/search
 *
 * The discovered repository roots are printed to stdout, then both linked
 * lists are released before exiting.
 */
int main (int argc, char** argv){
    if(argc != 2){
        printf("need an argument directory, currently given %d\n",argc);
        return -1;
    }
    char *first_path_dir = argv[1];
    Node *HEAD = NULL;
    Node *git_repo = NULL;
    char *dirToSearch = ".git";
    int dirToSearchSize = strlen(dirToSearch);
    search_repos(&HEAD, &git_repo, &first_path_dir, &dirToSearch,&dirToSearchSize);
    
    printSLL(git_repo); 
    freeSLL(HEAD);
    freeSLL(git_repo);
    return 0;
}
