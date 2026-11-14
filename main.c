#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <locale.h>

#define SIZE_NAME 100
#define PATH_SIZE 1024
#define HASH_size 10
#define COMMIT_SIZE 2048

#define NAME_PERCENT   16
#define PATH_PERCENT   40
#define COMMIT_PERCENT 40
#define STATUS_PERCENT 4

#define COL_GAP 2

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
        (*node)->node->has_upstream = false;
        (*node)->node->is_ahead = false;
        (*node)->node->is_behind = false;
        (*node)->node->has_upstream = (strstr(buffer, "origin") != NULL);
        (*node)->node->is_ahead = (strstr(buffer, "ahead") != NULL);
        (*node)->node->is_behind = (strstr(buffer, "behind") != NULL);
    }
    pclose(fp);
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

typedef struct {
    int ind_selected_for_higlight;
    int ind_start_win;
    int win_x;
    int win_y;

    // Column widths
    int name_w;
    int path_w;
    int commit_w;
    int status_w;

    // Column positions
    int name_x;
    int path_x;
    int commit_x;
    int status_x;

} UiState;

static void init_colors_safe(void) {
    if (!has_colors()) return;
    start_color();
    use_default_colors();
    init_pair(1, COLOR_GREEN,  -1); // good / up to date
    init_pair(2, COLOR_YELLOW, -1); // ahead/behind/no upstream
    init_pair(3, COLOR_RED,    -1); // dirty / errors
    init_pair(4, COLOR_CYAN,   -1); // headers
}

void update_columns(UiState *state)
{
    int old_win_x = state->win_x;
    int old_win_y = state->win_y;
    getmaxyx(stdscr,state->win_y,state->win_x);
    if(old_win_x != state->win_x || old_win_y != state->win_y) clear();
    int width = state->win_x;

    // Column positions
    state->name_x   = 0;
    state->path_x   = width * NAME_PERCENT / 100;
    state->commit_x = width * (NAME_PERCENT + PATH_PERCENT) / 100;
    state->status_x = width * (NAME_PERCENT + PATH_PERCENT + COMMIT_PERCENT) / 100;

    // Column widths (including the gap)
    state->name_w   = state->path_x - state->name_x;
    state->path_w   = state->commit_x - state->path_x;
    state->commit_w = state->status_x - state->commit_x;
    state->status_w = width - state->status_x;
}

void init_curse(UiState *State){
    initscr();
    cbreak();
    noecho();
    init_colors_safe();
    update_columns(State);
    keypad(stdscr,true);
}

void draw_header(UiState *state){
    attron(COLOR_PAIR(4) | A_BOLD);
    mvhline(0, 0, ' ', state->win_x);
    mvprintw(0, 1, "Git Repositories");
    attroff(COLOR_PAIR(4) | A_BOLD);
    attron(A_BOLD);
    mvprintw(2, state->name_x,   "NAME");
    mvprintw(2, state->path_x,   "PATH");
    mvprintw(2, state->commit_x, "LAST COMMIT");
    mvprintw(2, state->status_x, "STATUS");
    attroff(A_BOLD);
    mvhline(3, 0, ACS_HLINE, state->win_x);
}

void draw_body(SllRepo *p_node,UiState *p_state){
    SllRepo *p_tmp = p_node;
    for(int i=p_state->ind_start_win;i<Cnt_node; i++){
        int y = i + 4;
        if (i == p_state->ind_selected_for_higlight) attron(A_REVERSE);
        /*
        The format %-28.28s means:
        - - → Left-align the text.
        - 28 → Use a minimum width of 28 characters.
        - .28 → Print at most 28 bytes from the string.
        */
        mvprintw(y, p_state->name_x,
                 "%-*.*s",
                 p_state->name_w - COL_GAP,
                 p_state->name_w - COL_GAP,
                 p_tmp->node->name);

        mvprintw(y, p_state->path_x,
                 "%-*.*s",
                 p_state->path_w - COL_GAP,
                 p_state->path_w - COL_GAP,
                 p_tmp->node->path);

        mvprintw(y, p_state->commit_x,
                 "%-*.*s",
                 p_state->commit_w - COL_GAP,
                 p_state->commit_w - COL_GAP,
                 p_tmp->node->last_commit);

        if (i == p_state->ind_selected_for_higlight)
            attroff(A_REVERSE);

        p_tmp = p_tmp->next;
    }

    mvhline(p_state->win_y - 1, 0, ' ', p_state->win_x);
    attron(A_DIM);
    mvprintw(p_state->win_y - 1, 1, "↑/↓ move  PgUp/PgDn scroll   q quit   total: %u", Cnt_node);// unsigned integer
    attroff(A_DIM);
}

void drawUI(SllRepo *p_node, UiState *p_state){
    draw_header(p_state);
    draw_body(p_node, p_state);
}

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
    setlocale(LC_ALL, "");
    UiState State;
    State.ind_start_win = 0;
    State.win_x = 800; State.win_y = 1200;
    init_curse(&State);
    int c = 0;
    int highlight = 0;
    while(c != (int) 'q'){
        drawUI(HEAD, &State);
        c = wgetch(stdscr);
        switch(c){
            case KEY_DOWN:
                State.ind_selected_for_higlight++;
                break;
            case KEY_UP:
                State.ind_selected_for_higlight--;
                if(State.ind_selected_for_higlight)
                break;
            default:
        }
        if(State.ind_selected_for_higlight < 0) State.ind_selected_for_higlight = Cnt_node - 1;
        State.ind_selected_for_higlight = abs(State.ind_selected_for_higlight % Cnt_node);
        if(State.ind_selected_for_higlight > State.win_y) State.ind_start_win++;
        if(State.ind_selected_for_higlight < 0) State.ind_start_win--;
        update_columns(&State);
        refresh();
    }
    clear();
    endwin();
    //print(HEAD);
    freeSll(&HEAD);
    return 0;
}
