from pathlib import Path

root = Path(__file__).resolve().parents[1]
data = root / "data"
testdata = root / "testdata"
data.mkdir(exist_ok=True)
testdata.mkdir(exist_ok=True)


def write(name, lines, folder=data):
    (folder / name).write_text("\n".join(lines) + "\n", encoding="utf-8")


departments = [
    "DEP0001|内科|常见内科疾病|普通病房",
    "DEP0002|外科|普通外科疾病|普通病房",
    "DEP0003|儿科|儿童常见疾病|儿科病房",
    "DEP0004|骨科|骨伤和关节疾病|骨科病房",
    "DEP0005|皮肤科|皮肤常见疾病|普通病房",
]
write("department.txt", departments)

titles = ["住院医师", "主治医师", "副主任医师", "主任医师"]
doctors = []
for i in range(1, 21):
    dept = (i - 1) % 5 + 1
    doctors.append(f"DOC{i:04d}|医生{i:03d}|DEP{dept:04d}|{titles[(i - 1) % 4]}|周{(i - 1) % 5 + 1}上午|60")
write("doctor.txt", doctors)

patients = []
for i in range(1, 131):
    if i in (1, 2):
        name = "张三"
    elif i == 3:
        name = "李四"
    else:
        name = f"患者{i:03d}"
    gender = "男" if i % 2 == 0 else "女"
    age = 18 + i % 70
    phone = f"138{i:08d}"
    year = 1980 + i % 30
    card = f"110101{year:04d}0101{i:04d}"
    status = "门诊" if i <= 100 else "住院"
    patients.append(f"PAT{i:04d}|{name}|{gender}|{age}|{phone}|{card}|{status}")
write("patient.txt", patients)
write("patient_30_ok.txt", patients[:30], testdata)
write(
    "patient_bad_field.txt",
    [
        "PAT9001|错误患者|男|20|13899990001|110101200001019001",
        "PAT9002|正常患者|女|22|13899990002|110101200001019002|门诊",
    ],
    testdata,
)

medicines = []
for i in range(1, 21):
    dept = (i - 1) % 5 + 1
    price = 2.5 + i
    stock = 100 + i
    medicines.append(
        f"MED{i:04d}|药品{i:03d}|通用{i:03d}|商品{i:03d}|别名{i:03d}|DEP{dept:04d}|{price:.2f}|{stock}|10"
    )
write("medicine.txt", medicines)

wards = [
    "WAR0001|普通病房|DEP0001|120.00|20|10",
    "WAR0002|普通病房|DEP0002|150.00|20|12",
    "WAR0003|儿科病房|DEP0003|100.00|15|10",
    "WAR0004|骨科病房|DEP0004|180.00|15|10",
    "WAR0005|普通病房|DEP0005|110.00|10|8",
]
write("ward.txt", wards)

beds = []
bed_no = 1
patient_no = 101
ward_info = [
    ("WAR0001", 20, 10),
    ("WAR0002", 20, 8),
    ("WAR0003", 15, 5),
    ("WAR0004", 15, 5),
    ("WAR0005", 10, 2),
]
for ward_id, total, occupied in ward_info:
    for i in range(1, total + 1):
        if i <= occupied:
            beds.append(f"BED{bed_no:04d}|{ward_id}|占用|PAT{patient_no:04d}|2026-05-01")
            patient_no += 1
        else:
            beds.append(f"BED{bed_no:04d}|{ward_id}|空闲||")
        bed_no += 1
write("bed.txt", beds)

records = []
payments = []
for i in range(1, 101):
    doc = (i - 1) % 20 + 1
    dept = (doc - 1) % 5 + 1
    day = (i - 1) % 28 + 1
    records.append(f"REC{i:04d}|PAT{i:04d}|DOC{doc:04d}|DEP{dept:04d}|挂号|2026-05-{day:02d}|10.00|PAY{i:04d}|门诊挂号")
    payments.append(f"PAY{i:04d}|PAT{i:04d}|挂号|REC{i:04d}|10.00|已缴费|2026-05-{day:02d}|挂号费")

for i in range(101, 131):
    bed = i - 100
    if bed <= 10:
        dept = 1
    elif bed <= 18:
        dept = 2
    elif bed <= 23:
        dept = 3
    elif bed <= 28:
        dept = 4
    else:
        dept = 5
    records.append(f"REC{i:04d}|PAT{i:04d}||DEP{dept:04d}|入院|2026-05-01|0.00|BED{bed:04d}|样例住院患者")

prescriptions = []
items = []
for i in range(1, 11):
    doc = (i - 1) % 20 + 1
    dept = (doc - 1) % 5 + 1
    med1 = (i - 1) % 20 + 1
    med2 = i % 20 + 1
    price1 = 2.5 + med1
    price2 = 2.5 + med2
    total = price1 * 2 + price2
    prescriptions.append(f"RX{i:04d}|PAT{i:04d}|DOC{doc:04d}|DEP{dept:04d}|2026-05-10|2|{total:.2f}|未缴费")
    items.append(f"RX{i:04d}|MED{med1:04d}|2|{price1:.2f}")
    items.append(f"RX{i:04d}|MED{med2:04d}|1|{price2:.2f}")
    pay_no = 100 + i
    payments.append(f"PAY{pay_no:04d}|PAT{i:04d}|处方|RX{i:04d}|{total:.2f}|未缴费|2026-05-10|处方药费")

write("record.txt", records)
write("prescription.txt", prescriptions)
write("prescription_item.txt", items)
write("payment.txt", payments)

print("sample data generated")
