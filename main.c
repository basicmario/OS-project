
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_FILES 1000
#define MAX_PATH_LENGTH 4096

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: reader.exe <folder_path>\n");
        return 1;
    }

    char searchPath[MAX_PATH_LENGTH];

    snprintf(searchPath, sizeof(searchPath),
             "%s\\*", argv[1]);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Could not open folder.\n");
        return 1;
    }

    char *files[MAX_FILES];
    int count = 0;

    // Collect filenames
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        if (count >= MAX_FILES)
            break;

        files[count] = malloc(strlen(findData.cFileName) + 1);

        if (files[count] == NULL)
            break;

        strcpy(files[count], findData.cFileName);
        count++;

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // Sort filenames alphabetically
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {

            if (strcmp(files[i], files[j]) > 0) {
                char *temp = files[i];
                files[i] = files[j];
                files[j] = temp;
            }
        }
    }

    // Read files sequentially
    for (int i = 0; i < count; i++) {

        char filePath[MAX_PATH_LENGTH];

        snprintf(filePath, sizeof(filePath),
                 "%s\\%s", argv[1], files[i]);

        FILE *file = fopen(filePath, "rb");

        if (file == NULL) {
            perror("File open failed");
            free(files[i]);
            continue;
        }

        printf("\nReading: %s\n", files[i]);

        char buffer[1024];
        size_t bytes;

        while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, bytes, stdout);
        }

        fclose(file);
        free(files[i]);
    }

    return 0;
}
