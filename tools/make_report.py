from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Image as RLImage,
    Table, TableStyle, PageBreak
)

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
DOCS.mkdir(exist_ok=True)

FONT = Path(r"C:\Windows\Fonts\simhei.ttf")
FONT_NAME = "SimHei"
pdfmetrics.registerFont(TTFont(FONT_NAME, str(FONT)))


def font(size, bold=False):
    return ImageFont.truetype(str(FONT), size=size)


def box(draw, xy, text, size=26, fill=(255, 255, 255), outline=(120, 165, 190), width=2):
    x1, y1, x2, y2 = xy
    draw.rounded_rectangle(xy, radius=6, fill=fill, outline=outline, width=width)
    f = font(size)
    bbox = draw.textbbox((0, 0), text, font=f)
    tx = x1 + (x2 - x1 - (bbox[2] - bbox[0])) / 2
    ty = y1 + (y2 - y1 - (bbox[3] - bbox[1])) / 2 - 2
    draw.text((tx, ty), text, fill=(55, 88, 110), font=f)


def line(draw, p1, p2, fill=(120, 165, 190), width=3):
    draw.line([p1, p2], fill=fill, width=width)


def make_overview():
    img = Image.new("RGB", (1500, 850), "white")
    d = ImageDraw.Draw(img)
    d.text((80, 365), "轻量级医疗管理系统", fill=(35, 55, 80), font=font(44))
    d.line((80, 430, 430, 430), fill=(35, 55, 80), width=5)
    line(d, (430, 430), (520, 430))
    line(d, (520, 190), (520, 670))

    branches = [
        ("患者端", 190, ["新患者信息录入", "挂号缴费", "个人信息查询", "医疗记录查询", "缴费记录查询"]),
        ("医护端", 430, ["接诊看诊", "检查缴费", "多药处方", "药房发药", "入院办理", "出院结算"]),
        ("管理端", 670, ["基础信息维护", "药品库存管理", "病房床位管理", "统计报表", "系统测试页"]),
    ]
    for title, y, items in branches:
        line(d, (520, y), (650, y))
        d.text((650, y - 22), title, fill=(35, 55, 80), font=font(30))
        line(d, (800, y), (900, y))
        line(d, (900, y - 105), (900, y + 120))
        for i, item in enumerate(items):
            yy = y - 105 + i * 45
            line(d, (900, yy + 20), (940, yy + 20))
            d.text((950, yy), item, fill=(55, 80, 105), font=font(24))
            d.line((950, yy + 34, 1180, yy + 34), fill=(120, 140, 160), width=2)

    path = DOCS / "系统概述图.png"
    img.save(path)
    return path


def make_flowchart():
    img = Image.new("RGB", (1500, 1100), "white")
    d = ImageDraw.Draw(img)
    d.text((610, 40), "就医全流程图", fill=(35, 55, 80), font=font(42))

    # patient lane
    box(d, (90, 160, 310, 225), "患者录入/查询")
    box(d, (90, 300, 310, 365), "选择科室医生")
    box(d, (90, 440, 310, 505), "挂号缴费")
    box(d, (90, 580, 310, 645), "查询个人记录")

    # medical lane
    box(d, (610, 160, 830, 225), "医生接诊")
    box(d, (610, 300, 830, 365), "看诊记录")
    box(d, (480, 440, 700, 505), "检查缴费")
    box(d, (740, 440, 960, 505), "多药处方缴费")
    box(d, (610, 580, 830, 645), "药房发药")
    box(d, (480, 720, 700, 785), "入院分配床位")
    box(d, (740, 720, 960, 785), "出院结算")

    # admin lane
    box(d, (1160, 160, 1400, 225), "基础信息维护")
    box(d, (1160, 300, 1400, 365), "药品库存管理")
    box(d, (1160, 440, 1400, 505), "病房床位管理")
    box(d, (1160, 580, 1400, 645), "统计报表")
    box(d, (1160, 720, 1400, 785), "系统测试页")

    # arrows and links
    line(d, (200, 225), (200, 300))
    line(d, (200, 365), (200, 440))
    line(d, (310, 472), (610, 192))
    line(d, (720, 225), (720, 300))
    line(d, (720, 365), (590, 440))
    line(d, (720, 365), (850, 440))
    line(d, (850, 505), (720, 580))
    line(d, (590, 505), (590, 720))
    line(d, (850, 645), (850, 720))
    line(d, (700, 752), (740, 752))
    line(d, (830, 612), (1160, 332))
    line(d, (590, 752), (1160, 472))
    line(d, (850, 752), (1160, 612))
    line(d, (200, 505), (200, 580))
    line(d, (200, 612), (1160, 612))

    d.text((90, 105), "患者端", fill=(35, 55, 80), font=font(30))
    d.text((610, 105), "医护端", fill=(35, 55, 80), font=font(30))
    d.text((1160, 105), "管理端", fill=(35, 55, 80), font=font(30))

    path = DOCS / "就医流程图.png"
    img.save(path)
    return path


def make_pdf(overview_path, flow_path):
    pdf_path = ROOT / "轻量级医疗管理系统实验报告.pdf"
    try:
        with open(pdf_path, "ab"):
            pass
    except PermissionError:
        pdf_path = ROOT / "轻量级医疗管理系统实验报告_修正版.pdf"
    doc = SimpleDocTemplate(
        str(pdf_path), pagesize=A4,
        rightMargin=1.6 * cm, leftMargin=1.6 * cm,
        topMargin=1.5 * cm, bottomMargin=1.5 * cm
    )
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name="CNTitle", fontName=FONT_NAME, fontSize=24, leading=32, alignment=1))
    styles.add(ParagraphStyle(name="CNHeading", fontName=FONT_NAME, fontSize=16, leading=24, spaceBefore=14, spaceAfter=8))
    styles.add(ParagraphStyle(name="CNBody", fontName=FONT_NAME, fontSize=10.5, leading=17, firstLineIndent=21))
    styles.add(ParagraphStyle(name="CNList", fontName=FONT_NAME, fontSize=10.5, leading=17))
    styles.add(ParagraphStyle(name="CNCell", fontName=FONT_NAME, fontSize=8.5, leading=13, wordWrap="CJK"))
    styles.add(ParagraphStyle(name="CNCellHead", fontName=FONT_NAME, fontSize=9, leading=13, wordWrap="CJK", textColor=colors.black))

    story = []
    P = lambda text, style="CNBody": Paragraph(text, styles[style])
    C = lambda text: Paragraph(text, styles["CNCell"])
    H = lambda text: Paragraph(text, styles["CNCellHead"])

    story.append(P("轻量级医疗管理系统实验报告", "CNTitle"))
    story.append(Spacer(1, 0.4 * cm))
    story.append(P("项目名称：轻量级医疗管理系统（Hospital Information System）", "CNList"))
    story.append(P("开发语言：C 语言 + EasyX 图形界面", "CNList"))
    story.append(P("数据结构：结构体 + 单向链表", "CNList"))
    story.append(P("数据保存：UTF-8 txt 文件", "CNList"))
    story.append(PageBreak())

    story.append(P("目录", "CNTitle"))
    toc = [
        "一、系统概述",
        "二、一些约定",
        "三、具体功能实现",
        "四、流程图展示",
        "五、图形化界面相关",
        "六、测试样例与结果",
        "七、一些防止错误操作的措施及效果",
    ]
    for item in toc:
        story.append(P(item + " ................................................................", "CNHeading"))
    story.append(PageBreak())

    story.append(P("一、系统概述", "CNHeading"))
    story.append(P("本系统面向小型医院门诊、医护处理、住院床位、药房药品和缴费统计等基础场景。系统按患者端、医护端、管理端划分功能，用户在界面中通过按钮逐页操作，尽量避免直接输入内部编号。"))
    story.append(P("系统后台使用多个单向链表保存患者、医生、科室、药品、病房、床位、医疗记录、处方和缴费记录。数据通过 txt 文件保存，程序启动后读取数据，操作完成后可以保存。"))
    story.append(RLImage(str(overview_path), width=16.5 * cm, height=9.35 * cm))

    story.append(P("二、一些约定", "CNHeading"))
    rules = [
        "编号由系统自动生成，例如患者编号 PAT0001，医疗记录编号 REC0001，缴费编号 PAY0001。",
        "患者查询和办理业务主要输入姓名；出现重名时显示身份证号和电话，再输入序号选择。",
        "日期统一使用 YYYY-MM-DD 格式；金额限制为 0-999999.99；数量必须是正整数。",
        "基础数据已有业务关联时不允许删除，避免历史记录找不到对应对象。",
        "文件字段之间使用竖线分隔，所以输入内容不能包含 | 字符。",
    ]
    for rule in rules:
        story.append(P("• " + rule, "CNList"))

    story.append(P("三、具体功能实现", "CNHeading"))
    data = [
        [H("端口"), H("主要功能"), H("实现说明")],
        [C("患者端"), C("新患者录入、挂号缴费、个人信息查询、医疗记录查询、缴费记录查询"), C("患者重名时通过身份证号和电话区分；挂号时先选择科室，再显示该科室医生。")],
        [C("医护端"), C("接诊、检查、开多药处方、药房发药、入院、出院结算"), C("检查和处方生成缴费记录；发药前检查库存；出院按住院天数计算费用。")],
        [C("管理端"), C("患者、医生、科室、药品、病房床位维护，统计报表，系统测试页"), C("支持基础信息维护和查询统计；删除前检查医疗记录、处方、缴费、床位等关联记录。")],
    ]
    table = Table(data, colWidths=[2.0 * cm, 6.4 * cm, 8.0 * cm], repeatRows=1)
    table.setStyle(TableStyle([
        ("FONTNAME", (0, 0), (-1, -1), FONT_NAME),
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#DDEAF3")),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
    ]))
    story.append(table)

    story.append(P("四、流程图展示", "CNHeading"))
    story.append(P("系统主要流程为：患者录入或查询信息，选择科室和医生后挂号缴费；医生完成接诊后，可根据情况开检查、开处方或办理住院；药房在处方缴费后发药并扣减库存；出院时根据住院天数生成费用并释放床位。"))
    story.append(RLImage(str(flow_path), width=16.5 * cm, height=12.1 * cm))

    story.append(P("五、图形化界面相关", "CNHeading"))
    story.append(P("图形界面使用 EasyX 实现，采用一页一页的操作方式。首页进入患者端、医护端、管理端；每个端内部用大按钮进入具体功能；结果页提供分页按钮，方便展示长列表和测试结果。"))
    story.append(P("输入框标题中写明输入限制，例如姓名长度、日期格式、金额范围、身份证号格式等。后台函数也会再次校验，避免错误数据进入链表。"))

    story.append(P("六、测试样例与结果", "CNHeading"))
    tests = [
        [H("测试类型"), H("测试内容"), H("预期结果")],
        [C("输入校验"), C("空姓名、非法字符、错误电话、错误身份证、非法日期、负金额"), C("系统拒绝输入并提示原因")],
        [C("业务流程"), C("挂号缴费、看诊、检查缴费、多药处方、发药、入院、出院"), C("记录生成正确，库存和床位状态更新正确")],
        [C("文件导入"), C("字段缺失、字段过多、重复编号、关联不存在"), C("拒绝错误文件并显示错误行号")],
        [C("统计报表"), C("收入统计、床位使用、药品库存、医生/科室工作量"), C("统计结果能够正常显示")],
    ]
    t2 = Table(tests, colWidths=[3 * cm, 7 * cm, 6 * cm], repeatRows=1)
    t2.setStyle(TableStyle([
        ("FONTNAME", (0, 0), (-1, -1), FONT_NAME),
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#DDEAF3")),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
    ]))
    story.append(t2)

    story.append(P("七、一些防止错误操作的措施及效果", "CNHeading"))
    measures = [
        "使用列表选择科室、医生、药品和床位，减少用户手动输入编号造成的错误。",
        "患者重名时显示身份证号和电话，避免把记录加到错误患者名下。",
        "发药前先检查全部药品库存，库存不足时不扣减任何药品。",
        "住院时占用床位并减少空床数，出院缴费后释放床位并恢复空床数。",
        "删除基础信息前检查是否已有业务记录，防止历史数据断开。",
        "系统测试页集中展示边界测试，便于答辩时说明系统可靠性。",
    ]
    for item in measures:
        story.append(P("• " + item, "CNList"))

    doc.build(story)
    return pdf_path


if __name__ == "__main__":
    overview = make_overview()
    flow = make_flowchart()
    pdf = make_pdf(overview, flow)
    print(overview)
    print(flow)
    print(pdf)
