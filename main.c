#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ncurses.h>

#define SIZE_NAME 100
#define PATH_SIZE 1024
#define HASH_size 10
#define COMMIT_SIZE 2048

int Cnt_node = 0;

typedef struct GitRepo {
    char name[SIZE_NAME];
    char path[PATH_SIZE];
    char hash[HASH_size];
    char last_commit[COMMIT_SIZE];
    bool has_upstream;
    bool is_behind;
    bool is_ahead;

} GitRepo;

typedef struct SllRepo{
    GitRepo *node;
    struct SllRepo *previous;
    struct SllRepo *next;

} SllRepo;

void freeSll(SllRepo **node){
    if((*node) == NULL) {
        printf("Head is already null\n");
    }
    SllRepo *tmp_node = NULL;
    while((*node) != NULL){
        SllRepo *tmp_node = (*node)->next;
        free((*node)->node);
        free((*node));
        (*node) = tmp_node;
    }
}

void trim_new_line(char *s){
    int n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

void set_last_commit(SllRepo **node, char *path){
    char buffer[COMMIT_SIZE] = {'\0'};
    char command[COMMIT_SIZE] = {'\0'};
    snprintf(command, COMMIT_SIZE, "cd %s 2>/dev/null && git log -1 --pretty=format:\"%%h %%s (%%cr)\"", path);
    FILE *fp = popen(command,"r");
    if(fp == NULL){
        printf("Can not get commits from Git repository %s\n", path);
        perror("popen");
        return;
    }
    else{
        if (fgets(buffer, sizeof(buffer),fp) == NULL) {printf("Can bot get commits from Git repository during copy %s\n", path);}
        trim_new_line(buffer);
        snprintf((*node)->node->last_commit,COMMIT_SIZE,"%s",buffer);
    }
    pclose(fp);
}

void set_info_repo(SllRepo **node, char *path){
    char buffer[256];
    char command[2048] = {'\0'};
    snprintf(command, COMMIT_SIZE, "cd %s 2>/dev/null && git status --porcelain -b v1", path);
    FILE *fp = popen(command,"r");
    if(fp == NULL){
        printf("Can bot get info from Git repository %s\n", path);
        perror("popen");
        return;
    }
    else{
        /*
        char count_loop = 0;
        while((fgets(buffer, sizeof(buffer),fp) != NULL)){
            count_loop++;
        }
        if(count_loop != 1){
            printf("something goes wrong while processing info about the git repo %s, info is superioir to buffer\n", path);
        }
        */
        if (fgets(buffer, sizeof(buffer),fp) == NULL) {printf("Can bot get info from Git repository during copy %s\n", path);}
        (*node)->node->has_upstream = (strstr(buffer, "origin") != NULL);
        (*node)->node->is_ahead = (strstr(buffer, "ahead") != NULL);
        (*node)->node->is_behind = (strstr(buffer, "behind") != NULL);
    }
    pclose(fp);
    return;
}

void set_name(SllRepo ** node,char * dir_path){
    int pos_to_trim = strlen(dir_path);
    char *name =(*node)->node->name;
    while (dir_path[pos_to_trim] != '/' && pos_to_trim != 0){
        pos_to_trim--;
    }
    pos_to_trim++; // to avoid keeping the '/' character
    if( pos_to_trim  >= SIZE_NAME){
        fprintf(stderr, "Invalid directory name\n");
        exit(1);
    }
    sprintf(name, "%s",dir_path + pos_to_trim);
}

void set_path(SllRepo ** node,char * dir_path){
    if(strlen(dir_path) >= PATH_SIZE){
        fprintf(stderr, "Path of git repo is longer than expected\n");
        exit(1);
    }
    sprintf((*node)->node->path,"%s" ,dir_path);
}

void populate_new_node(SllRepo **node, char *dir_path){
    if(node == NULL || *node == NULL){
        fprintf(stderr,"Node is null in populate new_node\n");
    }
    if(dir_path == NULL || (0 == strcmp(dir_path, "\0"))){
        fprintf(stderr, "dir_path is null or empty string in populate new_node\n");
    }
    (*node)->node = malloc(sizeof(GitRepo));
    if ((*node)->node == NULL)
    {
        perror("Can not create node"); // perror have already a newline at the end
        exit(1);
    }
    set_path(node, dir_path);
    set_name(node,dir_path);
    set_info_repo(node, dir_path);
    set_last_commit(node, dir_path);
    Cnt_node += 1;
}

int is_git_repo(const char *path_directory, struct dirent *directory){
    struct stat stats;
    if( 0 == stat(path_directory,&stats) && S_ISDIR(stats.st_mode)){ // the d_type in dirent struct is not supported evrywhere
        if(directory != NULL){
            if(strcmp(directory->d_name,".git") == 0){
                return true;
            }
        }
    }
    return false;
}

void search_git_repo_recursive(char *arg, SllRepo **head, SllRepo **tail){
    char *tmp_arg_path = arg;
    DIR *p_directory = opendir(tmp_arg_path);
    struct dirent *p_tmp_dir = NULL;
    bool is_repo_has_git_repo = false;
    if(p_directory == NULL){
        perror("Can't read directory opendir");
        exit(1);
    }
    /* First pass: search for .git
       readdir doesn't give the repos hidden repos first
    */
    struct stat stats;
    while ((p_tmp_dir = readdir(p_directory)) != NULL) {
        char *name = p_tmp_dir->d_name;
        char *tmp_path = malloc(sizeof(char) * (strlen(tmp_arg_path) + 1 + strlen(name) + 1)); // allocate enough space for the all path arg/tmp_dir\0
        if(tmp_path == NULL){perror("can not allocate memory for temp path\n"); exit(1);}
        sprintf(tmp_path, "%s/%s", tmp_arg_path, name);
        if ((strcmp(name, ".git") == 0) && (0 == stat(tmp_path,&stats)) && S_ISDIR(stats.st_mode)) {
            // Found a Git repo
            is_repo_has_git_repo = true;
            free(tmp_path);
            break;
        }
        free(tmp_path);
    }
    if(! is_repo_has_git_repo){
        rewinddir(p_directory); /* resets the position of the directory stream dirp to the beginning of the directory. */
        while ((p_tmp_dir = readdir(p_directory)) != NULL){
            if ((strcmp(p_tmp_dir->d_name,"..") != 0) && ( 0 != strcmp(".",p_tmp_dir->d_name))){
                char *tmp_path = malloc(sizeof(char) * (strlen(tmp_arg_path) + 1 + strlen(p_tmp_dir->d_name) + 1)); // allocate enough space for the all path arg/tmp_dir\0
                if(tmp_path == NULL){perror("can not allocate memory for temp path\n"); exit(1);}
                sprintf(tmp_path, "%s/%s", tmp_arg_path, p_tmp_dir->d_name);
                if((0 == stat(tmp_path,&stats)) && S_ISDIR(stats.st_mode)){
                    search_git_repo_recursive(tmp_path, head, tail);
                }
                free(tmp_path);
            }
        }
    }
    else{
        if(*head == NULL){
            *head = malloc(sizeof(SllRepo));
            if(*head == NULL){
                perror("can not allocate memory for head");
                exit(1);
            }
            (*head)->next = NULL;
            (*head)->previous = NULL;
            populate_new_node(head,tmp_arg_path);
            //populate new node function TO_DO
            *tail = *head;
        }
        else{
            if(*tail == NULL){
                perror("can not allocate memory for tail");
                exit(1);
            }
            while((*tail)->next != NULL){
                (*tail)->next->previous = *tail;
                *tail = (*tail)->next;

            }
            SllRepo *new_node = malloc(sizeof(SllRepo));
            if(new_node == NULL){
                perror("Can not allocate memory for a new node");
                exit(1);
            }
            new_node->next = NULL;
            new_node->previous = NULL;
            //populate new node function TO_DO
            (*tail)->next = new_node;
            (*tail)->next->previous = *tail;
            *tail = (*tail)->next;
            populate_new_node(tail,tmp_arg_path);
            
        }
    }
    closedir(p_directory);
    return;
}

void print(SllRepo *node){
    while(node !=  NULL){
        printf("%s\n",node->node->name);
        printf("%s\n",node->node->path);
        printf("has upstream: %d\n",node->node->has_upstream);
        printf("is ahead: %d\n",node->node->is_ahead);
        printf("is behind: %d\n",node->node->is_behind);
        printf("%s\n",node->node->last_commit);
        node = node->next;
    }
}

void trim_slash_arg_cli(char **arg){
    if(arg == NULL){ perror("arg should not be NULL"); exit(1);}
    int len = strlen(*arg);
    if( (*arg)[len -1] == '/'){
        (*arg)[len -1] = '\0';
    }
}

/*
##############################################################################################
#                                                                                            #
#                                         NCURSE                                             #
#                                                                                            #
##############################################################################################
*/

int main(int argc, char *argv[]){
    SllRepo *HEAD = NULL;
    SllRepo *TAIL = NULL;
    if(argc == 1){
        search_git_repo_recursive(".",&HEAD,&TAIL);
    }
    else{
        for(int i=1; i < argc; i++){
            trim_slash_arg_cli(&(argv[i]));
            search_git_repo_recursive(argv[i],&HEAD,&TAIL);
        }
    }
    initscr();
    cbreak();
    noecho();
    int x = 800, y = 1200;
    getmaxyx(stdscr,y,x);
    keypad(stdscr,true);
    int c = 0;
    int highlight = 0;
    while(c != (int) 'q'){
        refresh();
        c = wgetch(stdscr);
        switch(c){
            case KEY_DOWN:
                highlight = highlight + 1;
                highlight = highlight % Cnt_node;
                break;
            case KEY_UP:
                highlight = highlight - 1;
                if (highlight < 0 ) highlight = highlight + Cnt_node; // fix moving upward because of negative modulus
                highlight = highlight % Cnt_node;
                break;
            default:
        }
    }
    clear();
    endwin();
    //print(HEAD);
    freeSll(&HEAD);
    return 0;
}
