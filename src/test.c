#include "test.h"
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
#include "file.h"
#include "report.h"

static int testNo = 1;

static void appendTest(char *out,int outSize,const char *name,const char *input,
    const char *expect,const char *actual,int pass,const char *level)
{
    char line[LINE_LEN * 2];
    snprintf(line,sizeof(line),"T%03d | %s | 输入：%s | 预期：%s | 实际：%s | %s | 难度：%s\n",
        testNo,name,input,expect,actual,pass ? "通过" : "失败",level);
    appendText(out,outSize,line);
    testNo++;
}

static void makeTestBase(void)
{
    clearAllData();
    addDepartment("DEP0001","内科","常见内科疾病","普通病房");
    addDepartment("DEP0002","外科","普通外科疾病","普通病房");
    addDoctor("DOC0001","王医生","DEP0001","主治医师","周一上午",50);
    addDoctor("DOC0002","李医生","DEP0002","副主任医师","周二上午",40);
    addPatient("PAT0001","张三","男",20,"13800000001","110101200001010011","门诊");
    addPatient("PAT0002","张三","男",30,"13800000002","110101199601010022","门诊");
    addPatient("PAT0003","李四","女",25,"13800000003","110101199901010033","门诊");
    addMedicine("MED0001","阿莫西林","阿莫西林","再林","消炎药","DEP0001",3.50,100,20);
    addMedicine("MED0002","布洛芬","布洛芬","芬必得","止痛药","DEP0001",2.00,80,10);
    addWard("WAR0001","普通病房","DEP0001",120.00,2,2);
    addBed("BED0001","WAR0001","空闲","","");
    addBed("BED0002","WAR0001","空闲","","");
}

void runInputTests(char *out,int outSize)
{
    double money;
    int age;
    clearText(out,outSize);
    testNo = 1;
    appendLine(out,outSize,"输入校验测试");
    appendTest(out,outSize,"空姓名","空字符串","拒绝",checkName("") ? "通过了" : "拒绝了",!checkName(""),"简单");
    appendTest(out,outSize,"姓名含非法字符","张|三","拒绝",checkName("张|三") ? "通过了" : "拒绝了",!checkName("张|三"),"简单");
    appendTest(out,outSize,"年龄下界","0","通过",checkAge(0) ? "通过了" : "拒绝了",checkAge(0),"简单");
    appendTest(out,outSize,"年龄越界","121","拒绝",checkAge(121) ? "通过了" : "拒绝了",!checkAge(121),"简单");
    appendTest(out,outSize,"电话位数错误","138001","拒绝",checkPhone("138001") ? "通过了" : "拒绝了",!checkPhone("138001"),"简单");
    appendTest(out,outSize,"身份证格式","11010120000101001X","通过",checkCardId("11010120000101001X") ? "通过了" : "拒绝了",checkCardId("11010120000101001X"),"中等");
    appendTest(out,outSize,"非闰年日期","2025-02-29","拒绝",checkDate("2025-02-29") ? "通过了" : "拒绝了",!checkDate("2025-02-29"),"中等");
    appendTest(out,outSize,"闰年日期","2024-02-29","通过",checkDate("2024-02-29") ? "通过了" : "拒绝了",checkDate("2024-02-29"),"中等");
    appendTest(out,outSize,"金额小数位","12.345","拒绝",
        parseMoneyStrict("12.345",&money) ? "通过了" : "拒绝了",!parseMoneyStrict("12.345",&money),"中等");
    appendTest(out,outSize,"整数严格解析","12a","拒绝",
        parseIntStrict("12a",&age) ? "通过了" : "拒绝了",!parseIntStrict("12a",&age),"简单");
}

void runFlowTests(char *out,int outSize)
{
    char id[ID_LEN];
    char payId[ID_LEN];
    char rxId[ID_LEN];
    char recId[ID_LEN];
    char visitRecId[ID_LEN];
    char checkNote[TEXT_LEN];
    Bed *bed;
    Ward *ward;
    MedicalRecord *latestVisit;
    Prescription *rxCheck;
    int code;
    int beforeStock;
    int dayCount;
    Patient *sameNameList[5];
    int sameNameCount;

    clearText(out,outSize);
    testNo = 1;
    appendLine(out,outSize,"业务流程测试");
    makeTestBase();
    clearText(visitRecId, sizeof(visitRecId));

    makeNextPatientId(id,ID_LEN);
    appendTest(out,outSize,"自动编号","已有 PAT0001-PAT0003","生成 PAT0004",
        id,strcmp(id,"PAT0004") == 0,"简单");

    sameNameCount = collectPatientsByName("张三",sameNameList,5);
    appendTest(out,outSize,"重名患者查询","张三","查到2人",
        sameNameCount == 2 ? "查到2人" : "数量不对",sameNameCount == 2,"中等");

    makeNextRecordId(recId,ID_LEN);
    makeNextPaymentId(payId,ID_LEN);
    addPayment(payId,"PAT0001","挂号",recId,10.00,"未缴费","2026-05-11","挂号费");
    payPayment(payId,"2026-05-11");
    code = isSourcePaid("挂号",recId) ?
        addMedicalRecord(recId,"PAT0001","DOC0001","DEP0001","挂号","2026-05-11",10.00,payId,"普通门诊",1) :
        ERR_INPUT;
    appendTest(out,outSize,"挂号先缴费","挂号费10元","缴费后生成挂号记录",
        codeText(code),code == OK,"中等");
    appendTest(out,outSize,"当日同科室挂号查重","PAT0001+内科+2026-05-11",
        "应判定已挂号",hasPatientDeptRegisterOnDate("PAT0001","DEP0001","2026-05-11") ? "已挂过" : "未挂过",
        hasPatientDeptRegisterOnDate("PAT0001","DEP0001","2026-05-11"),"简单");
    appendTest(out,outSize,"医生挂号人次","DOC0001","统计为1",
        countRegisterRecordsByDoctor("DOC0001") == 1 ? "为1" : "不对",
        countRegisterRecordsByDoctor("DOC0001") == 1,"简单");

    makeNextRecordId(recId,ID_LEN);
    code = addMedicalRecord(recId,"PAT0001","DOC0001","DEP0001","看诊","2026-05-11",0,"","诊断：感冒",1);
    safeCopy(visitRecId, ID_LEN, recId);
    appendTest(out,outSize,"看诊记录","张三看诊","生成看诊记录",codeText(code),code == OK,"简单");
    latestVisit = findLatestVisitByPatient("PAT0001");
    appendTest(out,outSize,"接诊编号刷新","接诊后查询当前看诊记录","应取到刚生成的REC编号",
        latestVisit != NULL ? latestVisit->id : "未找到",
        latestVisit != NULL && strcmp(latestVisit->id, visitRecId) == 0,"中等");

    makeNextRecordId(recId,ID_LEN);
    makeNextPaymentId(payId,ID_LEN);
    snprintf(checkNote, sizeof(checkNote), "DEP0001%cDOC0001%c血常规", '\x1e', '\x1e');
    addPayment(payId,"PAT0001","检查",recId,35.00,"未缴费","2026-05-11",checkNote);
    code = payPayment(payId,"2026-05-11");
    if (code == OK) code = addMedicalRecord(recId,"PAT0001","DOC0001","DEP0001","检查","2026-05-11",35.00,payId,"血常规",1);
    appendTest(out,outSize,"检验检查","血常规35元","缴费并登记后生成检查记录",codeText(code),code == OK,"中等");

    makeNextPrescriptionId(rxId,ID_LEN);
    addPrescription(rxId,"PAT0001","DOC0001","DEP0001","2026-05-11",visitRecId,"");
    rxCheck = findPrescriptionById(rxId);
    appendTest(out,outSize,"处方沿用接诊编号","接诊后开处方","处方medicalRecordId等于看诊REC",
        rxCheck != NULL ? rxCheck->medicalRecordId : "未找到处方",
        rxCheck != NULL && strcmp(rxCheck->medicalRecordId, visitRecId) == 0,"中等");
    addPrescriptionItem(rxId,"MED0001",2);
    addPrescriptionItem(rxId,"MED0002",3);
    makeNextPaymentId(payId,ID_LEN);
    addPayment(payId,"PAT0001","处方",rxId,findPrescriptionById(rxId)->totalFee,"未缴费","2026-05-11","多药处方");
    code = payPayment(payId,"2026-05-11");
    if (code == OK) code = setPrescriptionPaid(rxId);
    beforeStock = findMedicineById("MED0001")->stock;
    if (code == OK) code = sendPrescription(rxId);
    appendTest(out,outSize,"多药处方发药","2种药","缴费后扣库存",
        code == OK && findMedicineById("MED0001")->stock == beforeStock - 2 ? "库存正确减少" : "库存不正确",
        code == OK && findMedicineById("MED0001")->stock == beforeStock - 2,"较难");
    rxCheck = findPrescriptionById(rxId);
    appendTest(out,outSize,"发药沿用接诊编号","处方发药后核对编号","发药界面仍显示同一看诊REC",
        rxCheck != NULL ? rxCheck->medicalRecordId : "未找到处方",
        rxCheck != NULL && strcmp(rxCheck->medicalRecordId, visitRecId) == 0,"中等");

    bed = findFreeBed("DEP0001","普通病房");
    code = bed ? occupyBed(bed->id,"PAT0001","2026-05-11") : ERR_NO_SPACE;
    appendTest(out,outSize,"办理入院","普通病房","占用一个床位",codeText(code),code == OK,"中等");

    bed = findBedByPatient("PAT0001");
    ward = bed ? findWardById(bed->wardId) : NULL;
    dayCount = daysBetween("2026-05-11","2026-05-14");
    makeNextPaymentId(payId,ID_LEN);
    code = addPayment(payId,"PAT0001","住院",bed ? bed->id : "BED0001",ward ? ward->dailyFee * dayCount : 0,
        "未缴费","2026-05-14","出院结算");
    if (code == OK) code = payPayment(payId,"2026-05-14");
    if (code == OK) {
        makeNextRecordId(recId,ID_LEN);
        code = addMedicalRecord(recId,"PAT0001","","DEP0001","出院","2026-05-14",
            ward ? ward->dailyFee * dayCount : 0,payId,"出院结算并释放床位",1);
    }
    if (code == OK) code = releasePatientBed("PAT0001");
    appendTest(out,outSize,"出院结算","住院3天","缴费后释放床位",
        code == OK && findBedByPatient("PAT0001") == NULL ? "床位已释放" : "释放失败",
        code == OK && findBedByPatient("PAT0001") == NULL,"较难");

    code = deletePatient("PAT0001");
    appendTest(out,outSize,"关联删除限制","删除已有记录患者","拒绝删除",
        codeText(code),code == ERR_LINKED,"中等");
}

void runFileTests(char *out,int outSize)
{
    char msg[TEXT_LEN];
    int code1,code2;
    clearText(out,outSize);
    testNo = 1;
    appendLine(out,outSize,"文件导入测试");
    code1 = checkDataFile("testdata\\patient_30_ok.txt",7,msg,sizeof(msg));
    appendTest(out,outSize,"正常文件字段","patient_30_ok.txt","通过",msg,code1 == OK,"简单");
    code2 = checkDataFile("testdata\\patient_bad_field.txt",7,msg,sizeof(msg));
    appendTest(out,outSize,"字段数量错误","patient_bad_field.txt","拒绝整文件",msg,code2 != OK,"中等");
}

void runReportTests(char *out,int outSize)
{
    char temp[RESULT_LEN];
    clearText(out,outSize);
    testNo = 1;
    appendLine(out,outSize,"统计报表测试");
    makeTestBase();
    addPayment("PAY0001","PAT0001","挂号","REC0001",10.00,"已缴费","2026-05-11","挂号费");
    reportIncome(temp,sizeof(temp));
    appendTest(out,outSize,"收入统计","一笔挂号费","总收入包含10元",
        strstr(temp,"10.00") != NULL ? "包含10.00" : "没有统计到",strstr(temp,"10.00") != NULL,"简单");
    reportBedUse(temp,sizeof(temp));
    appendTest(out,outSize,"床位统计","2张空床","显示床位使用率",
        strstr(temp,"使用率") != NULL ? "有使用率" : "没有使用率",strstr(temp,"使用率") != NULL,"简单");
    reportMedicineStock(temp,sizeof(temp));
    appendTest(out,outSize,"库存统计","药品库存","显示库存状态",
        strstr(temp,"正常") != NULL ? "有库存状态" : "没有库存状态",strstr(temp,"正常") != NULL,"简单");
}

void runAllTests(char *out,int outSize)
{
    char part[RESULT_LEN];
    clearText(out,outSize);
    appendLine(out,outSize,"系统全部测试");
    appendLine(out,outSize,"答辩注意：测试入口集中展示输入校验、流程和边界，说明测试覆盖面即可（难度：中等）");
    runInputTests(part,sizeof(part));
    appendText(out,outSize,part);
    appendLine(out,outSize,"");
    runFlowTests(part,sizeof(part));
    appendText(out,outSize,part);
    appendLine(out,outSize,"");
    runFileTests(part,sizeof(part));
    appendText(out,outSize,part);
    appendLine(out,outSize,"");
    runReportTests(part,sizeof(part));
    appendText(out,outSize,part);
}
