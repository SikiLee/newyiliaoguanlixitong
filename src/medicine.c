#include "medicine.h"
#include "department.h"
#include "data.h"

void makeNextMedicineId(char *out, int outSize)
{
    Medicine *p = medicineHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "MED");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "MED", max + 1);
}

Medicine *findMedicineById(const char *id)
{
    Medicine *p = medicineHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

static int medicineNameMatch(Medicine *p, const char *name)
{
    return strcmp(p->name, name) == 0 || strcmp(p->commonName, name) == 0 ||
        strcmp(p->tradeName, name) == 0 || strcmp(p->alias, name) == 0;
}

int collectMedicinesByName(const char *name, Medicine *list[], int maxCount)
{
    Medicine *p = medicineHead;
    int count = 0;
    while (p != NULL && count < maxCount) {
        if (medicineNameMatch(p, name)) {
            list[count++] = p;
        }
        p = p->next;
    }
    return count;
}

int addMedicine(const char *id, const char *name, const char *commonName,
    const char *tradeName, const char *alias, const char *deptId,
    double price, int stock, int lowLimit)
{
    Medicine *node;
    Medicine *p;
    if (isBlank(id) || !checkName(name) || !checkName(commonName) ||
        !checkName(tradeName) || isBlank(alias) || hasBadChar(alias) ||
        findDepartmentById(deptId) == NULL || !checkMoney(price) ||
        stock < 0 || lowLimit < 0) {
        return ERR_INPUT;
    }
    if (findMedicineById(id) != NULL) return ERR_REPEAT;
    node = (Medicine *)malloc(sizeof(Medicine));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->name, NAME_LEN, name);
    safeCopy(node->commonName, NAME_LEN, commonName);
    safeCopy(node->tradeName, NAME_LEN, tradeName);
    safeCopy(node->alias, NAME_LEN, alias);
    safeCopy(node->deptId, ID_LEN, deptId);
    node->price = price;
    node->stock = stock;
    node->lowLimit = lowLimit;
    node->next = NULL;
    if (medicineHead == NULL) {
        medicineHead = node;
    } else {
        p = medicineHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int updateMedicine(const char *id, double price, int stock, int lowLimit)
{
    Medicine *p = findMedicineById(id);
    if (p == NULL) return ERR_NOT_FOUND;
    if (!checkMoney(price) || stock < 0 || lowLimit < 0) return ERR_INPUT;
    p->price = price;
    p->stock = stock;
    p->lowLimit = lowLimit;
    return OK;
}

static int medicineHasLink(const char *id)
{
    PrescriptionItem *item = prescriptionItemHead;
    while (item != NULL) {
        if (strcmp(item->medicineId, id) == 0) return 1;
        item = item->next;
    }
    return 0;
}

int deleteMedicine(const char *id)
{
    Medicine *p = medicineHead;
    Medicine *pre = NULL;
    if (medicineHasLink(id)) return ERR_LINKED;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (pre == NULL) medicineHead = p->next;
            else pre->next = p->next;
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

int reduceMedicineStock(const char *id, int count)
{
    Medicine *p = findMedicineById(id);
    if (p == NULL) return ERR_NOT_FOUND;
    if (!checkCount(count)) return ERR_INPUT;
    if (p->stock < count) {
        // 答辩注意：发药前先检查库存，不能先扣再判断（难度：中等）
        return ERR_NO_SPACE;
    }
    p->stock -= count;
    return OK;
}

void listMedicines(char *out, int outSize)
{
    Medicine *p = medicineHead;
    Department *dept;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "药品编号 | 药品名 | 通用名 | 商品名 | 别名 | 科室 | 单价 | 库存 | 下限");
    while (p != NULL) {
        dept = findDepartmentById(p->deptId);
        snprintf(line, sizeof(line), "%s | %s | %s | %s | %s | %s | %.2f | %d | %d\n",
            p->id, p->name, p->commonName, p->tradeName, p->alias,
            dept ? dept->name : "未知科室", p->price, p->stock, p->lowLimit);
        appendText(out, outSize, line);
        p = p->next;
    }
}
