#include <windows.h>
#include <graphics.h>
#include <string>
#include <vector>
#include <ctime>

extern "C" {
#include "common.h"
#include "data.h"
#include "patient.h"
#include "department.h"
#include "doctor.h"
#include "medicine.h"
#include "ward.h"
#include "record.h"
#include "payment.h"
#include "prescription.h"
#include "report.h"
#include "file.h"
#include "test.h"
#include "gui.h"
}

#define WIN_W 1060
#define WIN_H 620
#define PAGE_HOME 0
#define PAGE_PATIENT 1
#define PAGE_MEDICAL 2
#define PAGE_ADMIN 3
#define PAGE_RESULT 4
#define PAGE_PATIENT_AUTH 5
#define PAGE_DOCTOR_AUTH 6

typedef struct Button {
    int x1, y1, x2, y2;
    const wchar_t *text;
    int action;
} Button;

typedef struct CheckItem {
    const char *id;
    const char *name;
    const char *deptId;
    double price;
} CheckItem;

typedef struct DashboardStats {
    double totalIncome;
    int todayRegisterCount;
    int unpaidCount;
    int occupiedBeds;
    int totalBeds;
    double bedOccupancyRate;
    int lowStockMedicineCount;
} DashboardStats;

typedef struct WarningStats {
    int lowStockMedicineCount;
    int unpaidPaymentCount;
    int bedShortageWardCount;
    int lowDoctorSlotCount;
} WarningStats;

// 答辩注意：按科室预设检查项目列表，数据来自项目约定（难度：简单）
static CheckItem internalCheckItems[] = {
    // DEP0001 内科
    {"CHK001", "血常规检查",     "DEP0001", 25.00},
    {"CHK002", "尿常规检查",     "DEP0001", 20.00},
    {"CHK003", "肝功能检查",     "DEP0001", 60.00},
    {"CHK004", "心电图检查",     "DEP0001", 35.00},
    {"CHK005", "胸部X光检查",    "DEP0001", 50.00},
    // DEP0002 外科
    {"CHK006", "伤口清创处理",   "DEP0002", 30.00},
    {"CHK007", "无菌换药服务",   "DEP0002", 15.00},
    {"CHK008", "外科缝合术",     "DEP0002", 40.00},
    {"CHK009", "浅表肿物切除术", "DEP0002", 80.00},
    {"CHK010", "伤口拆线服务",   "DEP0002", 20.00},
    // DEP0003 儿科
    {"CHK011", "儿童血常规",     "DEP0003", 25.00},
    {"CHK012", "儿童微量元素检测","DEP0003", 45.00},
    {"CHK013", "骨密度检测",     "DEP0003", 55.00},
    {"CHK014", "儿童听力筛查",   "DEP0003", 30.00},
    {"CHK015", "儿童视力筛查",   "DEP0003", 30.00},
    // DEP0004 骨科
    {"CHK016", "骨密度检测",     "DEP0004", 55.00},
    {"CHK017", "关节X光片",      "DEP0004", 45.00},
    {"CHK018", "腰椎CT检查",     "DEP0004", 180.00},
    {"CHK019", "核磁共振成像(MRI)","DEP0004", 350.00},
    {"CHK020", "骨折复位检查",   "DEP0004", 40.00},
    // DEP0005 皮肤科
    {"CHK021", "皮肤真菌镜检",   "DEP0005", 20.00},
    {"CHK022", "过敏原检测",     "DEP0005", 80.00},
    {"CHK023", "伍德灯检查",     "DEP0005", 15.00},
    {"CHK024", "皮肤镜检查",     "DEP0005", 30.00},
    {"CHK025", "斑贴试验",       "DEP0005", 50.00},
};
static int internalCheckItemCount = sizeof(internalCheckItems) / sizeof(internalCheckItems[0]);

static int currentPage = PAGE_HOME;
static int lastPage = PAGE_HOME;
static char resultText[RESULT_LEN];
static int resultPageNo = 0;
static char loggedPatientId[ID_LEN] = "";
static char loggedDoctorId[ID_LEN] = "";
static int adminLoggedIn = 0;
static char currentAdminName[NAME_LEN] = "";
static char lastAutoBackupDate[DATE_LEN] = "";
static DWORD lastAutoBackupTick = 0;

typedef struct AuthAccount {
    char role[STATUS_LEN];
    char phone[PHONE_LEN];
    char password[STATUS_LEN];
    char targetId[ID_LEN];
} AuthAccount;

static std::vector<AuthAccount> authAccounts;

static void drawTop(const wchar_t *title);
static void drawButtons(Button buttons[], int count);
static void drawSmallNote(const wchar_t *text);
static int hit(Button buttons[], int count, int x, int y);
static int checkPayPassword6(const char *s);
static int runPatientNameInput(char *name, int nameSize);
static Patient *findPatientByPhoneLocal(const char *phone);
static AuthAccount *findAuthByPhone(const char *phone);
static AuthAccount *findAuthByLogin(const char *role, const char *phone, const char *password);
static AuthAccount *findAuthByTarget(const char *role, const char *targetId);
static int addAuthAccount(const char *role, const char *phone, const char *password, const char *targetId);
static int loadAuthData(const char *path);
static int saveAuthData(const char *path);
static Patient *currentPatient(void);
static Doctor *currentDoctor(void);
static Patient *choosePatientForCurrentDoctor(void);
static void doPatientLogin(void);
static void doPatientRegister(void);
static void doDoctorLogin(void);
static void doDoctorRegister(void);
static void doAdminLogin(void);
static void writeAdminLog(const char *action, const char *object, const char *result);
static void buildDashboardStats(DashboardStats *stats);
static void buildWarningStats(WarningStats *stats);
static void buildWarningReport(char *out, int outSize);
static void showWarningCenter(void);
static void showAdminLogs(void);
static int performBackup(const char *reason, char *out, int outSize);
static int restoreLatestBackup(char *out, int outSize);
static void manageBackupRestore(void);
static void checkAutoBackup(void);
static void drawAdminDashboard(void);

static std::wstring toWide(const char *s)
{
    int len;
    std::wstring out;
    if (s == NULL) return L"";
    len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (len <= 0) return L"";
    out.resize(len - 1);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], len);
    return out;
}

static std::string toUtf8(const wchar_t *s)
{
    int len;
    std::string out;
    if (s == NULL) return "";
    len = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    out.resize(len - 1);
    WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], len, NULL, NULL);
    return out;
}

static void message(const wchar_t *text)
{
    MessageBox(GetHWnd(), text, L"提示", MB_OK);
}

static void useClearFont(int height, int weight = FW_NORMAL)
{
    LOGFONT font;
    ZeroMemory(&font, sizeof(font));
    font.lfHeight = height;
    font.lfWeight = weight;
    font.lfCharSet = DEFAULT_CHARSET;
    font.lfQuality = CLEARTYPE_QUALITY;
    font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(font.lfFaceName, LF_FACESIZE, L"Microsoft YaHei UI");
    settextstyle(&font);
}

enum FormFieldKind {
    FORM_NAME,
    FORM_TEXT,
    FORM_MEMO,
    FORM_DATE,
    FORM_PHONE,
    FORM_CARD,
    FORM_INT,
    FORM_MONEY,
    FORM_GENDER,
    FORM_PAY_PASSWORD
};

typedef struct FormField {
    const wchar_t *label;
    const wchar_t *hint;
    char *value;
    int valueSize;
    FormFieldKind kind;
    int minValue;
    int maxValue;
} FormField;

static const wchar_t *formKindHint(const FormField *field)
{
    static wchar_t hint[128];
    if (field->hint != NULL && field->hint[0] != L'\0') return field->hint;
    if (field->kind == FORM_NAME) return L"2-20位，不能含 | 或换行";
    if (field->kind == FORM_TEXT) return L"可为空，不能含 | 或换行";
    if (field->kind == FORM_MEMO) return L"至少包含1个汉字，最多255字符，不能含 | 或换行";
    if (field->kind == FORM_DATE) return L"YYYY-MM-DD，例如 2026-05-15";
    if (field->kind == FORM_PHONE) return L"11位数字";
    if (field->kind == FORM_CARD) return L"18位，前17位数字，最后一位数字或X";
    if (field->kind == FORM_MONEY) return L"0-999999.99，最多两位小数";
    if (field->kind == FORM_GENDER) return L"只能输入 男 或 女";
    if (field->kind == FORM_PAY_PASSWORD) return L"恰好6位数字";
    swprintf(hint, 128, L"%d-%d 的整数", field->minValue, field->maxValue);
    return hint;
}

static int validateFormField(const FormField *field, wchar_t *msg, int msgSize)
{
    int intValue;
    double moneyValue;
    if (field->kind == FORM_NAME && !checkName(field->value)) {
        swprintf(msg, msgSize, L"%s 不符合限制：%s", field->label, formKindHint(field));
        return 0;
    }
    if (field->kind == FORM_TEXT && hasBadChar(field->value)) {
        swprintf(msg, msgSize, L"%s 不能包含 | 或换行。", field->label);
        return 0;
    }
    if (field->kind == FORM_MEMO && !checkMemo(field->value)) {
        swprintf(msg, msgSize, L"%s 不符合限制：%s", field->label, formKindHint(field));
        return 0;
    }
    if (field->kind == FORM_DATE && !checkDate(field->value)) {
        swprintf(msg, msgSize, L"%s 日期格式或日期值错误。", field->label);
        return 0;
    }
    if (field->kind == FORM_PHONE && !checkPhone(field->value)) {
        swprintf(msg, msgSize, L"%s 必须是11位数字。", field->label);
        return 0;
    }
    if (field->kind == FORM_CARD && !checkCardId(field->value)) {
        swprintf(msg, msgSize, L"%s 格式错误。", field->label);
        return 0;
    }
    if (field->kind == FORM_INT &&
        (!parseIntStrict(field->value, &intValue) || intValue < field->minValue || intValue > field->maxValue)) {
        swprintf(msg, msgSize, L"%s 必须是 %d-%d 的整数。", field->label, field->minValue, field->maxValue);
        return 0;
    }
    if (field->kind == FORM_MONEY && !parseMoneyStrict(field->value, &moneyValue)) {
        swprintf(msg, msgSize, L"%s 金额格式错误。", field->label);
        return 0;
    }
    if (field->kind == FORM_GENDER && !checkGender(field->value)) {
        swprintf(msg, msgSize, L"%s 只能输入男或女。", field->label);
        return 0;
    }
    if (field->kind == FORM_PAY_PASSWORD && !checkPayPassword6(field->value)) {
        swprintf(msg, msgSize, L"%s 必须是6位数字。", field->label);
        return 0;
    }
    return 1;
}

static void drawFormFields(const wchar_t *title, const wchar_t *note, FormField fields[], int count)
{
    int i;
    int twoCol = count > 4;
    int boxW = twoCol ? 320 : 560;
    int boxH = 48;
    int labelW = 118;
    int rowGap = 68;
    int leftX[2] = {70, 535};
    int topY = 195;
    Button buttons[] = {
        {360, 500, 500, 550, L"确定", 201},
        {560, 500, 700, 550, L"取消", 202}
    };
    drawTop(title);
    drawSmallNote(note);
    useClearFont(22);
    setbkmode(TRANSPARENT);
    for (i = 0; i < count; i++) {
        int col = twoCol ? (i % 2) : 0;
        int row = twoCol ? (i / 2) : i;
        int x = leftX[col];
        int y = topY + row * rowGap;
        RECT labelRect = { x, y + 5, x + labelW, y + boxH };
        RECT valueRect = { x + labelW + 16, y, x + labelW + 16 + boxW, y + boxH };
        settextcolor(RGB(20, 36, 64));
        drawtext(fields[i].label, &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        setfillcolor(WHITE);
        setlinecolor(RGB(128, 145, 170));
        setlinestyle(PS_SOLID, 2);
        solidrectangle(valueRect.left, valueRect.top, valueRect.right, valueRect.bottom);
        rectangle(valueRect.left, valueRect.top, valueRect.right, valueRect.bottom);
        useClearFont(22);
        settextcolor(fields[i].value[0] == '\0' ? RGB(82, 95, 115) : RGB(15, 23, 42));
        {
            std::wstring value = fields[i].value[0] == '\0' ? formKindHint(&fields[i]) : toWide(fields[i].value);
            RECT textRect = { valueRect.left + 10, valueRect.top, valueRect.right - 10, valueRect.bottom };
            drawtext(value.c_str(), &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        useClearFont(22);
    }
    drawButtons(buttons, 2);
    setlinestyle(PS_SOLID, 1);
}

static int hitFormField(FormField fields[], int count, int x, int y)
{
    int i;
    int twoCol = count > 4;
    int boxW = twoCol ? 320 : 560;
    int boxH = 48;
    int labelW = 118;
    int rowGap = 68;
    int leftX[2] = {70, 535};
    int topY = 195;
    for (i = 0; i < count; i++) {
        int col = twoCol ? (i % 2) : 0;
        int row = twoCol ? (i / 2) : i;
        int bx1 = leftX[col] + labelW + 16;
        int by1 = topY + row * rowGap;
        int bx2 = bx1 + boxW;
        int by2 = by1 + boxH;
        if (x >= bx1 && x <= bx2 && y >= by1 && y <= by2) return i;
    }
    return -1;
}

static int runForm(const wchar_t *title, const wchar_t *note, FormField fields[], int count)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"", 201},
        {560, 500, 700, 550, L"", 202}
    };
    drawFormFields(title, note, fields, count);
    while (1) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int fieldIndex = hitFormField(fields, count, msg.x, msg.y);
                int action = hit(buttons, 2, msg.x, msg.y);
                if (fieldIndex >= 0) {
                    wchar_t buf[512];
                    std::wstring oldValue = toWide(fields[fieldIndex].value);
                    safeCopy(fields[fieldIndex].value, fields[fieldIndex].valueSize, "");
                    wcsncpy_s(buf, 512, oldValue.c_str(), _TRUNCATE);
                    if (InputBox(buf, 512, formKindHint(&fields[fieldIndex]), fields[fieldIndex].label, oldValue.c_str(), 0, 0, false)) {
                        safeCopy(fields[fieldIndex].value, fields[fieldIndex].valueSize, toUtf8(buf).c_str());
                    } else {
                        safeCopy(fields[fieldIndex].value, fields[fieldIndex].valueSize, toUtf8(oldValue.c_str()).c_str());
                    }
                    drawFormFields(title, note, fields, count);
                } else if (action == 201) {
                    int i;
                    wchar_t msgText[256];
                    for (i = 0; i < count; i++) {
                        if (!validateFormField(&fields[i], msgText, 256)) {
                            message(msgText);
                            drawFormFields(title, note, fields, count);
                            break;
                        }
                    }
                    if (i == count) return 1;
                } else if (action == 202) {
                    return 0;
                }
            }
        }
        Sleep(10);
    }
}

static void drawPromptInputForm(const wchar_t *title, const char *promptText,
    const wchar_t *label, const wchar_t *hint, const char *value)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"确定", 201},
        {560, 500, 700, 550, L"取消", 202}
    };
    RECT promptRect = {70, 135, 990, 430};
    RECT labelRect = {145, 460, 300, 512};
    RECT boxRect = {320, 456, 910, 512};
    RECT valueRect = {336, 456, 894, 512};
    drawTop(title);
    drawSmallNote(L"请先查看页面内容，再点击输入框填写。");
    setfillcolor(WHITE);
    setlinecolor(RGB(128, 145, 170));
    setlinestyle(PS_SOLID, 2);
    solidrectangle(promptRect.left, promptRect.top, promptRect.right, promptRect.bottom);
    rectangle(promptRect.left, promptRect.top, promptRect.right, promptRect.bottom);
    useClearFont(22);
    settextcolor(RGB(15, 23, 42));
    {
        RECT textRect = {promptRect.left + 18, promptRect.top + 14, promptRect.right - 18, promptRect.bottom - 14};
        drawtext(toWide(promptText).c_str(), &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }
    useClearFont(22);
    settextcolor(RGB(20, 36, 64));
    drawtext(label, &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    setfillcolor(WHITE);
    setlinecolor(RGB(128, 145, 170));
    solidrectangle(boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
    rectangle(boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
    useClearFont(22);
    settextcolor(value[0] == '\0' ? RGB(82, 95, 115) : RGB(15, 23, 42));
    drawtext(value[0] == '\0' ? hint : toWide(value).c_str(), &valueRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    drawButtons(buttons, 2);
    setlinestyle(PS_SOLID, 1);
}

static int runPromptInput(const wchar_t *title, const char *promptText,
    const wchar_t *label, const wchar_t *hint, char *out, int outSize)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"", 201},
        {560, 500, 700, 550, L"", 202}
    };
    RECT boxRect = {320, 456, 910, 512};
    drawPromptInputForm(title, promptText, label, hint, out);
    while (1) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int action = hit(buttons, 2, msg.x, msg.y);
                if (msg.x >= boxRect.left && msg.x <= boxRect.right &&
                    msg.y >= boxRect.top && msg.y <= boxRect.bottom) {
                    wchar_t buf[512];
                    std::wstring oldValue = toWide(out);
                    wcsncpy_s(buf, 512, oldValue.c_str(), _TRUNCATE);
                    if (InputBox(buf, 512, hint, label, oldValue.c_str(), 0, 0, false)) {
                        safeCopy(out, outSize, toUtf8(buf).c_str());
                    }
                    drawPromptInputForm(title, promptText, label, hint, out);
                } else if (action == 201) {
                    return 1;
                } else if (action == 202) {
                    return 0;
                }
            }
        }
        Sleep(10);
    }
}

static void drawPromptForm(const wchar_t *title, const char *promptText, FormField fields[], int count)
{
    int i;
    Button buttons[] = {
        {360, 500, 500, 550, L"确定", 201},
        {560, 500, 700, 550, L"取消", 202}
    };
    RECT promptRect = {70, 135, 990, 390};
    int labelW = 110;
    int boxW = count > 1 ? 300 : 560;
    int boxH = 48;
    int startX[2] = {70, 535};
    int startY = 415;
    drawTop(title);
    drawSmallNote(L"请在同一页完成本次流程需要填写的内容。");
    setfillcolor(WHITE);
    setlinecolor(RGB(128, 145, 170));
    setlinestyle(PS_SOLID, 2);
    solidrectangle(promptRect.left, promptRect.top, promptRect.right, promptRect.bottom);
    rectangle(promptRect.left, promptRect.top, promptRect.right, promptRect.bottom);
    useClearFont(22);
    settextcolor(RGB(15, 23, 42));
    {
        RECT textRect = {promptRect.left + 18, promptRect.top + 14, promptRect.right - 18, promptRect.bottom - 14};
        drawtext(toWide(promptText).c_str(), &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }
    for (i = 0; i < count; i++) {
        int col = count > 1 ? (i % 2) : 0;
        int row = count > 2 ? (i / 2) : 0;
        int x = startX[col];
        int y = startY + row * 58;
        RECT labelRect = {x, y + 4, x + labelW, y + boxH};
        RECT boxRect = {x + labelW + 14, y, x + labelW + 14 + boxW, y + boxH};
        RECT valueRect = {boxRect.left + 10, boxRect.top, boxRect.right - 10, boxRect.bottom};
        useClearFont(22);
        settextcolor(RGB(20, 36, 64));
        drawtext(fields[i].label, &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        setfillcolor(WHITE);
        setlinecolor(RGB(128, 145, 170));
        solidrectangle(boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
        rectangle(boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
        useClearFont(22);
        settextcolor(fields[i].value[0] == '\0' ? RGB(82, 95, 115) : RGB(15, 23, 42));
        drawtext(fields[i].value[0] == '\0' ? formKindHint(&fields[i]) : toWide(fields[i].value).c_str(),
            &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    drawButtons(buttons, 2);
    setlinestyle(PS_SOLID, 1);
}

static int hitPromptFormField(int count, int x, int y)
{
    int i;
    int labelW = 110;
    int boxW = count > 1 ? 300 : 560;
    int boxH = 48;
    int startX[2] = {70, 535};
    int startY = 415;
    for (i = 0; i < count; i++) {
        int col = count > 1 ? (i % 2) : 0;
        int row = count > 2 ? (i / 2) : 0;
        int bx1 = startX[col] + labelW + 14;
        int by1 = startY + row * 58;
        int bx2 = bx1 + boxW;
        int by2 = by1 + boxH;
        if (x >= bx1 && x <= bx2 && y >= by1 && y <= by2) return i;
    }
    return -1;
}

static int runPromptForm(const wchar_t *title, const char *promptText, FormField fields[], int count)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"", 201},
        {560, 500, 700, 550, L"", 202}
    };
    drawPromptForm(title, promptText, fields, count);
    while (1) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int fieldIndex = hitPromptFormField(count, msg.x, msg.y);
                int action = hit(buttons, 2, msg.x, msg.y);
                if (fieldIndex >= 0) {
                    wchar_t buf[512];
                    std::wstring oldValue = toWide(fields[fieldIndex].value);
                    wcsncpy_s(buf, 512, oldValue.c_str(), _TRUNCATE);
                    if (InputBox(buf, 512, formKindHint(&fields[fieldIndex]), fields[fieldIndex].label,
                            oldValue.c_str(), 0, 0, false)) {
                        safeCopy(fields[fieldIndex].value, fields[fieldIndex].valueSize, toUtf8(buf).c_str());
                    }
                    drawPromptForm(title, promptText, fields, count);
                } else if (action == 201) {
                    int i;
                    wchar_t msgText[256];
                    for (i = 0; i < count; i++) {
                        if (!validateFormField(&fields[i], msgText, 256)) {
                            message(msgText);
                            drawPromptForm(title, promptText, fields, count);
                            break;
                        }
                    }
                    if (i == count) return 1;
                } else if (action == 202) {
                    return 0;
                }
            }
        }
        Sleep(10);
    }
}

static void drawPatientNameInput(const char *name)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"确定", 201},
        {560, 500, 700, 550, L"取消", 202}
    };
    RECT labelRect = {210, 250, 380, 315};
    RECT boxRect = {400, 250, 850, 315};
    RECT valueRect = {418, 250, 832, 315};
    drawTop(L"选择患者");
    useClearFont(24, FW_SEMIBOLD);
    settextcolor(RGB(20, 36, 64));
    drawtext(L"患者姓名", &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    setfillcolor(WHITE);
    setlinecolor(RGB(88, 110, 140));
    setlinestyle(PS_SOLID, 2);
    solidrectangle(boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
    rectangle(boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
    useClearFont(24, FW_SEMIBOLD);
    settextcolor(name[0] == '\0' ? RGB(82, 95, 115) : RGB(15, 23, 42));
    drawtext(name[0] == '\0' ? L"请输入患者姓名" : toWide(name).c_str(),
        &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    drawButtons(buttons, 2);
    setlinestyle(PS_SOLID, 1);
}

static int runPatientNameInput(char *name, int nameSize)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"", 201},
        {560, 500, 700, 550, L"", 202}
    };
    RECT boxRect = {400, 250, 850, 315};
    clearText(name, nameSize);
    drawPatientNameInput(name);
    while (1) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int action = hit(buttons, 2, msg.x, msg.y);
                if (msg.x >= boxRect.left && msg.x <= boxRect.right &&
                    msg.y >= boxRect.top && msg.y <= boxRect.bottom) {
                    wchar_t buf[512];
                    wcsncpy_s(buf, 512, toWide(name).c_str(), _TRUNCATE);
                    if (InputBox(buf, 512, L"2-20位，不能含 | 或换行", L"患者姓名",
                            toWide(name).c_str(), 0, 0, false)) {
                        safeCopy(name, nameSize, toUtf8(buf).c_str());
                    }
                    drawPatientNameInput(name);
                } else if (action == 201) {
                    if (!checkName(name)) {
                        message(L"患者姓名不符合限制：2-20位，不能含 | 或换行");
                        drawPatientNameInput(name);
                    } else {
                        return 1;
                    }
                } else if (action == 202) {
                    return 0;
                }
            }
        }
        Sleep(10);
    }
}

static int askName(const wchar_t *title, char *out, int outSize)
{
    FormField fields[] = {
        {title, L"", out, outSize, FORM_NAME, 0, 0}
    };
    clearText(out, outSize);
    return runForm(title, L"点击输入框填写信息。", fields, 1);
}

static int askMemo(const wchar_t *title, char *out, int outSize)
{
    FormField fields[] = {
        {title, L"", out, outSize, FORM_MEMO, 0, 0}
    };
    clearText(out, outSize);
    return runForm(title, L"点击输入框填写信息。", fields, 1);
}

static int askText(const wchar_t *title, char *out, int outSize)
{
    FormField fields[] = {
        {title, L"", out, outSize, FORM_TEXT, 0, 0}
    };
    clearText(out, outSize);
    return runForm(title, L"点击输入框填写信息。", fields, 1);
}

static int askDate(const wchar_t *title, char *out, int outSize)
{
    FormField fields[] = {
        {title, L"", out, outSize, FORM_DATE, 0, 0}
    };
    if (!checkDate(out)) clearText(out, outSize);
    return runForm(title, L"点击输入框填写日期。", fields, 1);
}

static int askPhone(char *out, int outSize)
{
    FormField fields[] = {
        {L"联系电话", L"", out, outSize, FORM_PHONE, 0, 0}
    };
    clearText(out, outSize);
    return runForm(L"联系电话", L"点击输入框填写电话。", fields, 1);
}

static int askCard(char *out, int outSize)
{
    FormField fields[] = {
        {L"身份证号", L"", out, outSize, FORM_CARD, 0, 0}
    };
    clearText(out, outSize);
    return runForm(L"身份证号", L"点击输入框填写身份证号。", fields, 1);
}

static int askInt(const wchar_t *title, int minValue, int maxValue, int *value)
{
    char buf[64];
    FormField fields[] = {
        {title, L"", buf, sizeof(buf), FORM_INT, minValue, maxValue}
    };
    clearText(buf, sizeof(buf));
    if (!runForm(title, L"点击输入框填写整数。", fields, 1)) return 0;
    return parseIntStrict(buf, value);
}

static int askMoney(const wchar_t *title, double *value)
{
    char buf[64];
    FormField fields[] = {
        {title, L"", buf, sizeof(buf), FORM_MONEY, 0, 0}
    };
    clearText(buf, sizeof(buf));
    if (!runForm(title, L"点击输入框填写金额。", fields, 1)) return 0;
    return parseMoneyStrict(buf, value);
}

static int checkPayPassword6(const char *s)
{
    int i;
    int len = (int)strlen(s);
    if (len != 6) return 0;
    for (i = 0; i < 6; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}

static int askPayPassword(char *out, int outSize)
{
    FormField fields[] = {
        {L"支付密码", L"", out, outSize, FORM_PAY_PASSWORD, 0, 0}
    };
    clearText(out, outSize);
    return runForm(L"支付密码", L"点击输入框填写六位支付密码。", fields, 1);
}

static std::string trimAscii(const std::string &s)
{
    size_t start = 0;
    size_t end = s.size();
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) start++;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) end--;
    return s.substr(start, end - start);
}

static int parseNumberedChoiceLine(const std::string &line, int *number, std::string *text)
{
    size_t i = 0;
    int value = 0;
    std::string s = trimAscii(line);
    if (s.empty() || s[0] < '0' || s[0] > '9') return 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
        value = value * 10 + (s[i] - '0');
        i++;
    }
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
    if (i >= s.size() || (s[i] != '.' && s[i] != ')' && s[i] != ':')) return 0;
    i++;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
    *number = value;
    *text = trimAscii(s.substr(i));
    return 1;
}

static void splitLines(const char *text, std::vector<std::string> *lines)
{
    const char *start = text;
    const char *p = text;
    lines->clear();
    while (*p != '\0') {
        if (*p == '\n') {
            lines->push_back(std::string(start, p - start));
            start = p + 1;
        }
        p++;
    }
    if (p != start) lines->push_back(std::string(start, p - start));
}

static void extractChoiceItems(const char *choices, int maxChoice,
    std::vector<std::string> *items, std::string *header)
{
    std::vector<std::string> lines;
    int i;
    int currentIndex = -1;
    items->assign(maxChoice, "");
    header->clear();
    splitLines(choices, &lines);
    for (i = 0; i < (int)lines.size(); i++) {
        int number = 0;
        std::string text;
        if (parseNumberedChoiceLine(lines[i], &number, &text) && number >= 1 && number <= maxChoice) {
            (*items)[number - 1] = text.empty() ? trimAscii(lines[i]) : text;
            currentIndex = number - 1;
        } else if (items->empty() || (*items)[0].empty()) {
            std::string line = trimAscii(lines[i]);
            if (!line.empty() && line.find("序号") == std::string::npos && line.find("请选择") == std::string::npos) {
                if (!header->empty()) header->append("\n");
                header->append(line);
            }
        } else if (currentIndex >= 0 && currentIndex < maxChoice) {
            std::string line = trimAscii(lines[i]);
            if (!line.empty()) {
                (*items)[currentIndex].append("\n");
                (*items)[currentIndex].append(line);
            }
        }
    }
    for (i = 0; i < maxChoice; i++) {
        if ((*items)[i].empty()) {
            char fallback[32];
            snprintf(fallback, sizeof(fallback), "选项 %d", i + 1);
            (*items)[i] = fallback;
        }
    }
}

static int choiceItemsPerPage(const std::string &header)
{
    (void)header;
    return 4;
}

static void drawChoicePage(const wchar_t *title, const std::vector<std::string> &items,
    const std::string &header, int page)
{
    int perPage = choiceItemsPerPage(header);
    int start = page * perPage;
    int end = start + perPage;
    int totalPage = ((int)items.size() + perPage - 1) / perPage;
    int startY = header.empty() ? 160 : 200;
    int i;
    wchar_t pageText[64];
    Button nav[] = {
        {300, 500, 440, 550, L"上一页", 901},
        {470, 500, 610, 550, L"下一页", 902},
        {780, 500, 930, 550, L"取消", 202}
    };
    if (end > (int)items.size()) end = (int)items.size();
    drawTop(title);
    if (!header.empty()) {
        RECT headerRect = {120, 140, 940, 188};
        useClearFont(22, FW_SEMIBOLD);
        settextcolor(RGB(25, 35, 50));
        drawtext(toWide(header.c_str()).c_str(), &headerRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }
    setbkmode(TRANSPARENT);
    for (i = start; i < end; i++) {
        int row = i - start;
        int y1 = startY + row * 72;
        RECT r = {120, y1, 940, y1 + 60};
        int multiline = items[i].find('\n') != std::string::npos || items[i].size() > 52;
        setfillcolor(RGB(248, 250, 252));
        setlinecolor(RGB(92, 65, 154));
        setlinestyle(PS_SOLID, 2);
        solidrectangle(r.left, r.top, r.right, r.bottom);
        rectangle(r.left, r.top, r.right, r.bottom);
        settextcolor(RGB(15, 23, 42));
        {
            RECT textRect = {r.left + 18, r.top + 4, r.right - 18, r.bottom - 4};
            useClearFont(multiline ? 20 : 26, FW_SEMIBOLD);
            drawtext(toWide(items[i].c_str()).c_str(), &textRect,
                DT_LEFT | (multiline ? DT_TOP : DT_VCENTER) | (multiline ? DT_WORDBREAK : DT_SINGLELINE));
        }
    }
    if (totalPage > 1) {
        swprintf(pageText, 64, L"%d/%d", page + 1, totalPage);
        useClearFont(20, FW_SEMIBOLD);
        settextcolor(RGB(25, 35, 50));
        outtextxy(635, 515, pageText);
        drawButtons(nav, 3);
    } else {
        Button cancel[] = {
            {780, 500, 930, 550, L"取消", 202}
        };
        drawButtons(cancel, 1);
    }
    setlinestyle(PS_SOLID, 1);
}

static int hitChoiceItem(int itemCount, const std::string &header, int page, int x, int y)
{
    int perPage = choiceItemsPerPage(header);
    int start = page * perPage;
    int end = start + perPage;
    int startY = header.empty() ? 160 : 200;
    int i;
    if (end > itemCount) end = itemCount;
    for (i = start; i < end; i++) {
        int row = i - start;
        int y1 = startY + row * 72;
        if (x >= 120 && x <= 940 && y >= y1 && y <= y1 + 60) return i + 1;
    }
    return -1;
}

static int askChoice(const wchar_t *title, const char *choices, int maxChoice, int *choice)
{
    std::vector<std::string> items;
    std::string header;
    int perPage;
    int totalPage;
    int page = 0;
    Button nav[] = {
        {300, 500, 440, 550, L"", 901},
        {470, 500, 610, 550, L"", 902},
        {780, 500, 930, 550, L"", 202}
    };
    Button cancel[] = {
        {780, 500, 930, 550, L"", 202}
    };
    if (maxChoice <= 0) return 0;
    extractChoiceItems(choices, maxChoice, &items, &header);
    perPage = choiceItemsPerPage(header);
    totalPage = (maxChoice + perPage - 1) / perPage;
    drawChoicePage(title, items, header, page);
    while (1) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int selected = hitChoiceItem(maxChoice, header, page, msg.x, msg.y);
                int action = totalPage > 1 ? hit(nav, 3, msg.x, msg.y) : hit(cancel, 1, msg.x, msg.y);
                if (selected >= 1 && selected <= maxChoice) {
                    *choice = selected;
                    return 1;
                }
                if (action == 901 && page > 0) {
                    page--;
                    drawChoicePage(title, items, header, page);
                } else if (action == 902 && page < totalPage - 1) {
                    page++;
                    drawChoicePage(title, items, header, page);
                } else if (action == 202) {
                    return 0;
                }
            }
        }
        Sleep(10);
    }
}

// 答辩注意：获取当前日期字符串，格式YYYY-MM-DD，用于默认日期填充（难度：简单）
static void getCurrentDate(char *out, int outSize)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    strftime(out, outSize, "%Y-%m-%d", &t);
}

// 答辩注意：获取当前时间字符串，格式YYYY-MM-DD HH:MM:SS，用于操作时间戳（难度：简单）
static void getCurrentDateTime(char *out, int outSize)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    strftime(out, outSize, "%Y-%m-%d %H:%M:%S", &t);
}

static void openResult(const char *text)
{
    safeCopy(resultText, RESULT_LEN, text);
    resultPageNo = 0;
    lastPage = currentPage;
    currentPage = PAGE_RESULT;
}

static void removeLineEnd(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static int isDirectoryA(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static int isFileA(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static int ensureDirectoryA(const char *path)
{
    if (isDirectoryA(path)) return 1;
    if (CreateDirectoryA(path, NULL)) return 1;
    return GetLastError() == ERROR_ALREADY_EXISTS && isDirectoryA(path);
}

static const char *adminNameForLog(void)
{
    return currentAdminName[0] ? currentAdminName : "admin";
}

static void writeAdminLog(const char *action, const char *object, const char *result)
{
    FILE *fp;
    char timestamp[TIME_LEN];
    if (!ensureDirectoryA("data")) return;
    fp = fopen("data\\admin_log.txt", "a");
    if (fp == NULL) return;
    getCurrentDateTime(timestamp, sizeof(timestamp));
    fprintf(fp, "%s|%s|%s|%s|%s\n",
        timestamp,
        adminNameForLog(),
        action ? action : "",
        object ? object : "",
        result ? result : "");
    fclose(fp);
}

static void buildDashboardStats(DashboardStats *stats)
{
    Payment *payment;
    MedicalRecord *record;
    Medicine *medicine;
    Ward *ward;
    Bed *bed;
    char today[DATE_LEN];
    int wardTotal = 0;
    int wardFree = 0;

    memset(stats, 0, sizeof(*stats));
    getCurrentDate(today, sizeof(today));

    payment = paymentHead;
    while (payment != NULL) {
        if (strcmp(payment->status, "已缴费") == 0) {
            stats->totalIncome += payment->amount;
        } else if (strcmp(payment->status, "未缴费") == 0) {
            stats->unpaidCount++;
        }
        payment = payment->next;
    }

    record = recordHead;
    while (record != NULL) {
        if (strcmp(record->type, "挂号") == 0 && strcmp(record->date, today) == 0) {
            stats->todayRegisterCount++;
        }
        record = record->next;
    }

    ward = wardHead;
    while (ward != NULL) {
        wardTotal += ward->totalBeds;
        wardFree += ward->freeBeds;
        ward = ward->next;
    }
    if (wardTotal > 0) {
        stats->totalBeds = wardTotal;
        stats->occupiedBeds = wardTotal - wardFree;
    } else {
        bed = bedHead;
        while (bed != NULL) {
            stats->totalBeds++;
            if (strcmp(bed->status, "占用") == 0) stats->occupiedBeds++;
            bed = bed->next;
        }
    }
    if (stats->occupiedBeds < 0) stats->occupiedBeds = 0;
    if (stats->occupiedBeds > stats->totalBeds) stats->occupiedBeds = stats->totalBeds;
    if (stats->totalBeds > 0) {
        stats->bedOccupancyRate = stats->occupiedBeds * 100.0 / stats->totalBeds;
    }

    medicine = medicineHead;
    while (medicine != NULL) {
        if (medicine->stock <= medicine->lowLimit) stats->lowStockMedicineCount++;
        medicine = medicine->next;
    }
}

static void buildWarningStats(WarningStats *stats)
{
    Medicine *medicine;
    Payment *payment;
    Ward *ward;
    Doctor *doctor;

    memset(stats, 0, sizeof(*stats));

    medicine = medicineHead;
    while (medicine != NULL) {
        if (medicine->stock <= medicine->lowLimit) stats->lowStockMedicineCount++;
        medicine = medicine->next;
    }

    payment = paymentHead;
    while (payment != NULL) {
        if (strcmp(payment->status, "未缴费") == 0) stats->unpaidPaymentCount++;
        payment = payment->next;
    }

    ward = wardHead;
    while (ward != NULL) {
        if (ward->totalBeds > 0 && (ward->freeBeds <= 1 || ward->freeBeds * 100 <= ward->totalBeds * 10)) {
            stats->bedShortageWardCount++;
        }
        ward = ward->next;
    }

    doctor = doctorHead;
    while (doctor != NULL) {
        if (doctor->maxCount <= 3) stats->lowDoctorSlotCount++;
        doctor = doctor->next;
    }
}

static void buildWarningReport(char *out, int outSize)
{
    Medicine *medicine;
    Payment *payment;
    Ward *ward;
    Doctor *doctor;
    char line[LINE_LEN];
    int count;

    clearText(out, outSize);
    appendLine(out, outSize, "预警中心");
    appendLine(out, outSize, "用于集中查看库存、缴费、床位和医生号源风险。\n");

    appendLine(out, outSize, "【低库存药品】");
    count = 0;
    medicine = medicineHead;
    while (medicine != NULL) {
        if (medicine->stock <= medicine->lowLimit) {
            Department *dept = findDepartmentById(medicine->deptId);
            snprintf(line, sizeof(line), "%d. %s  科室：%s  库存：%d  下限：%d\n",
                ++count, medicine->name, dept ? dept->name : "未知科室", medicine->stock, medicine->lowLimit);
            appendText(out, outSize, line);
        }
        medicine = medicine->next;
    }
    if (count == 0) appendLine(out, outSize, "暂无低库存药品。");

    appendLine(out, outSize, "\n【未缴费账单】");
    count = 0;
    payment = paymentHead;
    while (payment != NULL) {
        if (strcmp(payment->status, "未缴费") == 0) {
            Patient *patient = findPatientById(payment->patientId);
            snprintf(line, sizeof(line), "%d. %s  患者：%s  类型：%s  金额：%.2f  日期：%s\n",
                ++count, payment->id, patient ? patient->name : "未知患者",
                payment->sourceType, payment->amount, payment->date);
            appendText(out, outSize, line);
        }
        payment = payment->next;
    }
    if (count == 0) appendLine(out, outSize, "暂无未缴费账单。");

    appendLine(out, outSize, "\n【空床不足病房】");
    count = 0;
    ward = wardHead;
    while (ward != NULL) {
        if (ward->totalBeds > 0 && (ward->freeBeds <= 1 || ward->freeBeds * 100 <= ward->totalBeds * 10)) {
            Department *dept = findDepartmentById(ward->deptId);
            snprintf(line, sizeof(line), "%d. %s  科室：%s  空床：%d/%d\n",
                ++count, ward->type, dept ? dept->name : "未知科室", ward->freeBeds, ward->totalBeds);
            appendText(out, outSize, line);
        }
        ward = ward->next;
    }
    if (count == 0) appendLine(out, outSize, "暂无床位紧张病房。");

    appendLine(out, outSize, "\n【医生号源不足】");
    count = 0;
    doctor = doctorHead;
    while (doctor != NULL) {
        if (doctor->maxCount <= 3) {
            Department *dept = findDepartmentById(doctor->deptId);
            snprintf(line, sizeof(line), "%d. %s  科室：%s  剩余号源：%d\n",
                ++count, doctor->name, dept ? dept->name : "未知科室", doctor->maxCount);
            appendText(out, outSize, line);
        }
        doctor = doctor->next;
    }
    if (count == 0) appendLine(out, outSize, "暂无号源不足医生。");
}

static const char *backupFileList[] = {
    "department.txt",
    "patient.txt",
    "doctor.txt",
    "medicine.txt",
    "ward.txt",
    "bed.txt",
    "record.txt",
    "prescription.txt",
    "prescription_item.txt",
    "payment.txt",
    "account.txt",
    "admin_log.txt"
};

static int backupFileCount(void)
{
    return (int)(sizeof(backupFileList) / sizeof(backupFileList[0]));
}

static void makeBackupFolderName(char *out, int outSize)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    strftime(out, outSize, "backup\\%Y-%m-%d_%H%M%S", &t);
}

static void copyTxtFiles(const char *fromDir, const char *toDir, int *copied, int *skipped, int *failed)
{
    int i;
    char src[MAX_PATH];
    char dst[MAX_PATH];
    *copied = 0;
    *skipped = 0;
    *failed = 0;
    for (i = 0; i < backupFileCount(); i++) {
        snprintf(src, sizeof(src), "%s\\%s", fromDir, backupFileList[i]);
        snprintf(dst, sizeof(dst), "%s\\%s", toDir, backupFileList[i]);
        if (!isFileA(src)) {
            (*skipped)++;
            continue;
        }
        if (CopyFileA(src, dst, FALSE)) {
            (*copied)++;
        } else {
            (*failed)++;
        }
    }
}

static int backupExistsForDate(const char *date)
{
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA data;
    HANDLE h;
    snprintf(pattern, sizeof(pattern), "backup\\%s_*", date);
    h = FindFirstFileA(pattern, &data);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(data.cFileName, ".") != 0 &&
            strcmp(data.cFileName, "..") != 0) {
            FindClose(h);
            return 1;
        }
    } while (FindNextFileA(h, &data));
    FindClose(h);
    return 0;
}

static int findLatestBackup(char *out, int outSize)
{
    WIN32_FIND_DATAA data;
    HANDLE h;
    char latest[MAX_PATH] = "";

    if (!isDirectoryA("backup")) return 0;
    h = FindFirstFileA("backup\\*", &data);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(data.cFileName, ".") != 0 &&
            strcmp(data.cFileName, "..") != 0 &&
            strchr(data.cFileName, '_') != NULL) {
            if (latest[0] == '\0' || strcmp(data.cFileName, latest) > 0) {
                safeCopy(latest, sizeof(latest), data.cFileName);
            }
        }
    } while (FindNextFileA(h, &data));
    FindClose(h);

    if (latest[0] == '\0') return 0;
    snprintf(out, outSize, "backup\\%s", latest);
    return 1;
}

static int performBackupCore(const char *reason, char *out, int outSize, int saveBeforeCopy)
{
    char folder[MAX_PATH];
    char saveMsg[TEXT_LEN];
    int copied, skipped, failed;

    clearText(out, outSize);
    if (!ensureDirectoryA("data") || !ensureDirectoryA("backup")) {
        appendLine(out, outSize, "备份失败：无法创建 data 或 backup 目录。");
        writeAdminLog(reason ? reason : "备份", "backup", "失败：目录不可用");
        return ERR_FILE;
    }

    if (saveBeforeCopy) {
        saveAllData("data", saveMsg, sizeof(saveMsg));
        saveAuthData("data\\account.txt");
    }
    makeBackupFolderName(folder, sizeof(folder));
    if (!ensureDirectoryA(folder)) {
        appendLine(out, outSize, "备份失败：无法创建备份目录。");
        writeAdminLog(reason ? reason : "备份", folder, "失败：目录创建失败");
        return ERR_FILE;
    }

    copyTxtFiles("data", folder, &copied, &skipped, &failed);
    snprintf(out, outSize,
        "%s完成\n备份目录：%s\n复制文件：%d\n跳过缺失文件：%d\n失败文件：%d\n",
        reason ? reason : "备份", folder, copied, skipped, failed);
    writeAdminLog(reason ? reason : "备份", folder, failed == 0 ? "成功" : "部分失败");
    return failed == 0 ? OK : ERR_FILE;
}

static int performBackup(const char *reason, char *out, int outSize)
{
    return performBackupCore(reason, out, outSize, 1);
}

static int restoreLatestBackup(char *out, int outSize)
{
    char folder[MAX_PATH];
    char loadMsg[TEXT_LEN];
    int copied, skipped, failed;

    clearText(out, outSize);
    if (!findLatestBackup(folder, sizeof(folder))) {
        appendLine(out, outSize, "恢复失败：没有找到可用备份。");
        writeAdminLog("恢复备份", "backup", "失败：无备份");
        return ERR_NOT_FOUND;
    }
    if (!ensureDirectoryA("data")) {
        appendLine(out, outSize, "恢复失败：无法创建 data 目录。");
        writeAdminLog("恢复备份", folder, "失败：data目录不可用");
        return ERR_FILE;
    }

    copyTxtFiles(folder, "data", &copied, &skipped, &failed);
    loadAllData("data", loadMsg, sizeof(loadMsg));
    loadAuthData("data\\account.txt");
    if (currentPatient() == NULL) clearText(loggedPatientId, sizeof(loggedPatientId));
    if (currentDoctor() == NULL) clearText(loggedDoctorId, sizeof(loggedDoctorId));

    snprintf(out, outSize,
        "恢复最近备份完成\n备份目录：%s\n恢复文件：%d\n备份中缺失文件：%d\n失败文件：%d\n\n%s",
        folder, copied, skipped, failed, loadMsg);
    writeAdminLog("恢复备份", folder, failed == 0 ? "成功" : "部分失败");
    return failed == 0 ? OK : ERR_FILE;
}

static void checkAutoBackup(void)
{
    char today[DATE_LEN];
    char out[RESULT_LEN];
    DWORD nowTick = GetTickCount();

    if (lastAutoBackupTick != 0 && nowTick - lastAutoBackupTick < 60000) return;
    lastAutoBackupTick = nowTick;

    getCurrentDate(today, sizeof(today));
    if (strcmp(lastAutoBackupDate, today) == 0) return;
    if (!backupExistsForDate(today)) {
        performBackupCore("自动备份", out, sizeof(out), 0);
    }
    safeCopy(lastAutoBackupDate, sizeof(lastAutoBackupDate), today);
}

static Patient *findPatientByPhoneLocal(const char *phone)
{
    Patient *p = patientHead;
    while (p != NULL) {
        if (strcmp(p->phone, phone) == 0) return p;
        p = p->next;
    }
    return NULL;
}

static AuthAccount *findAuthByPhone(const char *phone)
{
    int i;
    for (i = 0; i < (int)authAccounts.size(); i++) {
        if (strcmp(authAccounts[i].phone, phone) == 0) return &authAccounts[i];
    }
    return NULL;
}

static AuthAccount *findAuthByLogin(const char *role, const char *phone, const char *password)
{
    int i;
    for (i = 0; i < (int)authAccounts.size(); i++) {
        if (strcmp(authAccounts[i].role, role) == 0 &&
            strcmp(authAccounts[i].phone, phone) == 0 &&
            strcmp(authAccounts[i].password, password) == 0) {
            return &authAccounts[i];
        }
    }
    return NULL;
}

static AuthAccount *findAuthByTarget(const char *role, const char *targetId)
{
    int i;
    for (i = 0; i < (int)authAccounts.size(); i++) {
        if (strcmp(authAccounts[i].role, role) == 0 &&
            strcmp(authAccounts[i].targetId, targetId) == 0) {
            return &authAccounts[i];
        }
    }
    return NULL;
}

static int addAuthAccount(const char *role, const char *phone, const char *password, const char *targetId)
{
    AuthAccount account;
    if ((strcmp(role, "patient") != 0 && strcmp(role, "doctor") != 0) ||
        !checkPhone(phone) || !checkPayPassword6(password) || isBlank(targetId)) {
        return ERR_INPUT;
    }
    if (findAuthByPhone(phone) != NULL || findAuthByTarget(role, targetId) != NULL) {
        return ERR_REPEAT;
    }
    safeCopy(account.role, STATUS_LEN, role);
    safeCopy(account.phone, PHONE_LEN, phone);
    safeCopy(account.password, STATUS_LEN, password);
    safeCopy(account.targetId, ID_LEN, targetId);
    authAccounts.push_back(account);
    return OK;
}

static int loadAuthData(const char *path)
{
    FILE *fp;
    char line[LINE_LEN];
    authAccounts.clear();
    fp = fopen(path, "r");
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *role;
        char *phone;
        char *password;
        char *targetId;
        removeLineEnd(line);
        if (isBlank(line)) continue;
        role = strtok(line, "|");
        phone = strtok(NULL, "|");
        password = strtok(NULL, "|");
        targetId = strtok(NULL, "|");
        if (role == NULL || phone == NULL || password == NULL || targetId == NULL) {
            fclose(fp);
            authAccounts.clear();
            return ERR_FILE;
        }
        if (addAuthAccount(role, phone, password, targetId) != OK) {
            fclose(fp);
            authAccounts.clear();
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int saveAuthData(const char *path)
{
    FILE *fp = fopen(path, "w");
    int i;
    if (fp == NULL) return ERR_FILE;
    for (i = 0; i < (int)authAccounts.size(); i++) {
        fprintf(fp, "%s|%s|%s|%s\n",
            authAccounts[i].role, authAccounts[i].phone,
            authAccounts[i].password, authAccounts[i].targetId);
    }
    fclose(fp);
    return OK;
}

static Patient *currentPatient(void)
{
    if (loggedPatientId[0] == '\0') return NULL;
    return findPatientById(loggedPatientId);
}

static Doctor *currentDoctor(void)
{
    if (loggedDoctorId[0] == '\0') return NULL;
    return findDoctorById(loggedDoctorId);
}

static int containsPatient(Patient *list[], int count, const char *patientId)
{
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(list[i]->id, patientId) == 0) return 1;
    }
    return 0;
}

static Patient *choosePatientForCurrentDoctor(void)
{
    Doctor *doc = currentDoctor();
    Patient *list[50];
    int count, i, choice;
    char name[NAME_LEN];
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    if (doc == NULL) {
        message(L"请先登录医生账号。");
        return NULL;
    }
    while (1) {
        clearText(name, sizeof(name));
        if (!runPatientNameInput(name, sizeof(name))) return NULL;
        count = collectPatientsByName(name, list, 50);
        if (count == 0) {
            message(L"没有找到该患者，请重新输入姓名。");
            continue;
        }
        if (count == 1) return list[0];
        clearText(prompt, sizeof(prompt));
        {
            Department *dept = findDepartmentById(doc->deptId);
            snprintf(line, sizeof(line),
                "当前医生：%s  科室：%s\n找到多名同名患者，请按电话和身份证核对后点击患者按钮：\n",
                doc->name, dept ? dept->name : "未知科室");
            appendText(prompt, sizeof(prompt), line);
        }
        for (i = 0; i < count; i++) {
            snprintf(line, sizeof(line), "%d. %s  %s  电话：%s  身份证：%s  状态：%s\n",
                i + 1, list[i]->id, list[i]->name, list[i]->phone, list[i]->cardId, list[i]->status);
            appendText(prompt, sizeof(prompt), line);
        }
        if (!askChoice(L"选择患者", prompt, count, &choice)) return NULL;
        return list[choice - 1];
    }
}

static Patient *choosePatient(void)
{
    Patient *logged = currentPatient();
    if (currentPage == PAGE_PATIENT && logged != NULL) return logged;
    if (currentPage == PAGE_MEDICAL && loggedDoctorId[0] != '\0') {
        return choosePatientForCurrentDoctor();
    }
    char name[NAME_LEN];
    Patient *list[50];
    int count, i, choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(name, sizeof(name));
    clearText(prompt, sizeof(prompt));
    while (1) {
        if (!runPatientNameInput(name, sizeof(name))) return NULL;
        count = collectPatientsByName(name, list, 50);
        if (count == 0) {
            message(L"没有找到该患者，请重新输入姓名。");
            continue;
        }
        if (count == 1) return list[0];
        clearText(prompt, sizeof(prompt));
        appendLine(prompt, sizeof(prompt), "找到多名同名患者，请核对身份证和电话后点击患者按钮：");
        for (i = 0; i < count; i++) {
            snprintf(line, sizeof(line), "%d. %s 身份证：%s 电话：%s\n",
                i + 1, list[i]->name, list[i]->cardId, list[i]->phone);
            appendText(prompt, sizeof(prompt), line);
        }
        if (!askChoice(L"选择患者", prompt, count, &choice)) return NULL;
        return list[choice - 1];
    }
}

static Department *chooseDepartment(void)
{
    Department *p = departmentHead;
    Department *list[50];
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请点击科室按钮：");
    while (p != NULL && count < 50) {
        list[count] = p;
        snprintf(line, sizeof(line), "%d. %s  关联病房：%s\n", count + 1, p->name, p->wardType);
        appendText(prompt, sizeof(prompt), line);
        count++;
        p = p->next;
    }
    if (count == 0) {
        message(L"当前没有科室数据，请先在管理端添加科室。");
        return NULL;
    }
    if (!askChoice(L"选择科室", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static Doctor *chooseDoctorByDepartment(const char *deptId)
{
    Doctor *list[50];
    Doctor *p = doctorHead;
    Department *dept = findDepartmentById(deptId);
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    snprintf(line, sizeof(line), "当前科室：%s\n请点击医生按钮：\n", dept ? dept->name : "未知科室");
    appendText(prompt, sizeof(prompt), line);
    while (p != NULL && count < 50) {
        if (strcmp(p->deptId, deptId) == 0) {
            list[count] = p;
            {
                int used = countRegisterRecordsByDoctor(p->id);
                int remain = p->maxCount;
                int cap = remain + used;
                snprintf(line, sizeof(line), "%d. %s  职称：%s  出诊：%s  剩余号源：%d/%d\n",
                    count + 1, p->name, p->title, p->workTime, remain, cap);
            }
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        message(L"该科室没有医生，请先在管理端添加医生。");
        return NULL;
    }
    if (!askChoice(L"选择医生", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static double registerFeeByTitle(const char *title)
{
    if (title == NULL) return 10;
    if (strstr(title, "副主任医师") != NULL) return 25;
    if (strstr(title, "主任医师") != NULL) return 30;
    if (strstr(title, "主治医师") != NULL) return 10;
    if (strstr(title, "住院医师") != NULL) return 10;
    return 10;
}

static int ymdToCnWorkdigit(const char *ymd)
{
    struct tm t = {0};
    int y, m, d;
    time_t tt;
    if (!checkDate(ymd)) return -1;
    if (sscanf(ymd, "%d-%d-%d", &y, &m, &d) != 3) return -1;
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = 12;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = -1;
    tt = mktime(&t);
    if (tt == (time_t)-1) return -1;
    if (t.tm_wday == 0) return 7;
    return t.tm_wday;
}

static int doctorWorksOnDate(const Doctor *d, const char *ymd)
{
    char tag[16];
    int wd;
    if (d == NULL) return 0;
    wd = ymdToCnWorkdigit(ymd);
    if (wd < 0) return 0;
    snprintf(tag, sizeof(tag), "周%d", wd);
    return strstr(d->workTime, tag) != NULL;
}

static Doctor *chooseDoctorForRegister(const char *deptId, const char *visitDate)
{
    Doctor *list[50];
    Doctor *p = doctorHead;
    Department *dept = findDepartmentById(deptId);
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    double fee;
    clearText(prompt, sizeof(prompt));
    snprintf(line, sizeof(line), "就诊日期：%s\n当前科室：%s\n仅列出该日出诊的医生，请选择：\n",
        visitDate, dept ? dept->name : "未知科室");
    appendText(prompt, sizeof(prompt), line);
    while (p != NULL && count < 50) {
        if (strcmp(p->deptId, deptId) == 0 && doctorWorksOnDate(p, visitDate)) {
            list[count] = p;
            {
                int used = countRegisterRecordsByDoctor(p->id);
                int remain = p->maxCount;
                int cap = remain + used;
                fee = registerFeeByTitle(p->title);
                snprintf(line, sizeof(line), "%d. 医生：%s  职称：%s\n出诊：%s  剩余号源：%d/%d  挂号费：%.0f元\n",
                    count + 1, p->name, p->title, p->workTime, remain, cap, fee);
            }
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        message(L"所选日期该科室没有出诊医生，请更换就诊日期或科室。");
        return NULL;
    }
    if (!askChoice(L"选择医生", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static int chooseVisitDateForRegister(char *visitDate, int visitDateSize, int *regStatus)
{
    int opt;
    char today[DATE_LEN];
    getCurrentDate(today, sizeof(today));
    while (1) {
        getCurrentDate(today, sizeof(today));
        clearText(visitDate, visitDateSize);
        if (!askChoice(L"就诊日期", "1. 今天（直接挂号）\n2. 选择未来日期（预约）", 2, &opt)) return 0;
        if (opt == 1) {
            safeCopy(visitDate, visitDateSize, today);
            *regStatus = 1;
            return 1;
        }
        if (!askDate(L"预约日期", visitDate, visitDateSize)) return 0;
        if (!checkDate(visitDate) || compareDate(visitDate, today) <= 0) {
            message(L"预约日期必须是晚于今天的有效日期。");
            continue;
        }
        *regStatus = 0;
        return 1;
    }
}

static void buildCheckBundleNote(const char *deptId, const char *docId, const char *visitDate,
    const char *itemIds, const char *itemNames, char *out, int outSize)
{
    snprintf(out, outSize, "CHKBUNDLE;%s;%s;%s;%s;%s",
        deptId, docId ? docId : "", visitDate, itemIds, itemNames);
}

static int parseCheckBundleNote(const char *note, char *deptId, int deptSize,
    char *docId, int docSize, char *visitDate, int dateSize,
    char *itemIds, int idsSize, char *itemNames, int namesSize)
{
    char copy[TEXT_LEN];
    char *parts[6];
    int n = 0;
    char *p;
    if (note == NULL || strncmp(note, "CHKBUNDLE;", 10) != 0) return 0;
    safeCopy(copy, sizeof(copy), note);
    p = copy + 10;
    while (n < 6) {
        parts[n++] = p;
        p = strchr(p, ';');
        if (p == NULL) break;
        *p = '\0';
        p++;
    }
    if (n < 5) return 0;
    safeCopy(deptId, deptSize, parts[0]);
    safeCopy(docId, docSize, parts[1]);
    safeCopy(visitDate, dateSize, parts[2]);
    safeCopy(itemIds, idsSize, parts[3]);
    safeCopy(itemNames, namesSize, parts[4]);
    return 1;
}

static MedicalRecord *ensureActiveRegister(Patient *patient)
{
    MedicalRecord *reg = findLatestRegisterByPatient(patient->id);
    if (reg != NULL) return reg;
    message(L"该患者没有有效挂号记录（需已挂号状态），请先完成挂号。");
    return NULL;
}

static MedicalRecord *ensureCurrentVisit(Patient *patient)
{
    MedicalRecord *reg = findLatestRegisterByPatient(patient->id);
    MedicalRecord *visit = findLatestVisitByPatient(patient->id);
    if (visit != NULL) {
        if (reg == NULL ||
            compareDate(visit->date, reg->date) > 0 ||
            (strcmp(visit->date, reg->date) == 0 &&
                getIdNumber(visit->id, "REC") > getIdNumber(reg->id, "REC"))) {
            return visit;
        }
    }
    message(L"请先完成接诊，再为该患者开处方。");
    return NULL;
}

static Doctor *chooseDoctor(void)
{
    Doctor *list[80];
    Doctor *p = doctorHead;
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请点击医生按钮：");
    while (p != NULL && count < 80) {
        Department *dept = findDepartmentById(p->deptId);
        list[count] = p;
        snprintf(line, sizeof(line), "%d. %s  科室：%s  职称：%s  出诊：%s\n",
            count + 1, p->name, dept ? dept->name : "未知", p->title, p->workTime);
        appendText(prompt, sizeof(prompt), line);
        count++;
        p = p->next;
    }
    if (count == 0) {
        message(L"当前没有医生数据，请先在管理端添加医生。");
        return NULL;
    }
    if (!askChoice(L"选择医生", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static Medicine *chooseMedicine(void)
{
    Medicine *list[50];
    Medicine *p = medicineHead;
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请点击药品按钮：");
    while (p != NULL && count < 50) {
        Department *dept = findDepartmentById(p->deptId);
        list[count] = p;
        snprintf(line, sizeof(line), "%d. 药品：%s  通用名：%s\n商品名：%s  科室：%s  单价：%.2f  库存：%d\n",
            count + 1, p->name, p->commonName, p->tradeName,
            dept ? dept->name : "未知", p->price, p->stock);
        appendText(prompt, sizeof(prompt), line);
        count++;
        p = p->next;
    }
    if (count == 0) {
        message(L"当前没有药品数据，请先在管理端添加药品。");
        return NULL;
    }
    if (!askChoice(L"选择药品", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static Medicine *chooseMedicineByDepartment(const char *deptId)
{
    Medicine *list[50];
    Medicine *p = medicineHead;
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    Department *dept = findDepartmentById(deptId);
    clearText(prompt, sizeof(prompt));
    snprintf(line, sizeof(line), "当前科室：%s\n请点击药品按钮：\n", dept ? dept->name : "未知科室");
    appendText(prompt, sizeof(prompt), line);
    while (p != NULL && count < 50) {
        if (strcmp(p->deptId, deptId) == 0) {
            list[count] = p;
            snprintf(line, sizeof(line), "%d. 药品：%s  通用名：%s\n商品名：%s  单价：%.2f  库存：%d\n",
                count + 1, p->name, p->commonName, p->tradeName, p->price, p->stock);
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        message(L"该科室没有药品，请先在管理端添加药品。");
        return NULL;
    }
    if (!askChoice(L"选择药品", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

// 答辩注意：按科室筛选检查项目，支持多选，自动计算总费用（难度：中等）
static int chooseCheckItemsByDepartment(const char *deptId, int selectedIndices[], int maxSelected)
{
    CheckItem *list[50];
    int count = 0;
    int i;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    int choice;
    int selCount = 0;
    Department *dept = findDepartmentById(deptId);
    clearText(prompt, sizeof(prompt));
    snprintf(line, sizeof(line), "当前科室：%s\n请点击要选择的检查项目（可多选）：\n", dept ? dept->name : "未知科室");
    appendText(prompt, sizeof(prompt), line);
    for (i = 0; i < internalCheckItemCount; i++) {
        if (strcmp(internalCheckItems[i].deptId, deptId) == 0) {
            list[count] = &internalCheckItems[i];
            snprintf(line, sizeof(line), "%d. %s  单价：%.2f元\n", count + 1,
                list[count]->name, list[count]->price);
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
    }
    if (count == 0) {
        message(L"该科室没有预设检查项目。");
        return 0;
    }
    while (1) {
        char choicePrompt[RESULT_LEN];
        clearText(choicePrompt, sizeof(choicePrompt));
        appendText(choicePrompt, sizeof(choicePrompt), prompt);
        snprintf(line, sizeof(line), "%d. 完成选择（当前已选%d项）\n", count + 1, selCount);
        appendText(choicePrompt, sizeof(choicePrompt), line);
        snprintf(line, sizeof(line), "%d. 撤销上一项\n", count + 2);
        appendText(choicePrompt, sizeof(choicePrompt), line);
        if (!askChoice(L"检查项目选择", choicePrompt, count + 2, &choice)) {
            return selCount;
        }
        if (choice == count + 1) break;
        if (choice == count + 2) {
            if (selCount == 0) {
                message(L"当前没有已选项目，无法撤销。");
            } else {
                selCount--;
                message(L"已撤销上一项选择。");
            }
        } else if (choice >= 1 && choice <= count) {
            int already = 0;
            for (i = 0; i < selCount; i++) {
                if (selectedIndices[i] == choice - 1) {
                    already = 1;
                    break;
                }
            }
            if (!already && selCount < maxSelected) {
                selectedIndices[selCount++] = (int)(list[choice - 1] - internalCheckItems);
            } else if (already) {
                message(L"该项目已在选择列表中，请选择其他项目。");
            }
        } else {
            message(L"选择不在范围内，请重新选择。");
        }
    }
    return selCount;
}

static int confirmCheckItems(const char *detailText)
{
    char confirm[RESULT_LEN];
    snprintf(confirm, sizeof(confirm), "%s\n\n是否确认开立以上检查项目？", detailText);
    return MessageBox(GetHWnd(), toWide(confirm).c_str(), L"确认检查项目", MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
}

static Ward *chooseWardByDepartment(const char *deptId)
{
    Ward *p = wardHead;
    Ward *list[50];
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请点击病房按钮：");
    while (p != NULL && count < 50) {
        if (strcmp(p->deptId, deptId) == 0) {
            list[count] = p;
            snprintf(line, sizeof(line), "%d. %s  每日费用：%.2f  空床：%d/%d\n",
                count + 1, p->type, p->dailyFee, p->freeBeds, p->totalBeds);
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        message(L"该科室没有病房，请先在管理端添加病房。");
        return NULL;
    }
    if (!askChoice(L"选择病房", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static Ward *chooseWardAll(void)
{
    Ward *p = wardHead;
    Ward *list[80];
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请点击病房按钮：");
    while (p != NULL && count < 80) {
        Department *dept = findDepartmentById(p->deptId);
        list[count] = p;
        snprintf(line, sizeof(line), "%d. %s  科室：%s  日费：%.2f  空床：%d/%d\n",
            count + 1, p->type, dept ? dept->name : "未知", p->dailyFee, p->freeBeds, p->totalBeds);
        appendText(prompt, sizeof(prompt), line);
        count++;
        p = p->next;
    }
    if (count == 0) {
        message(L"当前没有病房数据，请先添加病房。");
        return NULL;
    }
    if (!askChoice(L"选择病房", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static Bed *chooseFreeBed(void)
{
    Bed *p = bedHead;
    Bed *list[100];
    int count = 0;
    int choice;
    char prompt[RESULT_LEN];
    char line[LINE_LEN];
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请点击空闲床位按钮：");
    while (p != NULL && count < 100) {
        if (strcmp(p->status, "空闲") == 0) {
            Ward *ward = findWardById(p->wardId);
            Department *dept = ward ? findDepartmentById(ward->deptId) : NULL;
            list[count] = p;
            snprintf(line, sizeof(line), "%d. %s  病房：%s  科室：%s\n",
                count + 1, p->id, ward ? ward->type : "未知", dept ? dept->name : "未知");
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        message(L"当前没有空闲床位。");
        return NULL;
    }
    if (!askChoice(L"选择空床", prompt, count, &choice)) return NULL;
    return list[choice - 1];
}

static void doPatientLogin(void)
{
    char phone[PHONE_LEN];
    char password[STATUS_LEN];
    AuthAccount *account;
    Patient *patient;
    FormField fields[] = {
        {L"手机号", L"", phone, sizeof(phone), FORM_PHONE, 0, 0},
        {L"登录密码", L"6位数字", password, sizeof(password), FORM_PAY_PASSWORD, 0, 0}
    };
    clearText(phone, sizeof(phone));
    clearText(password, sizeof(password));
    if (!runForm(L"患者登录", L"请输入注册时绑定的唯一手机号和6位登录密码。", fields, 2)) return;
    account = findAuthByLogin("patient", phone, password);
    if (account == NULL) {
        message(L"手机号或密码错误，或该手机号不是患者账号。");
        return;
    }
    patient = findPatientById(account->targetId);
    if (patient == NULL) {
        message(L"账号关联的患者信息不存在，请联系管理端核对。");
        return;
    }
    safeCopy(loggedPatientId, ID_LEN, patient->id);
    currentPage = PAGE_PATIENT;
}

static void doPatientRegister(void)
{
    char id[ID_LEN], name[NAME_LEN], gender[STATUS_LEN], phone[PHONE_LEN], card[NAME_LEN];
    char password[STATUS_LEN];
    char ageText[32] = "";
    int age, code;
    char out[RESULT_LEN];
    FormField fields[] = {
        {L"手机号", L"", phone, sizeof(phone), FORM_PHONE, 0, 0},
        {L"登录密码", L"6位数字", password, sizeof(password), FORM_PAY_PASSWORD, 0, 0},
        {L"患者姓名", L"", name, sizeof(name), FORM_NAME, 0, 0},
        {L"性别", L"", gender, sizeof(gender), FORM_GENDER, 0, 0},
        {L"年龄", L"", ageText, sizeof(ageText), FORM_INT, 0, 120},
        {L"身份证号", L"", card, sizeof(card), FORM_CARD, 0, 0}
    };
    clearText(phone, sizeof(phone));
    clearText(password, sizeof(password));
    clearText(name, sizeof(name));
    clearText(gender, sizeof(gender));
    clearText(card, sizeof(card));
    if (!runForm(L"患者注册", L"注册时一次性填写患者信息，后续患者端业务会自动关联该账号。", fields, 6)) return;
    parseIntStrict(ageText, &age);
    if (findAuthByPhone(phone) != NULL || findPatientByPhoneLocal(phone) != NULL) {
        message(L"该手机号已经被使用，请换一个唯一手机号。");
        return;
    }
    if (findPatientByCardId(card) != NULL) {
        message(L"身份证号已经存在，不能重复注册。");
        return;
    }
    makeNextPatientId(id, sizeof(id));
    code = addPatient(id, name, gender, age, phone, card, "门诊");
    if (code == OK) code = addAuthAccount("patient", phone, password, id);
    if (code == OK) saveAuthData("data\\account.txt");
    if (code == OK) safeCopy(loggedPatientId, ID_LEN, id);
    snprintf(out, sizeof(out),
        "患者注册%s\n账号手机号：%s\n患者编号：%s\n患者：%s\n后续患者端业务已自动关联该账号。\n",
        code == OK ? "成功" : "失败", phone, id, name);
    openResult(out);
}

static void doDoctorLogin(void)
{
    char phone[PHONE_LEN];
    char password[STATUS_LEN];
    AuthAccount *account;
    Doctor *doctor;
    FormField fields[] = {
        {L"手机号", L"", phone, sizeof(phone), FORM_PHONE, 0, 0},
        {L"登录密码", L"6位数字", password, sizeof(password), FORM_PAY_PASSWORD, 0, 0}
    };
    clearText(phone, sizeof(phone));
    clearText(password, sizeof(password));
    if (!runForm(L"医生登录", L"请输入医生账号绑定的唯一手机号和6位登录密码。", fields, 2)) return;
    account = findAuthByLogin("doctor", phone, password);
    if (account == NULL) {
        message(L"手机号或密码错误，或该手机号不是医生账号。");
        return;
    }
    doctor = findDoctorById(account->targetId);
    if (doctor == NULL) {
        message(L"账号关联的医生信息不存在，请联系管理端核对。");
        return;
    }
    safeCopy(loggedDoctorId, ID_LEN, doctor->id);
    currentPage = PAGE_MEDICAL;
}

static void doDoctorRegister(void)
{
    char phone[PHONE_LEN];
    char password[STATUS_LEN];
    Doctor *doctor;
    int code;
    char out[RESULT_LEN];
    FormField fields[] = {
        {L"手机号", L"", phone, sizeof(phone), FORM_PHONE, 0, 0},
        {L"登录密码", L"6位数字", password, sizeof(password), FORM_PAY_PASSWORD, 0, 0}
    };
    clearText(phone, sizeof(phone));
    clearText(password, sizeof(password));
    if (!runForm(L"医生注册", L"先填写手机号和密码，下一步从已有医生档案中选择要绑定的医生。", fields, 2)) return;
    if (findAuthByPhone(phone) != NULL) {
        message(L"该手机号已经被使用，请换一个唯一手机号。");
        return;
    }
    doctor = chooseDoctor();
    if (doctor == NULL) return;
    if (findAuthByTarget("doctor", doctor->id) != NULL) {
        message(L"该医生已经绑定过账号。");
        return;
    }
    code = addAuthAccount("doctor", phone, password, doctor->id);
    if (code == OK) saveAuthData("data\\account.txt");
    if (code == OK) safeCopy(loggedDoctorId, ID_LEN, doctor->id);
    {
        Department *dept = findDepartmentById(doctor->deptId);
        snprintf(out, sizeof(out),
            "医生注册%s\n账号手机号：%s\n医生：%s\n科室：%s\n后续医护端业务会自动使用该医生身份。\n",
            code == OK ? "成功" : "失败", phone, doctor->name, dept ? dept->name : "未知科室");
    }
    openResult(out);
}

static void doAdminLogin(void)
{
    char account[NAME_LEN];
    char password[STATUS_LEN];
    FormField fields[] = {
        {L"管理员账号", L"固定测试账号：admin", account, sizeof(account), FORM_TEXT, 0, 0},
        {L"管理员密码", L"固定测试密码：admin123", password, sizeof(password), FORM_TEXT, 0, 0}
    };

    clearText(account, sizeof(account));
    clearText(password, sizeof(password));
    if (!runForm(L"管理员登录", L"", fields, 2)) return;

    if (strcmp(account, "admin") != 0 || strcmp(password, "admin123") != 0) {
        writeAdminLog("管理员登录", account, "失败");
        message(L"管理员账号或密码错误。");
        return;
    }

    adminLoggedIn = 1;
    safeCopy(currentAdminName, sizeof(currentAdminName), "admin");
    writeAdminLog("管理员登录", "管理端", "成功");
    currentPage = PAGE_ADMIN;
}

static void doAddPatient(void)
{
    char id[ID_LEN], name[NAME_LEN], gender[STATUS_LEN], phone[PHONE_LEN], card[NAME_LEN], status[STATUS_LEN];
    char ageText[32] = "";
    int age, code;
    char out[RESULT_LEN];
    FormField fields[] = {
        {L"患者姓名", L"", name, sizeof(name), FORM_NAME, 0, 0},
        {L"性别", L"", gender, sizeof(gender), FORM_GENDER, 0, 0},
        {L"年龄", L"", ageText, sizeof(ageText), FORM_INT, 0, 120},
        {L"联系电话", L"", phone, sizeof(phone), FORM_PHONE, 0, 0},
        {L"身份证号", L"", card, sizeof(card), FORM_CARD, 0, 0},
        {L"患者状态", L"例如 门诊/住院，可为空", status, sizeof(status), FORM_TEXT, 0, 0}
    };
    clearText(name, sizeof(name));
    clearText(gender, sizeof(gender));
    clearText(phone, sizeof(phone));
    clearText(card, sizeof(card));
    clearText(status, sizeof(status));
    if (!runForm(L"新患者录入", L"点击每个输入框填写信息，全部填完后点确定。", fields, 6)) return;
    parseIntStrict(ageText, &age);
    if (findPatientByCardId(card) != NULL) {
        message(L"身份证号已经存在，不能重复添加。");
        return;
    }
    makeNextPatientId(id, sizeof(id));
    code = addPatient(id, name, gender, age, phone, card, status);
    snprintf(out, sizeof(out), "新增患者：%s\n系统生成编号：%s\n结果：%s\n", name, id, codeText(code));
    if (adminLoggedIn) writeAdminLog("新增患者", id, codeText(code));
    openResult(out);
}

static void doRegister(void)
{
    Patient *patient = choosePatient();
    Department *dept;
    Doctor *doctor;
    char visitDate[DATE_LEN];
    char today[DATE_LEN];
    char note[TEXT_LEN];
    char recId[ID_LEN];
    char payId[ID_LEN];
    char out[RESULT_LEN];
    char confirm[2048];
    char payPwd[16];
    char timestamp[TIME_LEN];
    double fee;
    int code;
    int prevSlot;
    int regStatus = 1;
    int visitOpt;
    const char *modeText;

    if (patient == NULL) return;
    while (1) {
        clearText(visitDate, sizeof(visitDate));
        getCurrentDate(today, sizeof(today));
        if (!askChoice(L"挂号类型", "1. 今天（直接挂号）\n2. 选择未来日期（预约）", 2, &visitOpt)) return;
        if (visitOpt == 1) {
            safeCopy(visitDate, sizeof(visitDate), today);
            regStatus = 1;
            break;
        }
        if (!askDate(L"预约日期", visitDate, sizeof(visitDate))) return;
        if (!checkDate(visitDate) || compareDate(visitDate, today) <= 0) {
            message(L"预约日期必须是晚于今天的有效日期。");
            continue;
        }
        regStatus = 0;
        break;
    }
    dept = chooseDepartment();
    if (dept == NULL) return;
    doctor = chooseDoctorForRegister(dept->id, visitDate);
    if (doctor == NULL) return;
    if (doctor->maxCount <= 0) {
        message(L"该医生号源已满，无法挂号。");
        return;
    }
    if (hasPatientDeptRegisterOnDate(patient->id, dept->id, visitDate)) {
        message(L"您已在该日期挂过该科室号，无需重复挂号。");
        return;
    }
    {
        FormField fields[] = {
            {L"挂号说明", L"可为空，不能含 | 或换行", note, sizeof(note), FORM_TEXT, 0, 0},
            {L"支付密码", L"", payPwd, sizeof(payPwd), FORM_PAY_PASSWORD, 0, 0}
        };
        clearText(note, sizeof(note));
        clearText(payPwd, sizeof(payPwd));
        if (!runForm(L"挂号信息填写", L"请填写挂号说明和六位支付密码。", fields, 2)) return;
    }
    fee = registerFeeByTitle(doctor->title);
    (void)payPwd;

    modeText = (regStatus == 1) ? "直接挂号（已挂号）" : "预约（就诊日当天来院候诊）";
    snprintf(confirm, sizeof(confirm),
        "请核对以下挂号信息，核对无误请点击「确定」进入缴费；点击「取消」可返回重新选择。\n\n"
        "患者：%s\n科室：%s\n医生：%s  （%s）\n出诊安排：%s\n就诊日期：%s\n业务类型：%s\n挂号费：%.2f 元\n说明：%s\n",
        patient->name, dept->name, doctor->name, doctor->title, doctor->workTime, visitDate,
        modeText, fee, note);
    if (MessageBox(GetHWnd(), toWide(confirm).c_str(), L"挂号确认单", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
        message(L"已取消挂号。");
        return;
    }

    getCurrentDate(today, sizeof(today));
    makeNextRecordId(recId, sizeof(recId));
    makeNextPaymentId(payId, sizeof(payId));
    prevSlot = doctor->maxCount;

    code = updateDoctor(doctor->id, doctor->title, doctor->workTime, prevSlot - 1);
    if (code != OK) {
        snprintf(out, sizeof(out), "扣减号源失败：%s\n", codeText(code));
        openResult(out);
        return;
    }
    code = addPayment(payId, patient->id, "挂号", recId, fee, "已缴费", today, "挂号费");
    if (code != OK) {
        updateDoctor(doctor->id, doctor->title, doctor->workTime, prevSlot);
        snprintf(out, sizeof(out), "生成缴费记录失败，号源已恢复。\n原因：%s\n", codeText(code));
        openResult(out);
        return;
    }
    code = addMedicalRecord(recId, patient->id, doctor->id, dept->id, "挂号", visitDate, fee, payId, note, regStatus);
    if (code != OK) {
        deletePaymentById(payId);
        updateDoctor(doctor->id, doctor->title, doctor->workTime, prevSlot);
        snprintf(out, sizeof(out), "挂号失败，缴费记录与号源已恢复。\n原因：%s\n", codeText(code));
        openResult(out);
        return;
    }
    getCurrentDateTime(timestamp, sizeof(timestamp));
    snprintf(out, sizeof(out),
        "挂号成功\n患者：%s\n科室：%s\n医生：%s\n就诊日期：%s\n挂号单号：%s\n缴费单号：%s\n挂号费：%.2f 元\n状态：%s\n医生号源：%d -> %d\n操作时间：%s\n",
        patient->name, dept->name, doctor->name, visitDate, recId, payId, fee,
        (regStatus == 1) ? "已挂号" : "预约",
        prevSlot, prevSlot - 1, timestamp);
    openResult(out);
}

static void doPatientProfile(void)
{
    Patient *patient = choosePatient();
    char out[RESULT_LEN];
    int pos;
    if (patient == NULL) return;
    snprintf(out, sizeof(out),
        "个人档案\n姓名：%s\n性别：%s\n年龄：%d\n电话：%s\n身份证：%s\n在院状态：%s\n",
        patient->name, patient->gender, patient->age, patient->phone, patient->cardId, patient->status);
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "【挂号记录】");
    pos = (int)strlen(out);
    listPatientMedicalPatientView(patient->id, "register", "", NULL, NULL, out + pos, RESULT_LEN - pos);
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "【就诊记录】（看诊、检查、入院、出院）");
    pos = (int)strlen(out);
    listPatientMedicalPatientView(patient->id, "visit", "", NULL, NULL, out + pos, RESULT_LEN - pos);
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "说明：费用汇总请使用「我的账单」；处方药品请使用「处方明细」；检查/药费/住院费请使用「待缴费」。");
    openResult(out);
}

static void doMyBills(void)
{
    Patient *patient = choosePatient();
    char out[RESULT_LEN];
    char sum[4096];
    char detail[RESULT_LEN];
    int pos;
    if (patient == NULL) return;
    paymentSummaryByPatient(patient->id, sum, sizeof(sum));
    listPaymentsByPatientPatientView(patient->id, NULL, NULL, detail, sizeof(detail));
    snprintf(out, sizeof(out), "我的账单\n患者：%s\n\n【费用汇总】\n", patient->name);
    pos = (int)strlen(out);
    snprintf(out + pos, RESULT_LEN - pos, "%s\n【明细】\n%s", sum, detail);
    openResult(out);
}

static void appendPrescriptionAdviceForPatient(const Prescription *rx, char *out, int outSize)
{
    PrescriptionItem *item;
    Medicine *med;
    char adviceCopy[TEXT_LEN];
    char *token;
    char line[LINE_LEN];
    int itemIdx = 0;
    int n = 0;
    if (rx == NULL) return;
    appendLine(out, outSize, "【用药医嘱】");
    if (isBlank(rx->advice)) {
        appendLine(out, outSize, "（暂无医嘱，请遵医嘱用药）");
        return;
    }
    item = prescriptionItemHead;
    while (item != NULL) {
        if (strcmp(item->prescriptionId, rx->id) == 0) {
            med = findMedicineById(item->medicineId);
            if (med != NULL) {
                safeCopy(adviceCopy, TEXT_LEN, rx->advice);
                token = strtok(adviceCopy, "|");
                for (int j = 0; j < itemIdx; j++) {
                    token = strtok(NULL, "|");
                    if (token == NULL) break;
                }
                if (token != NULL) {
                    snprintf(line, sizeof(line), "%s：%s\n", med->name, token);
                } else {
                    snprintf(line, sizeof(line), "%s：%s\n", med->name, rx->advice);
                }
                appendText(out, outSize, line);
                n++;
            }
            itemIdx++;
        }
        item = item->next;
    }
    if (n == 0) {
        snprintf(line, sizeof(line), "%s\n", rx->advice);
        appendText(out, outSize, line);
    }
}

static const char *prescriptionStatusHint(const char *status)
{
    if (status == NULL) return "";
    if (strcmp(status, "未缴费") == 0) {
        return "请前往「待缴费」缴纳药费；缴清后医护方可发药。";
    }
    if (strcmp(status, "已缴费") == 0) {
        return "药费已缴清，请等候医护发药或到药房窗口取药。";
    }
    if (strcmp(status, "已发药") == 0) {
        return "药品已发放，请严格按医嘱服药，如有不适请及时复诊。";
    }
    return "";
}

static void doPrescriptionDetail(void)
{
    Patient *patient = choosePatient();
    Prescription *list[50];
    Prescription *rx;
    Payment *pay;
    Doctor *doctor;
    Department *dept;
    char prompt[RESULT_LEN];
    char detail[RESULT_LEN];
    char out[RESULT_LEN];
    char line[LINE_LEN];
    char adviceBlock[RESULT_LEN];
    int n, choice, i, pos;
    if (patient == NULL) return;
    n = collectPrescriptionsByPatient(patient->id, list, 50);
    if (n == 0) {
        message(L"该患者还没有处方记录。");
        return;
    }
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请选择要查看的处方（按开具日期列出）：");
    for (i = 0; i < n; i++) {
        doctor = findDoctorById(list[i]->doctorId);
        dept = findDepartmentById(list[i]->deptId);
        snprintf(line, sizeof(line), "%d. %s  %s  %s  %d种药  %.2f元  %s\n",
            i + 1, list[i]->id, list[i]->date,
            dept ? dept->name : "未知科室",
            list[i]->itemCount, list[i]->totalFee, list[i]->status);
        appendText(prompt, sizeof(prompt), line);
        if (doctor != NULL) {
            snprintf(line, sizeof(line), "    开方医生：%s\n", doctor->name);
            appendText(prompt, sizeof(prompt), line);
        }
    }
    if (!askChoice(L"处方明细", prompt, n, &choice)) return;
    rx = list[choice - 1];
    doctor = findDoctorById(rx->doctorId);
    dept = findDepartmentById(rx->deptId);
    pay = findPaymentBySource("处方", rx->id);

    clearText(out, sizeof(out));
    snprintf(line, sizeof(line), "【处方详情】\n患者：%s\n", patient->name);
    appendText(out, sizeof(out), line);
    snprintf(line, sizeof(line),
        "处方编号：%s\n开具日期：%s\n接诊编号：%s\n开方医生：%s\n开方科室：%s\n处方状态：%s\n",
        rx->id, rx->date,
        isBlank(rx->medicalRecordId) ? "（无）" : rx->medicalRecordId,
        doctor ? doctor->name : "（无）",
        dept ? dept->name : "未知科室",
        rx->status);
    appendText(out, sizeof(out), line);
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "【缴费信息】");
    if (pay != NULL) {
        snprintf(line, sizeof(line),
            "缴费编号：%s\n缴费状态：%s\n应缴金额：%.2f元\n缴费日期：%s\n",
            pay->id, pay->status, pay->amount, pay->date);
        appendText(out, sizeof(out), line);
    } else {
        appendLine(out, sizeof(out), "暂无关联缴费单（请联系开方科室核对）。");
    }
    appendLine(out, sizeof(out), "");
    clearText(adviceBlock, sizeof(adviceBlock));
    appendPrescriptionAdviceForPatient(rx, adviceBlock, sizeof(adviceBlock));
    appendText(out, sizeof(out), adviceBlock);
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "【药品明细】");
    pos = (int)strlen(out);
    clearText(detail, sizeof(detail));
    listPrescriptionItemsForPatient(rx->id, detail, sizeof(detail));
    snprintf(out + pos, RESULT_LEN - pos, "%s", detail);
    pos = (int)strlen(out);
    snprintf(out + pos, RESULT_LEN - pos, "\n合计：%.2f元\n", rx->totalFee);
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "【温馨提示】");
    appendLine(out, sizeof(out), prescriptionStatusHint(rx->status));
    if (pay != NULL && strcmp(pay->status, "未缴费") == 0) {
        appendLine(out, sizeof(out), "您可在本系统「待缴费」中完成支付（需输入六位支付密码）。");
    }
    openResult(out);
}

static void doPatientPayFees(void)
{
    Patient *patient = choosePatient();
    Payment *list[50];
    int count, choice, code, codeExtra, i;
    char payDate[DATE_LEN];
    char payPwd[16];
    char prompt[RESULT_LEN], line[LINE_LEN], out[RESULT_LEN], timestamp[TIME_LEN];
    Payment *pay;
    Bed *bed;
    Ward *ward;
    Department *dept;
    char recId[ID_LEN];
    char pDept[ID_LEN], pDoc[ID_LEN], pDate[DATE_LEN], pIds[TEXT_LEN], pNames[TEXT_LEN];
    if (patient == NULL) return;
    count = collectUnpaidPaymentsByPatient(patient->id, list, 50);
    if (count == 0) {
        message(L"当前没有待缴费的账单。");
        return;
    }
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请先确认账单信息，再输入六位支付密码完成缴费。");
    appendLine(prompt, sizeof(prompt),
        "温馨提示：检查费、药费、住院费均须先缴费；缴费后检查记录将自动登记，住院费缴清后将自动出院。");
    appendLine(prompt, sizeof(prompt), "请选择要缴费的账单（未缴费）：");
    for (i = 0; i < count; i++) {
        if (strcmp(list[i]->sourceType, "检查") == 0 &&
            parseCheckBundleNote(list[i]->note, pDept, sizeof(pDept), pDoc, sizeof(pDoc),
                pDate, sizeof(pDate), pIds, sizeof(pIds), pNames, sizeof(pNames))) {
            snprintf(line, sizeof(line), "%d. 检查 %.2f元\n项目：%s\n",
                i + 1, list[i]->amount, pNames);
        } else {
            snprintf(line, sizeof(line), "%d. %s %.2f元\n说明：%s\n",
                i + 1, list[i]->sourceType, list[i]->amount, list[i]->note);
        }
        appendText(prompt, sizeof(prompt), line);
    }
    if (!askChoice(L"待缴费", prompt, count, &choice)) return;
    if (!askPayPassword(payPwd, sizeof(payPwd))) return;
    pay = list[choice - 1];
    (void)payPwd;
    getCurrentDate(payDate, sizeof(payDate));
    code = payPayment(pay->id, payDate);
    codeExtra = OK;
    if (code == OK && strcmp(pay->sourceType, "处方") == 0) {
        codeExtra = setPrescriptionPaid(pay->sourceId);
    } else if (code == OK && strcmp(pay->sourceType, "检查") == 0) {
        if (findRecordById(pay->sourceId) == NULL) {
            if (parseCheckBundleNote(pay->note, pDept, sizeof(pDept), pDoc, sizeof(pDoc),
                pDate, sizeof(pDate), pIds, sizeof(pIds), pNames, sizeof(pNames))) {
                codeExtra = addMedicalRecord(pay->sourceId, patient->id, pDoc, pDept,
                    "检查", pDate, pay->amount, pay->id, pIds, 1);
            } else {
                codeExtra = ERR_INPUT;
            }
        }
    } else if (code == OK && strcmp(pay->sourceType, "住院") == 0 &&
        strcmp(pay->note, "出院结算") == 0) {
        bed = findBedByPatient(patient->id);
        if (bed == NULL || strcmp(bed->id, pay->sourceId) != 0) {
            codeExtra = ERR_NOT_FOUND;
        } else {
            ward = findWardById(bed->wardId);
            dept = ward ? findDepartmentById(ward->deptId) : NULL;
            if (ward == NULL || dept == NULL) {
                codeExtra = ERR_NOT_FOUND;
            } else {
                makeNextRecordId(recId, sizeof(recId));
                codeExtra = addMedicalRecord(recId, patient->id, "", dept->id,
                    "出院", payDate, pay->amount, pay->id, "出院结算并释放床位", 1);
                if (codeExtra == OK) codeExtra = releasePatientBed(patient->id);
            }
        }
    }
    getCurrentDateTime(timestamp, sizeof(timestamp));
    if (code != OK) {
        snprintf(out, sizeof(out), "待缴费办理失败\n患者：%s\n原因：%s\n操作时间：%s\n",
            patient->name, codeText(code), timestamp);
    } else {
        snprintf(out, sizeof(out), "待缴费办理成功\n患者：%s\n缴费编号：%s\n同步结果：%s\n操作时间：%s\n",
            patient->name, pay->id, codeText(codeExtra), timestamp);
    }
    openResult(out);
}

static void doVisit(void)
{
    Patient *patient = choosePatient();
    Department *dept;
    Doctor *doctor;
    MedicalRecord *regRecord;
    char note[TEXT_LEN], recId[ID_LEN], out[RESULT_LEN], timestamp[TIME_LEN];
    char docId[ID_LEN];
    char complaint[TEXT_LEN];
    char diagnosis[TEXT_LEN];
    char confirmText[512];
    int code;
    if (patient == NULL) return;
    regRecord = ensureActiveRegister(patient);
    if (regRecord == NULL) return;
    dept = findDepartmentById(regRecord->deptId);
    doctor = findDoctorById(regRecord->doctorId);
    {
        Doctor *loginDoc = currentDoctor();
        if (loginDoc != NULL && strcmp(regRecord->doctorId, loginDoc->id) != 0) {
            message(L"该患者不是当前登录医生的挂号患者，请选择本人接诊患者。");
            return;
        }
        if (loginDoc != NULL) doctor = loginDoc;
    }
    safeCopy(docId, ID_LEN, regRecord->doctorId);
    // 答辩注意：接诊前先展示患者完整信息供确认，减少演示时信息遗忘（难度：简单）
    snprintf(confirmText, sizeof(confirmText),
        "患者姓名：%s\n年龄：%d岁\n身份证号：%s\n手机号：%s\n所挂科室：%s\n接诊医生：%s\n\n请确认患者信息无误后开始接诊。",
        patient->name, patient->age,
        patient->cardId, patient->phone,
        dept ? dept->name : "未知科室",
        doctor ? doctor->name : "(无)");
    if (MessageBox(GetHWnd(), toWide(confirmText).c_str(), L"患者信息确认", MB_OKCANCEL) != IDOK) {
        message(L"已取消接诊。");
        return;
    }
    {
        FormField fields[] = {
            {L"患者主诉", L"", complaint, sizeof(complaint), FORM_MEMO, 0, 0},
            {L"诊断建议", L"", diagnosis, sizeof(diagnosis), FORM_MEMO, 0, 0}
        };
        clearText(complaint, sizeof(complaint));
        clearText(diagnosis, sizeof(diagnosis));
        if (!runForm(L"接诊记录", L"点击输入框填写主诉和诊断处理建议。", fields, 2)) return;
    }
    snprintf(note, sizeof(note), "主诉：%s；诊断：%s", complaint, diagnosis);
    makeNextRecordId(recId, sizeof(recId));
    code = addMedicalRecord(recId, patient->id, docId, regRecord->deptId, "看诊", regRecord->date, 0, regRecord->id, note, 1);
    if (code != OK) {
        getCurrentDateTime(timestamp, sizeof(timestamp));
        snprintf(out, sizeof(out), "看诊记录添加失败\n患者：%s\n原因：%s\n操作时间：%s\n",
            patient->name, codeText(code), timestamp);
        openResult(out);
        return;
    }
    // 答辩注意：接诊完成后自动将患者状态标记为已接诊，并检查返回值（难度：简单）
    {
        int statusCode = updatePatientStatus(patient->id, "已接诊");
        if (statusCode != OK) {
            getCurrentDateTime(timestamp, sizeof(timestamp));
            snprintf(out, sizeof(out), "看诊记录已添加，但患者状态更新失败：%s\n记录编号：%s\n患者：%s\n科室：%s\n日期：%s\n主诉：%s\n诊断：%s\n操作时间：%s\n",
                codeText(statusCode), recId, patient->name,
                dept ? dept->name : "未知科室", regRecord->date,
                complaint, diagnosis, timestamp);
            openResult(out);
            return;
        }
    }
    getCurrentDateTime(timestamp, sizeof(timestamp));
    snprintf(out, sizeof(out), "看诊记录已添加\n记录编号：%s\n患者：%s\n医生：%s\n科室：%s\n日期：%s\n主诉：%s\n诊断：%s\n患者状态：已就诊\n结果：%s\n操作时间：%s\n",
        recId, patient->name, doctor ? doctor->name : "(无)",
        dept ? dept->name : "未知科室", regRecord->date,
        complaint, diagnosis, codeText(code), timestamp);
    openResult(out);
}

static void doCheck(void)
{
    Patient *patient = choosePatient();
    Department *dept;
    Doctor *doctor;
    MedicalRecord *regRecord;
    CheckItem *selList[20];
    int selectedIndices[20];
    char itemNote[TEXT_LEN];
    char recId[ID_LEN], payId[ID_LEN], out[RESULT_LEN], timestamp[TIME_LEN];
    char docId[ID_LEN];
    char info[RESULT_LEN];
    char detailText[RESULT_LEN];
    char itemNames[TEXT_LEN];
    char bundleNote[TEXT_LEN];
    double totalFee = 0;
    int selCount, i;
    int code;
    if (patient == NULL) return;
    regRecord = ensureActiveRegister(patient);
    if (regRecord == NULL) return;
    dept = findDepartmentById(regRecord->deptId);
    doctor = findDoctorById(regRecord->doctorId);
    {
        Doctor *loginDoc = currentDoctor();
        if (loginDoc != NULL && strcmp(regRecord->doctorId, loginDoc->id) != 0) {
            message(L"该患者不是当前登录医生的挂号患者，请选择本人开单患者。");
            return;
        }
        if (loginDoc != NULL) doctor = loginDoc;
    }
    safeCopy(docId, ID_LEN, regRecord->doctorId);
    // 答辩注意：从预设列表中多选检查项目，按挂号科室过滤（难度：中等）
    selCount = chooseCheckItemsByDepartment(regRecord->deptId, selectedIndices, 20);
    if (selCount == 0) {
        message(L"未选择任何检查项目，已取消。");
        return;
    }
    totalFee = 0;
    clearText(itemNote, sizeof(itemNote));
    clearText(detailText, sizeof(detailText));
    clearText(itemNames, sizeof(itemNames));
    appendLine(detailText, sizeof(detailText), "【检查项目明细】");
    for (i = 0; i < selCount; i++) {
        selList[i] = &internalCheckItems[selectedIndices[i]];
        totalFee += selList[i]->price;
        snprintf(info, sizeof(info), "%s(%.2f元)\n", selList[i]->name, selList[i]->price);
        appendText(detailText, sizeof(detailText), info);
        if (i > 0) safeCopy(itemNote + strlen(itemNote), TEXT_LEN - (int)strlen(itemNote), ",");
        safeCopy(itemNote + strlen(itemNote), TEXT_LEN - (int)strlen(itemNote), selList[i]->id);
        if (i > 0) strncat(itemNames, "、", TEXT_LEN - (int)strlen(itemNames) - 1);
        strncat(itemNames, selList[i]->name, TEXT_LEN - (int)strlen(itemNames) - 1);
    }
    // 用第一个选中检查项目对应的科室作为实际科室显示
    dept = findDepartmentById(selList[0]->deptId);
    snprintf(info, sizeof(info), "\n合计费用：%.2f元", totalFee);
    appendText(detailText, sizeof(detailText), info);
    if (!confirmCheckItems(detailText)) {
        message(L"已取消检查开单。");
        return;
    }
    makeNextRecordId(recId, sizeof(recId));
    makeNextPaymentId(payId, sizeof(payId));
    buildCheckBundleNote(regRecord->deptId, docId, regRecord->date, itemNote, itemNames,
        bundleNote, sizeof(bundleNote));
    code = addPayment(payId, patient->id, "检查", recId, totalFee, "未缴费", regRecord->date, bundleNote);
    getCurrentDateTime(timestamp, sizeof(timestamp));
    snprintf(out, sizeof(out), "检查单已开立（待缴费）\n患者：%s\n科室：%s\n检查项目：%s\n合计费用：%.2f元\n预留记录编号：%s\n缴费编号：%s\n请前往「待缴费」缴纳检查费，缴费后检查记录将自动登记。\n结果：%s\n操作时间：%s\n",
        patient->name, dept ? dept->name : "未知科室",
        itemNames, totalFee, recId, payId, codeText(code), timestamp);
    openResult(out);
}

static void drawPrescriptionOrderForm(const char *deptId, Medicine *meds[],
    char qtyText[][16], char adviceList[][TEXT_LEN], int slotCount)
{
    int i;
    Department *dept = findDepartmentById(deptId);
    Button buttons[] = {
        {360, 500, 500, 550, L"确定", 201},
        {560, 500, 700, 550, L"取消", 202}
    };
    drawTop(L"开处方");
    {
        wchar_t info[160];
        useClearFont(20, FW_SEMIBOLD);
        settextcolor(RGB(25, 35, 50));
        swprintf(info, 160, L"当前科室：%s    每行选择药品、填写数量和医嘱；不用的行留空。",
            dept ? toWide(dept->name).c_str() : L"未知科室");
        outtextxy(70, 145, info);
    }
    useClearFont(18, FW_SEMIBOLD);
    settextcolor(RGB(20, 36, 64));
    outtextxy(80, 188, L"序号");
    outtextxy(150, 188, L"药品");
    outtextxy(465, 188, L"数量");
    outtextxy(610, 188, L"用药医嘱");
    for (i = 0; i < slotCount; i++) {
        int y = 220 + i * 52;
        RECT noRect = {75, y, 125, y + 40};
        RECT medRect = {145, y, 445, y + 40};
        RECT qtyRect = {465, y, 585, y + 40};
        RECT adviceRect = {605, y, 965, y + 40};
        wchar_t noText[16];
        swprintf(noText, 16, L"%d", i + 1);
        useClearFont(18, FW_SEMIBOLD);
        settextcolor(RGB(20, 36, 64));
        drawtext(noText, &noRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        setfillcolor(WHITE);
        setlinecolor(RGB(88, 110, 140));
        setlinestyle(PS_SOLID, 2);
        solidrectangle(medRect.left, medRect.top, medRect.right, medRect.bottom);
        rectangle(medRect.left, medRect.top, medRect.right, medRect.bottom);
        solidrectangle(qtyRect.left, qtyRect.top, qtyRect.right, qtyRect.bottom);
        rectangle(qtyRect.left, qtyRect.top, qtyRect.right, qtyRect.bottom);
        solidrectangle(adviceRect.left, adviceRect.top, adviceRect.right, adviceRect.bottom);
        rectangle(adviceRect.left, adviceRect.top, adviceRect.right, adviceRect.bottom);
        useClearFont(18, FW_SEMIBOLD);
        settextcolor(meds[i] == NULL ? RGB(82, 95, 115) : RGB(15, 23, 42));
        {
            RECT textRect = {medRect.left + 10, medRect.top, medRect.right - 10, medRect.bottom};
            drawtext(meds[i] == NULL ? L"点击选择药品" : toWide(meds[i]->name).c_str(),
                &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        settextcolor(qtyText[i][0] == '\0' ? RGB(82, 95, 115) : RGB(15, 23, 42));
        {
            RECT textRect = {qtyRect.left + 10, qtyRect.top, qtyRect.right - 10, qtyRect.bottom};
            drawtext(qtyText[i][0] == '\0' ? L"数量" : toWide(qtyText[i]).c_str(),
                &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        settextcolor(adviceList[i][0] == '\0' ? RGB(82, 95, 115) : RGB(15, 23, 42));
        {
            RECT textRect = {adviceRect.left + 10, adviceRect.top, adviceRect.right - 10, adviceRect.bottom};
            drawtext(adviceList[i][0] == '\0' ? L"可不填" : toWide(adviceList[i]).c_str(),
                &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }
    drawButtons(buttons, 2);
    setlinestyle(PS_SOLID, 1);
}

static int runPrescriptionOrderForm(const char *deptId, Medicine *meds[],
    char qtyText[][16], char adviceList[][TEXT_LEN], int counts[], int slotCount, int *itemCount)
{
    Button buttons[] = {
        {360, 500, 500, 550, L"", 201},
        {560, 500, 700, 550, L"", 202}
    };
    int i;
    for (i = 0; i < slotCount; i++) {
        meds[i] = NULL;
        clearText(qtyText[i], 16);
        clearText(adviceList[i], TEXT_LEN);
        counts[i] = 0;
    }
    drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
    while (1) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int action = hit(buttons, 2, msg.x, msg.y);
                int slot = -1;
                int area = 0;
                for (i = 0; i < slotCount; i++) {
                    int y = 220 + i * 52;
                    if (msg.y >= y && msg.y <= y + 40) {
                        if (msg.x >= 145 && msg.x <= 445) { slot = i; area = 1; break; }
                        if (msg.x >= 465 && msg.x <= 585) { slot = i; area = 2; break; }
                        if (msg.x >= 605 && msg.x <= 965) { slot = i; area = 3; break; }
                    }
                }
                if (slot >= 0 && area == 1) {
                    Medicine *m = chooseMedicineByDepartment(deptId);
                    if (m != NULL) meds[slot] = m;
                    drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                } else if (slot >= 0 && area == 2) {
                    wchar_t buf[64];
                    wcsncpy_s(buf, 64, toWide(qtyText[slot]).c_str(), _TRUNCATE);
                    if (InputBox(buf, 64, L"1-9999 的整数", L"药品数量",
                            toWide(qtyText[slot]).c_str(), 0, 0, false)) {
                        safeCopy(qtyText[slot], 16, toUtf8(buf).c_str());
                    }
                    drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                } else if (slot >= 0 && area == 3) {
                    wchar_t buf[512];
                    wcsncpy_s(buf, 512, toWide(adviceList[slot]).c_str(), _TRUNCATE);
                    if (InputBox(buf, 512, L"可不填，不能含 | 或换行", L"用药医嘱",
                            toWide(adviceList[slot]).c_str(), 0, 0, false)) {
                        safeCopy(adviceList[slot], TEXT_LEN, toUtf8(buf).c_str());
                    }
                    drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                } else if (action == 201) {
                    int count = 0;
                    for (i = 0; i < slotCount; i++) {
                        int value;
                        int j;
                        if (meds[i] == NULL && isBlank(qtyText[i]) && isBlank(adviceList[i])) continue;
                        if (meds[i] == NULL) {
                            message(L"已填写数量或医嘱的行必须先选择药品。");
                            drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                            break;
                        }
                        if (!parseIntStrict(qtyText[i], &value) || value < 1 || value > 9999) {
                            message(L"药品数量必须是 1-9999 的整数。");
                            drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                            break;
                        }
                        if (hasBadChar(adviceList[i])) {
                            message(L"用药医嘱不能包含 | 或换行。");
                            drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                            break;
                        }
                        for (j = 0; j < i; j++) {
                            if (meds[j] != NULL && strcmp(meds[j]->id, meds[i]->id) == 0) {
                                message(L"同一张处方中药品不能重复选择。");
                                drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                                break;
                            }
                        }
                        if (j < i) break;
                        counts[i] = value;
                        count++;
                    }
                    if (i < slotCount) continue;
                    if (count == 0) {
                        message(L"请至少填写一行药品和数量。");
                        drawPrescriptionOrderForm(deptId, meds, qtyText, adviceList, slotCount);
                        continue;
                    }
                    *itemCount = count;
                    return 1;
                } else if (action == 202) {
                    return 0;
                }
            }
        }
        Sleep(10);
    }
}

static void doPrescription(void)
{
    Patient *patient = choosePatient();
    Department *dept;
    Doctor *doctor;
    MedicalRecord *regRecord;
    Medicine *meds[5];
    char qtyText[5][16];
    int counts[5];
    char rxId[ID_LEN], payId[ID_LEN], out[RESULT_LEN], itemText[RESULT_LEN], timestamp[TIME_LEN];
    int itemCount, i, code;
    Prescription *rxPtr = NULL;
    double totalFee = 0;
    int finalItemCount = 0;
    char adviceList[5][TEXT_LEN];
    char medicineNameList[5][NAME_LEN];
    char combinedAdvice[TEXT_LEN];
    char pairedAdviceText[TEXT_LEN];
    if (patient == NULL) return;
    regRecord = ensureCurrentVisit(patient);
    if (regRecord == NULL) return;
    dept = findDepartmentById(regRecord->deptId);
    doctor = findDoctorById(regRecord->doctorId);
    {
        Doctor *loginDoc = currentDoctor();
        if (loginDoc != NULL && strcmp(regRecord->doctorId, loginDoc->id) != 0) {
            message(L"该患者不是当前登录医生接诊的患者，不能开处方。");
            return;
        }
        if (loginDoc != NULL) doctor = loginDoc;
    }
    if (!runPrescriptionOrderForm(regRecord->deptId, meds, qtyText, adviceList, counts, 5, &itemCount)) return;
    makeNextPrescriptionId(rxId, sizeof(rxId));
    // 答辩注意：处方初始不写advice，等所有药品和对应医嘱录入完成后再汇入（难度：中等）
    code = addPrescription(rxId, patient->id, doctor ? doctor->id : "", regRecord->deptId,
        regRecord->date, regRecord->id, "");
    if (code != OK) {
        message(L"创建处方失败，请重试。");
        return;
    }
    // 答辩注意：如果药品库存检查失败，要删除已添加的处方和处方项，避免脏数据（难度：较难）
    for (i = 0; i < 5; i++) {
        if (meds[i] == NULL) continue;
        if (meds[i]->stock < counts[i]) {
            // 答辩注意：检查库存不足时，也要删除已添加的处方和处方项（难度：中等）
            char warn[256];
            snprintf(warn, sizeof(warn), "药品「%s」库存不足（当前库存：%d，需要：%d），已取消整张处方。",
                meds[i]->name, meds[i]->stock, counts[i]);
            deletePrescriptionItemsByRxId(rxId);
            deletePrescriptionById(rxId);
            message(toWide(warn).c_str());
            return;
        }
        code = addPrescriptionItem(rxId, meds[i]->id, counts[i]);
        if (code != OK) {
            deletePrescriptionItemsByRxId(rxId);
            deletePrescriptionById(rxId);
            message(L"添加药品失败。");
            return;
        }
        // 答辩注意：记下药品名，录入时同时保存药品名和对应医嘱，方便输出时一一配对（难度：简单）
        safeCopy(medicineNameList[finalItemCount], NAME_LEN, meds[i]->name);
        if (isBlank(adviceList[i])) safeCopy(adviceList[i], TEXT_LEN, "按医嘱使用");
        if (finalItemCount != i) safeCopy(adviceList[finalItemCount], TEXT_LEN, adviceList[i]);
        finalItemCount++;
    }
    // 答辩注意：将药品名和对应医嘱配对输出，每种药单独一行，格式为"药品名：医嘱"（难度：简单）
    pairedAdviceText[0] = '\0';
    for (i = 0; i < finalItemCount; i++) {
        char line[LINE_LEN];
        snprintf(line, sizeof(line), "%s：%s\n", medicineNameList[i], adviceList[i]);
        strncat(pairedAdviceText, line, TEXT_LEN - (int)strlen(pairedAdviceText) - 1);
    }
    // 获取处方信息前先检查是否创建成功
    rxPtr = findPrescriptionById(rxId);
    if (rxPtr == NULL || rxPtr->itemCount == 0) {
        if (rxPtr != NULL) {
            deletePrescriptionItemsByRxId(rxId);
            deletePrescriptionById(rxId);
        }
        message(L"处方为空或添加失败。");
        return;
    }
    // 答辩注意：处方医嘱汇总后写入，方便后续追溯（难度：简单）
    combinedAdvice[0] = '\0';
    for (i = 0; i < finalItemCount; i++) {
        if (i > 0) strncat(combinedAdvice, "|", TEXT_LEN - (int)strlen(combinedAdvice) - 1);
        strncat(combinedAdvice, adviceList[i], TEXT_LEN - (int)strlen(combinedAdvice) - 1);
    }
    safeCopy(rxPtr->advice, TEXT_LEN, combinedAdvice);
    totalFee = rxPtr->totalFee;
    makeNextPaymentId(payId, sizeof(payId));
    code = addPayment(payId, patient->id, "处方", rxId, totalFee, "未缴费", regRecord->date, "处方药费");
    if (code != OK) {
        deletePrescriptionItemsByRxId(rxId);
        deletePrescriptionById(rxId);
        getCurrentDateTime(timestamp, sizeof(timestamp));
        snprintf(out, sizeof(out), "处方创建成功但生成账单失败\n患者：%s\n处方编号：%s\n用药医嘱：\n%s失败原因：%s\n操作时间：%s\n",
            patient->name, rxId, pairedAdviceText + 0, codeText(code), timestamp);
        openResult(out);
        return;
    }
    listPrescriptionItems(rxId, itemText, sizeof(itemText));
    getCurrentDateTime(timestamp, sizeof(timestamp));
    snprintf(out, sizeof(out), "多药处方已生成（待缴费）\n患者：%s\n处方编号：%s\n接诊编号：%s\n用药医嘱：\n%s\n缴费编号：%s\n总金额：%.2f\n请前往「待缴费」缴纳药费后再发药。\n结果：%s\n操作时间：%s\n\n%s",
        patient->name, rxId, regRecord->id, pairedAdviceText + 0, payId, totalFee, codeText(code), timestamp, itemText);
    openResult(out);
}

static void doSendPrescription(void)
{
    Patient *patient = choosePatient();
    Prescription *rx;
    Prescription *list[50];
    int count = 0, choice, code;
    char prompt[RESULT_LEN], line[LINE_LEN], out[RESULT_LEN], timestamp[TIME_LEN];
    Doctor *loginDoc = currentDoctor();
    if (patient == NULL) return;
    rx = prescriptionHead;
    clearText(prompt, sizeof(prompt));
    appendLine(prompt, sizeof(prompt), "请选择已缴费未发药处方：");
    // 答辩注意：发药列表显示患者姓名，方便护士确认身份（难度：简单）
    {
        char header[LINE_LEN];
        snprintf(header, sizeof(header), "患者：%s\n", patient->name);
        appendText(prompt, sizeof(prompt), header);
    }
    while (rx != NULL && count < 50) {
        if (strcmp(rx->patientId, patient->id) == 0 && strcmp(rx->status, "已缴费") == 0 &&
            (loginDoc == NULL || strcmp(rx->doctorId, loginDoc->id) == 0 || strcmp(rx->deptId, loginDoc->deptId) == 0)) {
            list[count] = rx;
            // 答辩注意：发药列表展示患者姓名，方便护士核对身份（难度：简单）
            snprintf(line, sizeof(line), "%d. %s 患者：%s 日期：%s 金额：%.2f\n", count + 1, rx->id, patient->name, rx->date, rx->totalFee);
            appendText(prompt, sizeof(prompt), line);
            count++;
        }
        rx = rx->next;
    }
    if (count == 0) {
        message(L"该患者没有已缴费未发药的处方。请先完成处方缴费。");
        return;
    }
    if (!askChoice(L"选择处方", prompt, count, &choice)) return;
    // 发药前先检查所有药品库存，如果库存不足给出详细提示
    {
        PrescriptionItem *item;
        Medicine *med;
        char stockInfo[RESULT_LEN];
        int hasLowStock = 0;
        clearText(stockInfo, sizeof(stockInfo));
        item = prescriptionItemHead;
        while (item != NULL) {
            if (strcmp(item->prescriptionId, list[choice - 1]->id) == 0) {
                med = findMedicineById(item->medicineId);
                if (med != NULL && med->stock < item->count) {
                    // 答辩注意：发药前详细检查库存，给用户明确提示（难度：中等）
                    char itemLine[LINE_LEN];
                    snprintf(itemLine, sizeof(itemLine), "「%s」需要%d，当前库存%d\n",
                        med->name, item->count, med->stock);
                    appendText(stockInfo, sizeof(stockInfo), itemLine);
                    hasLowStock = 1;
                }
            }
            item = item->next;
        }
        if (hasLowStock) {
            message(L"处方中有药品库存不足，请先在管理端补充库存。");
            return;
        }
    }
    code = sendPrescription(list[choice - 1]->id);
    // 答辩注意：发药后检查返回值，给用户明确提示（难度：简单）
    getCurrentDateTime(timestamp, sizeof(timestamp));
    if (code != OK) {
        snprintf(out, sizeof(out), "处方发药失败\n患者：%s\n处方编号：%s\n接诊编号：%s\n原因：%s\n操作时间：%s\n",
            patient->name, list[choice - 1]->id,
            list[choice - 1]->medicalRecordId, codeText(code), timestamp);
        openResult(out);
        return;
    }
    // 答辩注意：发药成功后写医疗记录，便于追溯和统计（难度：简单）
    {
        char recId2[ID_LEN];
        makeNextRecordId(recId2, sizeof(recId2));
        Prescription *selRx = list[choice - 1];
        addMedicalRecord(recId2, patient->id, "", selRx->deptId,
            "发药", selRx->date, selRx->totalFee, selRx->id, "处方发药完成", 1);
    }
    // 答辩注意：将每种药品与医嘱一一对应展示，避免混杂和重复（难度：简单）
    {
        PrescriptionItem *item;
        Medicine *med;
        char medAdvice[RESULT_LEN];
        char adviceCopy[TEXT_LEN];
        char *token;
        int itemIdx = 0;
        clearText(medAdvice, sizeof(medAdvice));
        item = prescriptionItemHead;
        while (item != NULL) {
            if (strcmp(item->prescriptionId, list[choice - 1]->id) == 0) {
                med = findMedicineById(item->medicineId);
                if (med != NULL) {
                    char line[LINE_LEN];
                    safeCopy(adviceCopy, TEXT_LEN, list[choice - 1]->advice);
                    token = strtok(adviceCopy, "|");
                    for (int j = 0; j < itemIdx; j++) {
                        token = strtok(NULL, "|");
                        if (token == NULL) break;
                    }
                    if (token != NULL) {
                        snprintf(line, sizeof(line), "%s：%s\n", med->name, token);
                    } else {
                        snprintf(line, sizeof(line), "%s：%s\n", med->name, list[choice - 1]->advice);
                    }
                    appendText(medAdvice, sizeof(medAdvice), line);
                    itemIdx++;
                }
            }
            item = item->next;
        }
        snprintf(out, sizeof(out), "处方发药成功\n患者：%s\n处方编号：%s\n接诊编号：%s\n用药医嘱：\n%s结果：%s\n操作时间：%s\n",
            patient->name, list[choice - 1]->id,
            list[choice - 1]->medicalRecordId,
            medAdvice, codeText(code), timestamp);
    }
    openResult(out);
}

static void doAdmit(void)
{
    Patient *patient = choosePatient();
    Department *dept;
    Ward *ward;
    Bed *bed;
    MedicalRecord *regRecord;
    char date[DATE_LEN], recId[ID_LEN], payId[ID_LEN], out[RESULT_LEN], timestamp[TIME_LEN];
    double deposit;
    int code;
    if (patient == NULL) return;
    // 答辩注意：先检查该患者是否已经在住院，避免重复住院（难度：中等）
    if (findBedByPatient(patient->id) != NULL) {
        message(L"该患者已经在住院中，不能重复办理入院，请先办理出院。");
        return;
    }
    regRecord = findLatestRegisterByPatient(patient->id);
    // 答辩注意：无挂号记录时允许手动选择科室，适应急诊等特殊场景（难度：简单）
    if (regRecord == NULL) {
        Doctor *loginDoc = currentDoctor();
        if (loginDoc != NULL) {
            dept = findDepartmentById(loginDoc->deptId);
            if (dept == NULL) {
                message(L"当前医生关联科室不存在，无法办理入院。");
                return;
            }
        } else {
            char info[LINE_LEN];
            snprintf(info, sizeof(info), "该患者没有挂号记录。\n点击确定手动选择入院科室，点击取消返回。");
            if (MessageBox(GetHWnd(), toWide(info).c_str(), L"无挂号记录", MB_OKCANCEL) != IDOK) {
                return;
            }
            dept = chooseDepartment();
            if (dept == NULL) return;
        }
    } else {
        dept = findDepartmentById(regRecord->deptId);
        if (dept == NULL) {
            message(L"无法确定患者科室。");
            return;
        }
    }
    ward = chooseWardByDepartment(dept->id);
    if (ward == NULL) return;
    // 答辩注意：入院选择床位后，自动展示患者所属科室及对应单日护理费（难度：简单）
    {
        double nursingFee = 0.0;
        if (strcmp(dept->name, "内科") == 0) nursingFee = 18.0;
        else if (strcmp(dept->name, "外科") == 0) nursingFee = 28.0;
        else if (strcmp(dept->name, "骨科") == 0) nursingFee = 35.0;
        else if (strcmp(dept->name, "儿科") == 0) nursingFee = 32.0;
        else if (strcmp(dept->name, "皮肤科") == 0) nursingFee = 15.0;
        char info[512];
        // 答辩注意：未知科室护理费为0时发出警告，避免收费偏低（难度：简单）
        if (nursingFee == 0.0) {
            snprintf(info, sizeof(info),
                "患者所属科室：%s\n单日护理费：%.2f元/天\n\n⚠ 警告：该科室不在预设护理费列表中，护理费将按0元计算！\n请联系管理员添加该科室的护理费标准。\n\n（点击确定继续，点击取消返回）",
                dept->name, nursingFee);
            if (MessageBox(GetHWnd(), toWide(info).c_str(), L"⚠ 科室护理费未设置", MB_OKCANCEL) != IDOK) {
                return;
            }
        } else {
            snprintf(info, sizeof(info), "患者所属科室：%s\n单日护理费：%.2f元/天\n\n（护理费由系统自动匹配，不支持手动修改）",
                dept->name, nursingFee);
            MessageBox(GetHWnd(), toWide(info).c_str(), L"科室护理费信息", MB_OK);
        }
    }
    // 答辩注意：默认填充当天日期，减少用户输入（难度：简单）
    getCurrentDate(date, sizeof(date));
    bed = findFreeBed(dept->id, ward->type);
    if (bed == NULL) {
        message(L"没有符合条件的空床。");
        return;
    }
    // 入院押金缴纳：全方位严格容错校验，拦截空输入/纯空格/负数/字母/汉字/特殊符号/非法小数格式
    {
        char depositText[64] = "";
        FormField fields[] = {
            {L"入院日期", L"YYYY-MM-DD，默认当天，可点击修改", date, sizeof(date), FORM_DATE, 0, 0},
            {L"住院押金", L"", depositText, sizeof(depositText), FORM_MONEY, 0, 0}
        };
        if (!runForm(L"办理入院", L"点击输入框填写或修改入院信息。", fields, 2)) return;
        parseMoneyStrict(depositText, &deposit);
    }
    // 答辩注意：押金为0时给出警告，避免财务漏洞（难度：简单）
    if (deposit == 0.0) {
        if (MessageBox(GetHWnd(), L"押金金额为0元，住院期间费用将无法抵扣，是否继续？",
            L"押金为0警告", MB_OKCANCEL) != IDOK) {
            return;
        }
    }
    code = occupyBed(bed->id, patient->id, date);
    // 答辩注意：入院后检查返回值，给用户明确提示（难度：简单）
    if (code != OK) {
        snprintf(out, sizeof(out), "入院办理失败\n患者：%s\n床位：%s\n原因：%s\n",
            patient->name, bed->id, codeText(code));
        openResult(out);
        return;
    }
    makeNextRecordId(recId, sizeof(recId));
    code = addMedicalRecord(recId, patient->id, "", dept->id, "入院", date, 0, bed->id, "办理住院并分配床位", 1);
    // 答辩注意：入院记录写入失败时，必须释放已占用床位，不扣押金（难度：中等）
    if (code != OK) {
        releasePatientBed(patient->id);
        getCurrentDateTime(timestamp, sizeof(timestamp));
        snprintf(out, sizeof(out), "入院办理失败\n患者：%s\n床位：%s\n原因：%s\n操作时间：%s\n",
            patient->name, bed->id, codeText(code), timestamp);
        openResult(out);
        return;
    }
    // 答辩注意：住院押金单独生成一笔缴费记录，便于财务核查（难度：简单）
    makeNextPaymentId(payId, sizeof(payId));
    code = addPayment(payId, patient->id, "住院押金", bed->id, deposit, "已缴费", date, "入院押金");
    // 押金记录失败时也要释放床位，保证一致性
    if (code != OK) {
        releasePatientBed(patient->id);
        getCurrentDateTime(timestamp, sizeof(timestamp));
        snprintf(out, sizeof(out), "入院办理失败\n患者：%s\n床位：%s\n入院记录：%s\n押金记录失败：%s\n操作时间：%s\n",
            patient->name, bed->id, recId, codeText(code), timestamp);
        openResult(out);
        return;
    }
    getCurrentDateTime(timestamp, sizeof(timestamp));
    snprintf(out, sizeof(out), "入院办理完成\n患者：%s\n床位：%s\n入院记录：%s\n押金记录：%s\n押金金额：%.2f元\n结果：%s\n操作时间：%s\n",
        patient->name, bed->id, recId, payId, deposit, codeText(code), timestamp);
    openResult(out);
}

static void doLeave(void)
{
    Patient *patient = choosePatient();
    Bed *bed;
    Ward *ward;
    Department *dept;
    char date[DATE_LEN], recId[ID_LEN], payId[ID_LEN], out[RESULT_LEN], timestamp[TIME_LEN];
    int days;
    int code;
    if (patient == NULL) return;
    bed = findBedByPatient(patient->id);
    if (bed == NULL) {
        message(L"该患者没有占用床位。");
        return;
    }
    ward = findWardById(bed->wardId);
    dept = ward ? findDepartmentById(ward->deptId) : NULL;
    if (ward == NULL) {
        message(L"病房信息不存在，无法办理出院。");
        return;
    }
    days = -1;
    getCurrentDate(date, sizeof(date));
    {
        FormField fields[] = {
            {L"出院日期", L"YYYY-MM-DD，默认当天，可点击修改", date, sizeof(date), FORM_DATE, 0, 0}
        };
        if (!runForm(L"出院结算", L"点击输入框填写或修改出院日期。", fields, 1)) return;
        days = daysBetween(bed->inDate, date);
    }
    if (days <= 0) {
        message(L"出院日期不能早于或等于入院日期。");
        return;
    }
    // 答辩注意：按科室自动匹配护理费标准（难度：简单）
    double nursingFee = 0.0;
    if (dept != NULL) {
        if (strcmp(dept->name, "内科") == 0) nursingFee = 18.0;
        else if (strcmp(dept->name, "外科") == 0) nursingFee = 28.0;
        else if (strcmp(dept->name, "骨科") == 0) nursingFee = 35.0;
        else if (strcmp(dept->name, "儿科") == 0) nursingFee = 32.0;
        else if (strcmp(dept->name, "皮肤科") == 0) nursingFee = 15.0;
        // 答辩注意：未知科室护理费为0时发出警告（难度：简单）
        if (nursingFee == 0.0) {
            char warn[256];
            snprintf(warn, sizeof(warn),
                "科室「%s」不在预设护理费列表中，将按0元计算。\n请确认是否继续出院结算。",
                dept->name);
            if (MessageBox(GetHWnd(), toWide(warn).c_str(), L"⚠ 护理费未设置", MB_OKCANCEL) != IDOK) {
                return;
            }
        }
    }
    double bedFee = ward->dailyFee * days;
    double nursingTotal = nursingFee * days;
    // 答辩注意：汇总患者当前住院床位对应的预交押金金额，避免多次住院押金累加导致错误（难度：简单）
    Payment *p = paymentHead;
    double depositPaid = 0.0;
    while (p != NULL) {
        if (strcmp(p->patientId, patient->id) == 0 && strcmp(p->status, "已缴费") == 0 &&
            strcmp(p->sourceType, "住院押金") == 0 && strcmp(p->sourceId, bed->id) == 0) {
            depositPaid += p->amount;
        }
        p = p->next;
    }
    double totalFee = bedFee + nursingTotal;
    double balance = totalFee - depositPaid;
    double payAmount = balance > 0 ? balance : 0;
    {
        char preview[RESULT_LEN];
        snprintf(preview, sizeof(preview),
            "住院信息：\n患者：%s\n入院日期：%s\n出院日期：%s\n住院天数：%d天\n\n"
            "【费用明细】\n床位费：%.2f元（%s %.2f元/天 × %d天）\n"
            "护理费：%.2f元（%s %.2f元/天 × %d天）\n"
            "住院合计：%.2f元\n\n"
            "【押金信息】\n预交押金：%.2f元\n%s\n\n"
            "%s",
            patient->name, bed->inDate, date, days,
            bedFee, ward->type, ward->dailyFee, days,
            nursingTotal, dept ? dept->name : "未知科室", nursingFee, days,
            totalFee,
            depositPaid,
            balance >= 0 ? "押金不足，需补交差额" : "押金有余，应退费",
            payAmount > 0 ?
                "点击确定生成待缴住院费账单。缴清后将自动出院并释放床位。" :
                "押金已覆盖全部住院费用，点击确定将直接办理出院。");
        if (MessageBox(GetHWnd(), toWide(preview).c_str(), L"住院费用预览", MB_OKCANCEL) != IDOK) {
            message(L"已取消出院结算。");
            return;
        }
    }
    getCurrentDateTime(timestamp, sizeof(timestamp));
    if (payAmount <= 0) {
        makeNextRecordId(recId, sizeof(recId));
        code = addMedicalRecord(recId, patient->id, "", dept ? dept->id : ward->deptId,
            "出院", date, totalFee, "", "出院结算并释放床位（押金已抵扣）", 1);
        if (code == OK) code = releasePatientBed(patient->id);
        snprintf(out, sizeof(out),
            "出院办理完成（押金已覆盖费用）\n患者：%s\n住院天数：%d天\n\n"
            "【费用明细】\n床位费：%.2f元\n护理费：%.2f元\n住院合计：%.2f元\n\n"
            "【押金信息】\n预交押金：%.2f元\n应退费：%.2f元\n\n"
            "结果：%s\n操作时间：%s\n",
            patient->name, days,
            bedFee, nursingTotal, totalFee,
            depositPaid, -balance,
            codeText(code), timestamp);
        openResult(out);
        return;
    }
    makeNextPaymentId(payId, sizeof(payId));
    code = addPayment(payId, patient->id, "住院", bed->id, payAmount, "未缴费", date, "出院结算");
    snprintf(out, sizeof(out),
        "出院费用账单已生成（待缴费）\n患者：%s\n住院天数：%d天\n\n"
        "【费用明细】\n床位费：%.2f元\n护理费：%.2f元\n住院合计：%.2f元\n\n"
        "【押金信息】\n预交押金：%.2f元\n待缴金额：%.2f元\n缴费编号：%s\n\n"
        "请前往「待缴费」缴清住院费，缴清后将自动出院并释放床位。\n结果：%s\n操作时间：%s\n",
        patient->name, days,
        bedFee, nursingTotal, totalFee,
        depositPaid, payAmount, payId,
        codeText(code), timestamp);
    openResult(out);
}

static void managePatients(void)
{
    int choice;
    char out[RESULT_LEN];
    if (!askChoice(L"患者管理", "1. 新增患者\n2. 按姓名查询\n3. 修改电话和状态\n4. 删除患者\n5. 患者列表", 5, &choice)) return;
    if (choice == 1) { doAddPatient(); return; }
    if (choice == 2) {
        char name[NAME_LEN];
        if (!askName(L"患者姓名", name, sizeof(name))) return;
        queryPatientByName(name, out, sizeof(out));
        openResult(out);
    } else if (choice == 3) {
        Patient *p = choosePatient();
        char phone[PHONE_LEN], status[STATUS_LEN];
        int code;
        if (p == NULL) return;
        {
            FormField fields[] = {
                {L"联系电话", L"", phone, sizeof(phone), FORM_PHONE, 0, 0},
                {L"患者状态", L"例如 门诊/住院，可为空", status, sizeof(status), FORM_TEXT, 0, 0}
            };
            safeCopy(phone, sizeof(phone), p->phone);
            safeCopy(status, sizeof(status), p->status);
            if (!runForm(L"修改患者", L"点击输入框修改联系电话和患者状态。", fields, 2)) return;
        }
        code = updatePatient(p->id, phone, status);
        snprintf(out, sizeof(out), "修改患者结果：%s\n", codeText(code));
        writeAdminLog("修改患者", p->id, codeText(code));
        openResult(out);
    } else if (choice == 4) {
        Patient *p = choosePatient();
        int code;
        char patientId[ID_LEN];
        if (p == NULL) return;
        safeCopy(patientId, sizeof(patientId), p->id);
        code = deletePatient(p->id);
        snprintf(out, sizeof(out), "删除患者结果：%s\n", codeText(code));
        writeAdminLog("删除患者", patientId, codeText(code));
        openResult(out);
    } else {
        listPatients(out, sizeof(out));
        openResult(out);
    }
}

static void manageDepartments(void)
{
    int choice, code;
    char id[ID_LEN], name[NAME_LEN], intro[TEXT_LEN], wardType[NAME_LEN], out[RESULT_LEN];
    Department *dept;
    if (!askChoice(L"科室管理", "1. 新增科室\n2. 科室列表\n3. 修改科室\n4. 删除科室", 4, &choice)) return;
    if (choice == 1) {
        FormField fields[] = {
            {L"科室名称", L"", name, sizeof(name), FORM_NAME, 0, 0},
            {L"科室简介", L"可为空，不能含 | 或换行", intro, sizeof(intro), FORM_TEXT, 0, 0},
            {L"病房类型", L"关联病房类型，可为空", wardType, sizeof(wardType), FORM_TEXT, 0, 0}
        };
        clearText(name, sizeof(name));
        clearText(intro, sizeof(intro));
        clearText(wardType, sizeof(wardType));
        if (!runForm(L"新增科室", L"点击输入框填写科室基础信息。", fields, 3)) return;
        makeNextDepartmentId(id, sizeof(id));
        code = addDepartment(id, name, intro, wardType);
        snprintf(out, sizeof(out), "新增科室编号：%s\n结果：%s\n", id, codeText(code));
        writeAdminLog("新增科室", id, codeText(code));
        openResult(out);
    } else if (choice == 2) {
        listDepartments(out, sizeof(out));
        openResult(out);
    } else if (choice == 3) {
        dept = chooseDepartment();
        if (dept == NULL) return;
        {
            FormField fields[] = {
                {L"新简介", L"可为空，不能含 | 或换行", intro, sizeof(intro), FORM_TEXT, 0, 0},
                {L"新病房类型", L"可为空，不能含 | 或换行", wardType, sizeof(wardType), FORM_TEXT, 0, 0}
            };
            safeCopy(intro, sizeof(intro), dept->intro);
            safeCopy(wardType, sizeof(wardType), dept->wardType);
            if (!runForm(L"修改科室", L"点击输入框修改科室简介和病房类型。", fields, 2)) return;
        }
        code = updateDepartment(dept->id, intro, wardType);
        snprintf(out, sizeof(out), "修改科室结果：%s\n", codeText(code));
        writeAdminLog("修改科室", dept->id, codeText(code));
        openResult(out);
    } else {
        char deptId[ID_LEN];
        dept = chooseDepartment();
        if (dept == NULL) return;
        safeCopy(deptId, sizeof(deptId), dept->id);
        code = deleteDepartment(dept->id);
        snprintf(out, sizeof(out), "删除科室结果：%s\n", codeText(code));
        writeAdminLog("删除科室", deptId, codeText(code));
        openResult(out);
    }
}

static void manageDoctors(void)
{
    int choice, maxCount, code;
    char maxCountText[32];
    char id[ID_LEN], name[NAME_LEN], title[NAME_LEN], workTime[NAME_LEN], out[RESULT_LEN];
    Department *dept;
    Doctor *doctor;
    if (!askChoice(L"医生管理", "1. 新增医生\n2. 医生列表\n3. 修改医生\n4. 删除医生", 4, &choice)) return;
    if (choice == 1) {
        dept = chooseDepartment();
        if (dept == NULL) return;
        clearText(maxCountText, sizeof(maxCountText));
        {
            FormField fields[] = {
                {L"医生姓名", L"", name, sizeof(name), FORM_NAME, 0, 0},
                {L"职称", L"可为空，不能含 | 或换行", title, sizeof(title), FORM_TEXT, 0, 0},
                {L"出诊时间", L"可为空，不能含 | 或换行", workTime, sizeof(workTime), FORM_TEXT, 0, 0},
                {L"最大号源", L"", maxCountText, sizeof(maxCountText), FORM_INT, 1, 200}
            };
            clearText(name, sizeof(name));
            clearText(title, sizeof(title));
            clearText(workTime, sizeof(workTime));
            if (!runForm(L"新增医生", L"点击输入框填写医生信息。", fields, 4)) return;
            parseIntStrict(maxCountText, &maxCount);
        }
        makeNextDoctorId(id, sizeof(id));
        code = addDoctor(id, name, dept->id, title, workTime, maxCount);
        snprintf(out, sizeof(out), "新增医生编号：%s\n结果：%s\n", id, codeText(code));
        writeAdminLog("新增医生", id, codeText(code));
        openResult(out);
    } else if (choice == 2) {
        listDoctors(out, sizeof(out));
        openResult(out);
    } else if (choice == 3) {
        doctor = chooseDoctor();
        if (doctor == NULL) return;
        {
            FormField fields[] = {
                {L"新职称", L"可为空，不能含 | 或换行", title, sizeof(title), FORM_TEXT, 0, 0},
                {L"新出诊时间", L"可为空，不能含 | 或换行", workTime, sizeof(workTime), FORM_TEXT, 0, 0},
                {L"新最大号源", L"", maxCountText, sizeof(maxCountText), FORM_INT, 1, 200}
            };
            safeCopy(title, sizeof(title), doctor->title);
            safeCopy(workTime, sizeof(workTime), doctor->workTime);
            snprintf(maxCountText, sizeof(maxCountText), "%d", doctor->maxCount);
            if (!runForm(L"修改医生", L"点击输入框修改职称、出诊时间和号源。", fields, 3)) return;
            parseIntStrict(maxCountText, &maxCount);
        }
        code = updateDoctor(doctor->id, title, workTime, maxCount);
        snprintf(out, sizeof(out), "修改医生结果：%s\n", codeText(code));
        writeAdminLog("修改医生", doctor->id, codeText(code));
        openResult(out);
    } else {
        char doctorId[ID_LEN];
        doctor = chooseDoctor();
        if (doctor == NULL) return;
        safeCopy(doctorId, sizeof(doctorId), doctor->id);
        code = deleteDoctor(doctor->id);
        snprintf(out, sizeof(out), "删除医生结果：%s\n", codeText(code));
        writeAdminLog("删除医生", doctorId, codeText(code));
        openResult(out);
    }
}

static void manageMedicines(void)
{
    int choice, stock, lowLimit, code;
    double price;
    char priceText[64], stockText[32], lowLimitText[32];
    char id[ID_LEN], name[NAME_LEN], commonName[NAME_LEN], tradeName[NAME_LEN], alias[NAME_LEN], out[RESULT_LEN];
    Department *dept;
    Medicine *med;
    if (!askChoice(L"药品管理", "1. 新增药品\n2. 药品列表\n3. 修改价格库存\n4. 删除药品", 4, &choice)) return;
    if (choice == 1) {
        dept = chooseDepartment();
        if (dept == NULL) return;
        {
            FormField fields[] = {
                {L"药品名", L"", name, sizeof(name), FORM_NAME, 0, 0},
                {L"通用名", L"", commonName, sizeof(commonName), FORM_NAME, 0, 0},
                {L"商品名", L"", tradeName, sizeof(tradeName), FORM_NAME, 0, 0},
                {L"别名", L"可为空，不能含 | 或换行", alias, sizeof(alias), FORM_TEXT, 0, 0},
                {L"单价", L"", priceText, sizeof(priceText), FORM_MONEY, 0, 0},
                {L"库存", L"", stockText, sizeof(stockText), FORM_INT, 0, 9999},
                {L"库存下限", L"", lowLimitText, sizeof(lowLimitText), FORM_INT, 0, 9999}
            };
            clearText(name, sizeof(name));
            clearText(commonName, sizeof(commonName));
            clearText(tradeName, sizeof(tradeName));
            clearText(alias, sizeof(alias));
            clearText(priceText, sizeof(priceText));
            clearText(stockText, sizeof(stockText));
            clearText(lowLimitText, sizeof(lowLimitText));
            if (!runForm(L"新增药品", L"点击输入框填写药品资料、价格和库存。", fields, 7)) return;
            parseMoneyStrict(priceText, &price);
            parseIntStrict(stockText, &stock);
            parseIntStrict(lowLimitText, &lowLimit);
        }
        makeNextMedicineId(id, sizeof(id));
        code = addMedicine(id, name, commonName, tradeName, alias, dept->id, price, stock, lowLimit);
        snprintf(out, sizeof(out), "新增药品编号：%s\n结果：%s\n", id, codeText(code));
        writeAdminLog("新增药品", id, codeText(code));
        openResult(out);
    } else if (choice == 2) {
        listMedicines(out, sizeof(out));
        openResult(out);
    } else if (choice == 3) {
        med = chooseMedicine();
        if (med == NULL) return;
        {
            FormField fields[] = {
                {L"新单价", L"", priceText, sizeof(priceText), FORM_MONEY, 0, 0},
                {L"新库存", L"", stockText, sizeof(stockText), FORM_INT, 0, 9999},
                {L"新库存下限", L"", lowLimitText, sizeof(lowLimitText), FORM_INT, 0, 9999}
            };
            snprintf(priceText, sizeof(priceText), "%.2f", med->price);
            snprintf(stockText, sizeof(stockText), "%d", med->stock);
            snprintf(lowLimitText, sizeof(lowLimitText), "%d", med->lowLimit);
            if (!runForm(L"修改药品", L"点击输入框修改价格、库存和库存下限。", fields, 3)) return;
            parseMoneyStrict(priceText, &price);
            parseIntStrict(stockText, &stock);
            parseIntStrict(lowLimitText, &lowLimit);
        }
        code = updateMedicine(med->id, price, stock, lowLimit);
        snprintf(out, sizeof(out), "修改药品结果：%s\n", codeText(code));
        writeAdminLog("修改药品", med->id, codeText(code));
        openResult(out);
    } else {
        char medicineId[ID_LEN];
        med = chooseMedicine();
        if (med == NULL) return;
        safeCopy(medicineId, sizeof(medicineId), med->id);
        code = deleteMedicine(med->id);
        snprintf(out, sizeof(out), "删除药品结果：%s\n", codeText(code));
        writeAdminLog("删除药品", medicineId, codeText(code));
        openResult(out);
    }
}

static void manageWardBed(void)
{
    int choice, total, freeBeds, code;
    double dailyFee;
    char dailyFeeText[64], totalText[32], freeBedsText[32];
    char id[ID_LEN], type[NAME_LEN], out[RESULT_LEN], status[STATUS_LEN];
    Department *dept;
    Ward *ward;
    Bed *bed;
    if (!askChoice(L"病房床位管理", "1. 新增病房\n2. 新增床位\n3. 病房列表\n4. 床位列表\n5. 修改病房日费\n6. 删除空床", 6, &choice)) return;
    if (choice == 1) {
        dept = chooseDepartment();
        if (dept == NULL) return;
        {
            FormField fields[] = {
                {L"病房类型", L"可为空，不能含 | 或换行", type, sizeof(type), FORM_TEXT, 0, 0},
                {L"每日费用", L"", dailyFeeText, sizeof(dailyFeeText), FORM_MONEY, 0, 0},
                {L"总床位", L"", totalText, sizeof(totalText), FORM_INT, 1, 999},
                {L"空床数", L"先填写总床位，确定后会检查空床数不能超过总床位", freeBedsText, sizeof(freeBedsText), FORM_INT, 0, 999}
            };
            clearText(type, sizeof(type));
            clearText(dailyFeeText, sizeof(dailyFeeText));
            clearText(totalText, sizeof(totalText));
            clearText(freeBedsText, sizeof(freeBedsText));
            if (!runForm(L"新增病房", L"点击输入框填写病房类型、费用和床位数。", fields, 4)) return;
            parseMoneyStrict(dailyFeeText, &dailyFee);
            parseIntStrict(totalText, &total);
            parseIntStrict(freeBedsText, &freeBeds);
            if (freeBeds > total) {
                message(L"空床数不能大于总床位。");
                return;
            }
        }
        makeNextWardId(id, sizeof(id));
        code = addWard(id, type, dept->id, dailyFee, total, freeBeds);
        snprintf(out, sizeof(out), "新增病房编号：%s\n结果：%s\n", id, codeText(code));
        writeAdminLog("新增病房", id, codeText(code));
        openResult(out);
    } else if (choice == 2) {
        ward = chooseWardAll();
        if (ward == NULL) return;
        {
            FormField fields[] = {
                {L"床位状态", L"空闲 或 占用。新增演示建议用 空闲", status, sizeof(status), FORM_TEXT, 0, 0}
            };
            safeCopy(status, sizeof(status), "空闲");
            if (!runForm(L"新增床位", L"点击输入框填写床位状态。", fields, 1)) return;
        }
        makeNextBedId(out, sizeof(out));
        code = addBed(out, ward->id, status, "", "");
        snprintf(resultText, sizeof(resultText), "新增床位编号：%s\n结果：%s\n", out, codeText(code));
        writeAdminLog("新增床位", out, codeText(code));
        openResult(resultText);
    } else if (choice == 3) {
        listWards(out, sizeof(out));
        openResult(out);
    } else if (choice == 4) {
        listBeds(out, sizeof(out));
        openResult(out);
    } else if (choice == 5) {
        ward = chooseWardAll();
        if (ward == NULL) return;
        {
            FormField fields[] = {
                {L"新每日费用", L"", dailyFeeText, sizeof(dailyFeeText), FORM_MONEY, 0, 0}
            };
            snprintf(dailyFeeText, sizeof(dailyFeeText), "%.2f", ward->dailyFee);
            if (!runForm(L"修改病房", L"点击输入框修改病房每日费用。", fields, 1)) return;
            parseMoneyStrict(dailyFeeText, &dailyFee);
        }
        code = updateWard(ward->id, dailyFee);
        snprintf(out, sizeof(out), "修改病房结果：%s\n", codeText(code));
        writeAdminLog("修改病房", ward->id, codeText(code));
        openResult(out);
    } else {
        char bedId[ID_LEN];
        bed = chooseFreeBed();
        if (bed == NULL) return;
        safeCopy(bedId, sizeof(bedId), bed->id);
        code = deleteBed(bed->id);
        snprintf(out, sizeof(out), "删除床位结果：%s\n", codeText(code));
        writeAdminLog("删除床位", bedId, codeText(code));
        openResult(out);
    }
}

static void showWarningCenter(void)
{
    char out[RESULT_LEN];
    buildWarningReport(out, sizeof(out));
    writeAdminLog("查看预警中心", "预警中心", "成功");
    openResult(out);
}

static void showAdminLogs(void)
{
    FILE *fp;
    char out[RESULT_LEN];
    char line[LINE_LEN];

    clearText(out, sizeof(out));
    appendLine(out, sizeof(out), "操作日志");
    appendLine(out, sizeof(out), "格式：时间 | 管理员 | 操作类型 | 对象 | 结果\n");

    fp = fopen("data\\admin_log.txt", "r");
    if (fp == NULL) {
        appendLine(out, sizeof(out), "暂无操作日志。");
        writeAdminLog("查看操作日志", "admin_log.txt", "成功：暂无日志");
        openResult(out);
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        removeLineEnd(line);
        appendText(out, sizeof(out), line);
        appendLine(out, sizeof(out), "");
    }
    fclose(fp);
    writeAdminLog("查看操作日志", "admin_log.txt", "成功");
    openResult(out);
}

static void buildBackupList(char *out, int outSize)
{
    WIN32_FIND_DATAA data;
    HANDLE h;
    int count = 0;

    clearText(out, outSize);
    appendLine(out, outSize, "备份列表");
    appendLine(out, outSize, "目录格式：backup/YYYY-MM-DD_HHMMSS\n");

    if (!isDirectoryA("backup")) {
        appendLine(out, outSize, "暂无备份目录。");
        return;
    }

    h = FindFirstFileA("backup\\*", &data);
    if (h == INVALID_HANDLE_VALUE) {
        appendLine(out, outSize, "暂无备份目录。");
        return;
    }
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(data.cFileName, ".") != 0 &&
            strcmp(data.cFileName, "..") != 0) {
            char line[LINE_LEN];
            snprintf(line, sizeof(line), "%d. backup\\%s\n", ++count, data.cFileName);
            appendText(out, outSize, line);
        }
    } while (FindNextFileA(h, &data));
    FindClose(h);

    if (count == 0) appendLine(out, outSize, "暂无备份目录。");
}

static void manageBackupRestore(void)
{
    int choice;
    char out[RESULT_LEN];
    if (!askChoice(L"备份恢复", "1. 手动备份\n2. 恢复最近备份\n3. 查看备份列表", 3, &choice)) return;

    if (choice == 1) {
        performBackup("手动备份", out, sizeof(out));
        openResult(out);
        return;
    }
    if (choice == 2) {
        if (MessageBox(GetHWnd(),
            L"恢复会用最近备份覆盖当前 data 目录中的业务数据。\n该操作只允许管理员手动触发，确认继续吗？",
            L"确认恢复最近备份", MB_OKCANCEL | MB_ICONWARNING) != IDOK) {
            writeAdminLog("恢复备份", "backup", "取消");
            return;
        }
        restoreLatestBackup(out, sizeof(out));
        openResult(out);
        return;
    }

    buildBackupList(out, sizeof(out));
    writeAdminLog("查看备份列表", "backup", "成功");
    openResult(out);
}

static void showReports(void)
{
    int choice;
    char out[RESULT_LEN];
    if (!askChoice(L"统计报表", "1. 收入统计\n2. 床位使用\n3. 药品库存\n4. 医生工作量\n5. 科室工作量\n6. 所有医疗记录\n7. 所有缴费记录\n8. 所有处方", 8, &choice)) return;
    if (choice == 1) reportIncome(out, sizeof(out));
    else if (choice == 2) reportBedUse(out, sizeof(out));
    else if (choice == 3) reportMedicineStock(out, sizeof(out));
    else if (choice == 4) reportDoctorWork(out, sizeof(out));
    else if (choice == 5) reportDepartmentWork(out, sizeof(out));
    else if (choice == 6) listRecords(out, sizeof(out));
    else if (choice == 7) listPayments(out, sizeof(out));
    else listPrescriptions(out, sizeof(out));
    openResult(out);
}

static void doRunTests(void)
{
    char saveMsg[TEXT_LEN];
    char loadMsg[TEXT_LEN];
    char out[RESULT_LEN];
    saveAllData("data", saveMsg, sizeof(saveMsg));
    runAllTests(out, sizeof(out));
    loadAllData("data", loadMsg, sizeof(loadMsg));
    appendLine(out, sizeof(out), "");
    appendLine(out, sizeof(out), "测试结束后已尝试重新读取 data 文件夹，避免污染演示数据。");
    writeAdminLog("运行系统测试", "测试数据", "完成");
    openResult(out);
}

static void doSave(void)
{
    char msg[TEXT_LEN];
    saveAllData("data", msg, sizeof(msg));
    if (saveAuthData("data\\account.txt") != OK) {
        appendLine(msg, sizeof(msg), "账号数据保存失败，请检查 data 文件夹。");
    } else {
        appendLine(msg, sizeof(msg), "账号数据保存完成。");
    }
    writeAdminLog("保存数据", "data", "完成");
    openResult(msg);
}

static void doLoad(void)
{
    char msg[TEXT_LEN];
    loadAllData("data", msg, sizeof(msg));
    if (loadAuthData("data\\account.txt") != OK) {
        appendLine(msg, sizeof(msg), "账号数据读取失败，请检查 account.txt。");
    } else {
        appendLine(msg, sizeof(msg), "账号数据读取完成。");
    }
    if (currentPatient() == NULL) clearText(loggedPatientId, sizeof(loggedPatientId));
    if (currentDoctor() == NULL) clearText(loggedDoctorId, sizeof(loggedDoctorId));
    writeAdminLog("读取数据", "data", "完成");
    openResult(msg);
}

static COLORREF buttonColor(int action)
{
    if (action == 0 || action == 90 || action == 91 || action == 101) {
        return RGB(83, 198, 188);
    }
    if (action == 99) {
        return RGB(132, 70, 96);
    }
    return RGB(88, 61, 150);
}

static void drawTop(const wchar_t *title)
{
    setbkcolor(WHITE);
    cleardevice();
    setbkmode(TRANSPARENT);

    useClearFont(38, FW_SEMIBOLD);
    settextcolor(BLACK);
    outtextxy(70, 72, title);
}

static void drawButtons(Button buttons[], int count)
{
    int i;
    useClearFont(30, FW_SEMIBOLD);
    setbkmode(TRANSPARENT);
    for (i = 0; i < count; i++) {
        setfillcolor(buttonColor(buttons[i].action));
        setlinecolor(BLACK);
        setlinestyle(PS_SOLID, 3);
        solidrectangle(buttons[i].x1, buttons[i].y1, buttons[i].x2, buttons[i].y2);
        rectangle(buttons[i].x1, buttons[i].y1, buttons[i].x2, buttons[i].y2);
        RECT r = { buttons[i].x1, buttons[i].y1, buttons[i].x2, buttons[i].y2 };
        settextcolor(WHITE);
        drawtext(buttons[i].text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    setlinestyle(PS_SOLID, 1);
}

static void drawSmallNote(const wchar_t *text)
{
    useClearFont(22, FW_SEMIBOLD);
    settextcolor(RGB(25, 35, 50));
    outtextxy(74, 150, text);
}

static void drawMetricCard(int x1, int y1, int x2, int y2, const wchar_t *label, const char *value, COLORREF accent)
{
    RECT labelRect = { x1 + 14, y1 + 10, x2 - 14, y1 + 34 };
    RECT valueRect = { x1 + 14, y1 + 38, x2 - 14, y2 - 10 };

    setfillcolor(RGB(248, 250, 252));
    setlinecolor(accent);
    setlinestyle(PS_SOLID, 2);
    solidrectangle(x1, y1, x2, y2);
    rectangle(x1, y1, x2, y2);

    useClearFont(17, FW_SEMIBOLD);
    settextcolor(RGB(82, 92, 105));
    drawtext(label, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    useClearFont(25, FW_SEMIBOLD);
    settextcolor(RGB(18, 24, 38));
    drawtext(toWide(value).c_str(), &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void drawAdminDashboard(void)
{
    DashboardStats stats;
    WarningStats warnings;
    char value[64];
    wchar_t summary[256];
    RECT warnRect = {70, 238, 990, 298};

    buildDashboardStats(&stats);
    buildWarningStats(&warnings);

    snprintf(value, sizeof(value), "%.2f 元", stats.totalIncome);
    drawMetricCard(70, 130, 245, 215, L"总收入", value, RGB(31, 128, 96));

    snprintf(value, sizeof(value), "%d", stats.todayRegisterCount);
    drawMetricCard(260, 130, 435, 215, L"今日挂号数", value, RGB(37, 99, 235));

    snprintf(value, sizeof(value), "%d", stats.unpaidCount);
    drawMetricCard(450, 130, 625, 215, L"待缴费数量", value, RGB(217, 119, 6));

    snprintf(value, sizeof(value), "%.1f%%", stats.bedOccupancyRate);
    drawMetricCard(640, 130, 815, 215, L"住院占床率", value, RGB(126, 34, 206));

    snprintf(value, sizeof(value), "%d", stats.lowStockMedicineCount);
    drawMetricCard(830, 130, 990, 215, L"低库存药品", value, RGB(190, 18, 60));

    setfillcolor(RGB(255, 251, 235));
    setlinecolor(RGB(245, 158, 11));
    setlinestyle(PS_SOLID, 2);
    solidrectangle(warnRect.left, warnRect.top, warnRect.right, warnRect.bottom);
    rectangle(warnRect.left, warnRect.top, warnRect.right, warnRect.bottom);

    useClearFont(21, FW_SEMIBOLD);
    settextcolor(RGB(120, 53, 15));
    outtextxy(90, 252, L"预警摘要");

    swprintf(summary, 256, L"低库存 %d 项    未缴费 %d 笔    床位紧张 %d 个病房    号源不足 %d 名医生",
        warnings.lowStockMedicineCount,
        warnings.unpaidPaymentCount,
        warnings.bedShortageWardCount,
        warnings.lowDoctorSlotCount);
    useClearFont(20, FW_SEMIBOLD);
    settextcolor(RGB(30, 41, 59));
    outtextxy(230, 253, summary);

    setlinestyle(PS_SOLID, 1);
}

static int countResultLines(void)
{
    int i;
    int count = 1;
    if (resultText[0] == '\0') {
        return 0;
    }
    for (i = 0; resultText[i] != '\0'; i++) {
        if (resultText[i] == '\n') {
            count++;
        }
    }
    return count;
}

static void copyResultPage(char *out, int outSize, int pageNo, int pageSize)
{
    int line = 0;
    int start = pageNo * pageSize;
    int end = start + pageSize;
    int i = 0;
    int pos = 0;
    clearText(out, outSize);
    while (resultText[i] != '\0' && pos < outSize - 1) {
        if (line >= start && line < end) {
            out[pos++] = resultText[i];
        }
        if (resultText[i] == '\n') {
            line++;
            if (line >= end) {
                break;
            }
        }
        i++;
    }
    out[pos] = '\0';
}

static void drawResultPage(void)
{
    const int pageSize = 10;
    int totalLine = countResultLines();
    int totalPage = totalLine == 0 ? 1 : (totalLine + pageSize - 1) / pageSize;
    char pageText[RESULT_LEN];
    wchar_t pageInfo[64];
    Button pageButtons[] = {
        {250, 500, 370, 550, L"首页", 101},
        {370, 500, 510, 550, L"上一页", 102},
        {510, 500, 650, 550, L"下一页", 103},
        {650, 500, 770, 550, L"末页", 104},
        {830, 500, 980, 550, L"返回", 100}
    };
    if (resultPageNo < 0) resultPageNo = 0;
    if (resultPageNo >= totalPage) resultPageNo = totalPage - 1;
    copyResultPage(pageText, sizeof(pageText), resultPageNo, pageSize);

    drawTop(L"信息列表");
    useClearFont(22, FW_SEMIBOLD);
    settextcolor(RGB(25, 35, 50));
    swprintf(pageInfo, 64, L"第 %d / %d 页", resultPageNo + 1, totalPage);
    outtextxy(72, 150, pageInfo);

    setfillcolor(WHITE);
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 2);
    rectangle(55, 190, 1005, 480);

    RECT r = {75, 205, 985, 470};
    useClearFont(24, FW_SEMIBOLD);
    settextcolor(BLACK);
    drawtext(toWide(pageText).c_str(), &r, DT_LEFT | DT_TOP | DT_WORDBREAK);
    drawButtons(pageButtons, 5);
}

static void drawPage(void)
{
    Button home[] = {
        {410, 200, 650, 265, L"患者端", 1},
        {410, 300, 650, 365, L"医护端", 2},
        {410, 400, 650, 465, L"管理端", 3}
    };
    Button patient[] = {
        {90, 220, 310, 285, L"退出登录", 16},
        {330, 220, 550, 285, L"挂号", 11},
        {570, 220, 790, 285, L"个人档案", 12},
        {90, 305, 310, 370, L"待缴费", 15},
        {330, 305, 550, 370, L"我的账单", 13},
        {570, 305, 790, 370, L"处方明细", 14},
        {830, 510, 980, 575, L"返回", 0}
    };
    Button medical[] = {
        {120, 230, 345, 295, L"接诊", 20},
        {420, 230, 645, 295, L"检查开单", 21},
        {720, 230, 945, 295, L"开处方", 22},
        {120, 380, 345, 445, L"处方发药", 23},
        {420, 380, 645, 445, L"办理入院", 24},
        {720, 380, 945, 445, L"出院结算", 25},
        {830, 520, 980, 580, L"返回", 0}
    };
    Button admin[] = {
        {65, 325, 225, 375, L"患者管理", 30},
        {245, 325, 405, 375, L"医生管理", 31},
        {425, 325, 585, 375, L"科室管理", 32},
        {605, 325, 765, 375, L"药品管理", 33},
        {785, 325, 945, 375, L"病房床位", 34},
        {65, 395, 225, 445, L"详细报表", 35},
        {245, 395, 405, 445, L"预警中心", 37},
        {425, 395, 585, 445, L"操作日志", 38},
        {605, 395, 765, 445, L"备份恢复", 39},
        {785, 395, 945, 445, L"系统测试", 36},
        {155, 485, 315, 535, L"读取数据", 90},
        {450, 485, 610, 535, L"保存数据", 91},
        {745, 485, 905, 535, L"退出", 0}
    };
    Button patientAuth[] = {
        {300, 230, 500, 295, L"登录", 40},
        {560, 230, 760, 295, L"注册", 41},
        {830, 510, 980, 575, L"返回", 0}
    };
    Button doctorAuth[] = {
        {300, 230, 500, 295, L"登录", 42},
        {560, 230, 760, 295, L"注册", 43},
        {830, 510, 980, 575, L"返回", 0}
    };
    if (currentPage == PAGE_HOME) {
        setbkcolor(WHITE);
        cleardevice();
        setbkmode(TRANSPARENT);
        useClearFont(46, FW_SEMIBOLD);
        settextcolor(BLACK);
        {
            RECT titleRect = {0, 90, WIN_W, 150};
            drawtext(L"轻量级医疗管理系统", &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        drawButtons(home, 3);
    } else if (currentPage == PAGE_PATIENT) {
        drawTop(L"患者端");
        drawSmallNote(L"录入与挂号；档案可查挂号状态；处方明细含医嘱与缴费指引；待缴费用「待缴费」办理。");
        drawButtons(patient, 7);
    } else if (currentPage == PAGE_MEDICAL) {
        drawTop(L"医护端");
        drawSmallNote(L"看诊、检查开单、处方、发药、入院、出院；检查费与药费须先缴费后再继续后续步骤。");
        drawButtons(medical, 7);
    } else if (currentPage == PAGE_ADMIN) {
        drawTop(L"管理端治理与运营中心");
        drawAdminDashboard();
        drawButtons(admin, 13);
    } else if (currentPage == PAGE_PATIENT_AUTH) {
        drawTop(L"患者端");
        drawSmallNote(L"请先用唯一手机号登录；新患者在注册页一次性填写患者信息。");
        drawButtons(patientAuth, 3);
    } else if (currentPage == PAGE_DOCTOR_AUTH) {
        drawTop(L"医护端");
        drawSmallNote(L"请先用唯一手机号登录；医生注册需绑定管理端已有医生档案。");
        drawButtons(doctorAuth, 3);
    } else {
        drawResultPage();
    }
}

static int hit(Button buttons[], int count, int x, int y)
{
    int i;
    for (i = 0; i < count; i++) {
        if (x >= buttons[i].x1 && x <= buttons[i].x2 && y >= buttons[i].y1 && y <= buttons[i].y2) {
            return buttons[i].action;
        }
    }
    return -1;
}

static int actionAt(int x, int y)
{
    Button home[] = {
        {410, 200, 650, 265, L"", 1}, {410, 300, 650, 365, L"", 2},
        {410, 400, 650, 465, L"", 3}
    };
    Button patient[] = {
        {90, 220, 310, 285, L"", 16}, {330, 220, 550, 285, L"", 11},
        {570, 220, 790, 285, L"", 12}, {90, 305, 310, 370, L"", 15},
        {330, 305, 550, 370, L"", 13}, {570, 305, 790, 370, L"", 14},
        {830, 510, 980, 575, L"", 0}
    };
    Button medical[] = {
        {120, 230, 345, 295, L"", 20}, {420, 230, 645, 295, L"", 21},
        {720, 230, 945, 295, L"", 22}, {120, 380, 345, 445, L"", 23},
        {420, 380, 645, 445, L"", 24}, {720, 380, 945, 445, L"", 25},
        {830, 520, 980, 580, L"", 0}
    };
    Button admin[] = {
        {65, 325, 225, 375, L"", 30}, {245, 325, 405, 375, L"", 31},
        {425, 325, 585, 375, L"", 32}, {605, 325, 765, 375, L"", 33},
        {785, 325, 945, 375, L"", 34}, {65, 395, 225, 445, L"", 35},
        {245, 395, 405, 445, L"", 37}, {425, 395, 585, 445, L"", 38},
        {605, 395, 765, 445, L"", 39}, {785, 395, 945, 445, L"", 36},
        {155, 485, 315, 535, L"", 90}, {450, 485, 610, 535, L"", 91},
        {745, 485, 905, 535, L"", 0}
    };
    Button result[] = {
        {250, 500, 370, 550, L"", 101}, {370, 500, 510, 550, L"", 102},
        {510, 500, 650, 550, L"", 103}, {650, 500, 770, 550, L"", 104},
        {830, 500, 980, 550, L"", 100}
    };
    Button patientAuth[] = {
        {300, 230, 500, 295, L"", 40}, {560, 230, 760, 295, L"", 41},
        {830, 510, 980, 575, L"", 0}
    };
    Button doctorAuth[] = {
        {300, 230, 500, 295, L"", 42}, {560, 230, 760, 295, L"", 43},
        {830, 510, 980, 575, L"", 0}
    };
    if (currentPage == PAGE_HOME) return hit(home, 3, x, y);
    if (currentPage == PAGE_PATIENT) return hit(patient, 7, x, y);
    if (currentPage == PAGE_MEDICAL) return hit(medical, 7, x, y);
    if (currentPage == PAGE_ADMIN) return hit(admin, 13, x, y);
    if (currentPage == PAGE_PATIENT_AUTH) return hit(patientAuth, 3, x, y);
    if (currentPage == PAGE_DOCTOR_AUTH) return hit(doctorAuth, 3, x, y);
    return hit(result, 5, x, y);
}

static void handleAction(int action)
{
    if (action < 0) return;
    if (action == 0) {
        if (currentPage == PAGE_ADMIN && adminLoggedIn) {
            writeAdminLog("管理员退出", "管理端", "成功");
            adminLoggedIn = 0;
            clearText(currentAdminName, sizeof(currentAdminName));
        }
        currentPage = PAGE_HOME;
    }
    else if (action == 1) currentPage = loggedPatientId[0] ? PAGE_PATIENT : PAGE_PATIENT_AUTH;
    else if (action == 2) currentPage = loggedDoctorId[0] ? PAGE_MEDICAL : PAGE_DOCTOR_AUTH;
    else if (action == 3) {
        if (adminLoggedIn) currentPage = PAGE_ADMIN;
        else doAdminLogin();
    }
    else if (action == 10) doAddPatient();
    else if (action == 11) doRegister();
    else if (action == 12) doPatientProfile();
    else if (action == 13) doMyBills();
    else if (action == 14) doPrescriptionDetail();
    else if (action == 15) doPatientPayFees();
    else if (action == 16) {
        clearText(loggedPatientId, sizeof(loggedPatientId));
        currentPage = PAGE_PATIENT_AUTH;
    }
    else if (action == 20) doVisit();
    else if (action == 21) doCheck();
    else if (action == 22) doPrescription();
    else if (action == 23) doSendPrescription();
    else if (action == 24) doAdmit();
    else if (action == 25) doLeave();
    else if (action == 30) managePatients();
    else if (action == 31) manageDoctors();
    else if (action == 32) manageDepartments();
    else if (action == 33) manageMedicines();
    else if (action == 34) manageWardBed();
    else if (action == 35) showReports();
    else if (action == 36) doRunTests();
    else if (action == 37) showWarningCenter();
    else if (action == 38) showAdminLogs();
    else if (action == 39) manageBackupRestore();
    else if (action == 40) doPatientLogin();
    else if (action == 41) doPatientRegister();
    else if (action == 42) doDoctorLogin();
    else if (action == 43) doDoctorRegister();
    else if (action == 90) doLoad();
    else if (action == 91) doSave();
    else if (action == 99) {
        closegraph();
        exit(0);
    } else if (action == 100) {
        currentPage = lastPage;
    } else if (action == 101) {
        resultPageNo = 0;
    } else if (action == 102) {
        if (resultPageNo > 0) resultPageNo--;
    } else if (action == 103) {
        int totalLine = countResultLines();
        int totalPage = totalLine == 0 ? 1 : (totalLine + 9) / 10;
        if (resultPageNo < totalPage - 1) resultPageNo++;
    } else if (action == 104) {
        int totalLine = countResultLines();
        int totalPage = totalLine == 0 ? 1 : (totalLine + 9) / 10;
        resultPageNo = totalPage - 1;
    }
}

void runGui(void)
{
    SetProcessDPIAware();
    initgraph(WIN_W, WIN_H);
    setbkcolor(RGB(248, 250, 252));
    loadAllData("data", resultText, sizeof(resultText));
    loadAuthData("data\\account.txt");
    checkAutoBackup();
    drawPage();
    while (1) {
        checkAutoBackup();
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                handleAction(actionAt(msg.x, msg.y));
                drawPage();
            }
        }
        Sleep(10);
    }
}



