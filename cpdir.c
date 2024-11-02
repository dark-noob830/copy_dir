#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


void *copy_directory(void *args);
void copy_file(void *paths);

typedef struct {
    char source[2048];
    char destination[2048];
} PathFiles;

void *copy_dir(void *args){
    PathFiles *paths = (PathFiles *)args;
    DIR *dir = opendir(paths->source);

    if (dir == NULL) {
        perror("Error opening directory");
        return NULL;
    }

    mkdir(paths->destination, 0755);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char source_path[2048];
        char destination_path[2048];
        
        if (snprintf(source_path, sizeof(source_path), "%s/%s", paths->source, entry->d_name) >= sizeof(source_path) ||
            snprintf(destination_path, sizeof(destination_path), "%s/%s", paths->destination, entry->d_name) >= sizeof(destination_path)) {
            fprintf(stderr, "Path too long, skipping: %s\n", entry->d_name);
            continue;
        }
        
        PathFiles *new_paths = malloc(sizeof(PathFiles));
        strcpy(new_paths->source, source_path);
        strcpy(new_paths->destination, destination_path);
        
        
        if (entry->d_type == DT_DIR) {
            pthread_t thread;
            pthread_create(&thread, NULL, copy_dir, new_paths);
            pthread_detach(thread);
        }
         else {
            copy_file(new_paths);
        }
    }
    closedir(dir);
    free(paths);
    return NULL;
}


void copy_file(void *paths){
    PathFiles *path = (PathFiles *)paths;
    int src_file = open(path->source, O_RDONLY);
    if (src_file == -1) {
        perror("Error opening source file");
        return;
    }

    int dest_file = open(path->destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_file == -1) {
        perror("Error creating destination file");
        close(src_file);
        return;
    }

    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(src_file, buffer, sizeof(buffer))) > 0) {
        write(dest_file, buffer, bytes);
    }

    close(src_file);
    close(dest_file);
    

}


int main(int argc, char *argv[]){
    if (argc!=3){
        printf("Usage: %s <source_directory> <destination_directory>\n", argv[0]);
        return 1;
    }

    PathFiles *paths = malloc(sizeof(PathFiles));
    strcpy(paths->source, argv[1]);
    strcpy(paths->destination, argv[2]);
    copy_dir(paths);
    sleep(1);
    return 0;
}
