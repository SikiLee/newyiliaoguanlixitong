#ifndef DEPARTMENT_H
#define DEPARTMENT_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextDepartmentId(char *out, int outSize);
Department *findDepartmentById(const char *id);
Department *findDepartmentByName(const char *name);
int addDepartment(const char *id, const char *name, const char *intro, const char *wardType);
int updateDepartment(const char *id, const char *intro, const char *wardType);
int deleteDepartment(const char *id);
void listDepartments(char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
