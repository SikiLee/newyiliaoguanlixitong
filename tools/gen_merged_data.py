# -*- coding: utf-8 -*-
"""Generate doctor.txt: 110 rows (Chen schedule) with real Chinese names (Ran style)."""
import os
import shutil

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAN_DATA = os.path.join(os.path.dirname(BASE), "冉", "data")
CHEN_DATA = os.path.join(BASE, "data")

NAMES = [
    "李晓明", "王建国", "张丽华", "刘志强", "陈静", "赵文博", "孙佳怡", "周伟东", "吴海燕", "郑凯",
    "黄晓霞", "林国华", "杨雪梅", "徐明辉", "马秀英", "朱健康", "胡晓峰", "高峰", "曾雪琴", "范志远",
    "何俊杰", "罗婷婷", "梁志刚", "宋雨萱", "唐国强", "韩雪", "冯建军", "曹敏", "彭磊", "董文静",
    "袁志鹏", "蒋丽华", "蔡伟", "潘晓红", "杜明", "丁磊", "魏东", "薛芳", "叶青", "阎磊",
    "余静", "汪洋", "石磊", "金鑫", "陆婷", "孔祥", "白洁", "崔勇", "康宁", "毛敏",
    "邱伟", "秦芳", "江涛", "史强", "顾晓", "侯亮", "邵峰", "孟洁", "龙飞", "万丽",
    "段誉", "钱多多", "汤唯", "尹正", "黎明的", "易洋", "常远", "武强", "乔峰", "贺军",
    "赖敏", "龚涛", "文慧", "庞博", "樊登", "兰心", "殷实", "施耐", "洪涛", "翟明",
    "安平", "颜如玉", "倪萍", "严宽", "牛莉", "温峥嵘", "芦芳", "季晨", "皮埃尔", "卞京",
    "齐豫", "元芳", "卜凡", "顾城", "孟郊", "平措", "黄蓉", "和静", "穆桂英", "萧红",
    "尹志平", "姚贝娜", "邵逸夫", "湛卢", "汪曾祺", "祁同伟", "毛不易", "禹州", "狄更斯", "米雪",
]

TITLES = ["住院医师", "主治医师", "副主任医师", "主任医师"]
DEPTS = ["DEP0001", "DEP0002", "DEP0003", "DEP0004", "DEP0005"]
WEEK_SLOTS = [
    ("周1上午", 20), ("周2上午", 20), ("周3上午", 20), ("周4上午", 16),
    ("周5上午", 16), ("周6上午", 10), ("周7上午", 8),
]

def gen_doctors():
    lines = []
    idx = 0
    doc_num = 1
    for week, count in WEEK_SLOTS:
        for i in range(count):
            dept = DEPTS[i % 5]
            title = TITLES[i % 4]
            name = NAMES[idx % len(NAMES)]
            idx += 1
            lines.append(f"DOC{doc_num:04d}|{name}|{dept}|{title}|{week}|60")
            doc_num += 1
    assert len(lines) == 110
    with open(os.path.join(CHEN_DATA, "doctor.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")

def copy_ran_data():
    for fn in ("medicine.txt", "ward.txt", "bed.txt"):
        src = os.path.join(RAN_DATA, fn)
        dst = os.path.join(CHEN_DATA, fn)
        if os.path.isfile(src):
            shutil.copy2(src, dst)

if __name__ == "__main__":
    gen_doctors()
    copy_ran_data()
    print("doctor.txt (110), medicine.txt, ward.txt, bed.txt updated.")
