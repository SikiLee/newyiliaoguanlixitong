#ifndef FILE_H
#define FILE_H

#ifdef __cplusplus
extern "C" {
#endif

int loadAllData(const char *dir, char *msg, int msgSize);
int saveAllData(const char *dir, char *msg, int msgSize);
int checkDataFile(const char *path, int fieldCount, char *msg, int msgSize);

#ifdef __cplusplus
}
#endif

#endif
