#include "codeeditordialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QUuid>
#include <QStringList>
#include <QRegularExpression>
#include <QFont>
#include <QFontMetrics>
#include <QTextCharFormat>
#include <QSyntaxHighlighter>

// ─── Simple syntax highlighter ───────────────────────────────────────────────
class ScriptHighlighter : public QSyntaxHighlighter {
public:
    ScriptHighlighter(QTextDocument *doc) : QSyntaxHighlighter(doc) {}
protected:
    void highlightBlock(const QString &text) override {
        // Keywords
        QTextCharFormat kwFmt; kwFmt.setForeground(QColor(86,156,214));
        QTextCharFormat strFmt; strFmt.setForeground(QColor(206,145,120));
        QTextCharFormat numFmt; numFmt.setForeground(QColor(181,206,168));
        QTextCharFormat cmtFmt; cmtFmt.setForeground(QColor(106,153,85));
        QTextCharFormat fnFmt;  fnFmt.setForeground(QColor(220,220,170));

        static QStringList keywords = {"figure","node","bone","frame","pose",
                                       "anim_fps","anim_loop","background"};
        for (const QString &kw : keywords) {
            QRegularExpression re("\\b"+kw+"\\b");
            auto it = re.globalMatch(text);
            while (it.hasNext()) {
                auto m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), kwFmt);
            }
        }
        // Strings
        QRegularExpression str("'[^']*'|\"[^\"]*\"");
        auto it2 = str.globalMatch(text);
        while (it2.hasNext()) { auto m=it2.next(); setFormat(m.capturedStart(),m.capturedLength(),strFmt); }
        // Numbers
        QRegularExpression num("-?\\b\\d+\\.?\\d*\\b");
        auto it3 = num.globalMatch(text);
        while (it3.hasNext()) { auto m=it3.next(); setFormat(m.capturedStart(),m.capturedLength(),numFmt); }
        // Comments
        int ci = text.indexOf("//");
        if (ci >= 0) setFormat(ci, text.length()-ci, cmtFmt);
        // Hash comments
        ci = text.indexOf('#');
        if (ci >= 0) setFormat(ci, text.length()-ci, cmtFmt);
    }
};

// ─── Dialog ──────────────────────────────────────────────────────────────────
CodeEditorDialog::CodeEditorDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("محرر الكود — Pivot Script Language");
    setMinimumSize(720, 520);
    resize(800, 580);

    auto *vl = new QVBoxLayout(this);
    vl->setContentsMargins(8,8,8,8);
    vl->setSpacing(6);

    // Toolbar
    auto *tl = new QHBoxLayout;
    auto *runBtn    = new QPushButton("▶ تشغيل الكود");
    auto *sampleBtn = new QPushButton("مثال: رجل يمشي");
    auto *sample2   = new QPushButton("مثال: تشويق");
    auto *helpBtn   = new QPushButton("? مرجع API");
    auto *closeBtn  = new QPushButton("✕ إغلاق");
    runBtn->setStyleSheet("background:#185FA5;color:white;font-weight:bold;padding:6px 16px;border-radius:5px;border:none;");
    tl->addWidget(runBtn); tl->addWidget(sampleBtn); tl->addWidget(sample2);
    tl->addStretch(); tl->addWidget(helpBtn); tl->addWidget(closeBtn);
    vl->addLayout(tl);

    // Editor
    m_editor = new QTextEdit;
    QFont mono("Monospace", 12);
    mono.setStyleHint(QFont::Monospace);
    m_editor->setFont(mono);
    m_editor->setTabStopDistance(QFontMetrics(mono).horizontalAdvance(' ') * 4);
    m_editor->setLineWrapMode(QTextEdit::NoWrap);
    new ScriptHighlighter(m_editor->document());
    m_editor->setPlaceholderText(
        "// Pivot Script Language\n"
        "// الدوال المتاحة:\n"
        "// figure 'اسم'           — شخصية جديدة\n"
        "// node id x y size color — مفصل\n"
        "// bone from to width color — عظمة\n"
        "// frame duration         — إطار جديد (مدة بالمللي ثانية)\n"
        "// pose nodeId x y        — تحريك مفصل في الإطار الحالي\n"
        "// anim_fps 24            — سرعة التشغيل\n"
        "// anim_loop loop         — loop | once | pingpong\n"
        "// background #1e1e28     — لون الخلفية\n"
    );
    vl->addWidget(m_editor, 1);

    // Status
    m_status = new QLabel("اكتب الكود واضغط تشغيل");
    m_status->setStyleSheet("color:#9090aa;font-size:11px;padding:3px 6px;background:#16161e;border-radius:4px;");
    vl->addWidget(m_status);

    connect(runBtn,    &QPushButton::clicked, this, &CodeEditorDialog::runCode);
    connect(sampleBtn, &QPushButton::clicked, this, &CodeEditorDialog::loadSample);
    connect(closeBtn,  &QPushButton::clicked, this, &QDialog::close);
    connect(sample2,   &QPushButton::clicked, this, [this](){
        m_editor->setText(
"# مثال: تشويق متقدم\n"
"background #0d0d1a\n"
"anim_fps 18\n"
"anim_loop pingpong\n\n"
"figure 'fighter'\n"
"node head   0 -120 14 #378ADD\n"
"node neck   0  -93  7 #5DCAA5\n"
"node body   0  -20  8 #5DCAA5\n"
"node ls   -30  -75  7 #888780\n"
"node rs    30  -75  7 #888780\n"
"node le   -52  -40  6 #888780\n"
"node re    52  -40  6 #888780\n"
"node lh   -58   -8  5 #888780\n"
"node rh    58   -8  5 #888780\n"
"node hip    0  -20  8 #5DCAA5\n"
"node lk   -22   32  6 #888780\n"
"node rk    22   32  6 #888780\n"
"node lf   -27   85  6 #444441\n"
"node rf    27   85  6 #444441\n"
"bone head neck   4 #888780\n"
"bone neck body   6 #888780\n"
"bone neck ls     5 #888780\n"
"bone neck rs     5 #888780\n"
"bone ls le       4 #B4B2A9\n"
"bone rs re       4 #B4B2A9\n"
"bone le lh       3 #B4B2A9\n"
"bone re rh       3 #B4B2A9\n"
"bone body hip    6 #888780\n"
"bone hip lk      5 #B4B2A9\n"
"bone hip rk      5 #B4B2A9\n"
"bone lk lf       4 #B4B2A9\n"
"bone rk rf       4 #B4B2A9\n\n"
"# إطار 1 — وضع البداية\n"
"frame 100\n"
"pose le -70 -20\n"
"pose lh -90  15\n"
"pose re  35 -55\n"
"pose rh  30 -80\n"
"pose lk -40  20\n"
"pose lf -50  80\n\n"
"# إطار 2 — ضربة يد يسار\n"
"frame 80\n"
"pose le -90  -60\n"
"pose lh -110 -80\n"
"pose re   45  -30\n"
"pose rh   50   10\n\n"
"# إطار 3 — ارتداد\n"
"frame 120\n"
"pose le -52 -40\n"
"pose lh -58  -8\n"
"pose re  52 -40\n"
"pose rh  58  -8\n"
        );
    });
    connect(helpBtn, &QPushButton::clicked, this, [this](){
        m_editor->setText(
"# ═══════════════════════════════════════\n"
"# Pivot Script Language — مرجع كامل\n"
"# ═══════════════════════════════════════\n\n"
"# ── إعدادات عامة ──────────────────────\n"
"background #1e1e28    # لون خلفية الكانفس\n"
"anim_fps 24           # سرعة (1-120)\n"
"anim_loop loop        # loop | once | pingpong\n\n"
"# ── شخصية ─────────────────────────────\n"
"figure 'اسم الشخصية'\n\n"
"# ── مفصل ──────────────────────────────\n"
"node <id> <x> <y> [size=8] [color=#378ADD]\n"
"# id: نص بدون مسافات\n"
"# x,y: الموضع (0,0 = المركز)\n"
"# size: نصف القطر بالبكسل\n"
"# color: لون hex\n\n"
"# ── عظمة ───────────────────────────────\n"
"bone <from_id> <to_id> [width=4] [color=#888780]\n\n"
"# ── إطار ───────────────────────────────\n"
"frame [duration=100]   # مدة بالمللي ثانية\n"
"# يحفظ وضعية الشخصية الحالية كإطار\n\n"
"# ── وضعية ─────────────────────────────\n"
"pose <node_id> <x> <y>\n"
"# يغير موضع مفصل في آخر frame أضفته\n"
"# يمكن كتابة عدة pose تحت نفس frame\n\n"
"# ── مثال بسيط ─────────────────────────\n"
"figure 'ball'\n"
"node c 0 0 20 #378ADD\n"
"frame 100\n"
"pose c 0 -50\n"
"frame 100\n"
"pose c 0  50\n"
"frame 100\n"
"pose c 0   0\n"
        );
    });
}

QString CodeEditorDialog::uniqueId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

void CodeEditorDialog::loadSample()
{
    m_editor->setText(
"# رجل يمشي — مثال كامل\n"
"background #1a1a2a\n"
"anim_fps 12\n"
"anim_loop loop\n\n"
"figure 'walker'\n"
"node head  0 -120 14 #378ADD\n"
"node neck  0  -93  7 #5DCAA5\n"
"node body  0  -20  8 #5DCAA5\n"
"node ls  -30  -75  7 #888780\n"
"node rs   30  -75  7 #888780\n"
"node le  -52  -40  6 #888780\n"
"node re   52  -40  6 #888780\n"
"node lh  -58   -8  5 #888780\n"
"node rh   58   -8  5 #888780\n"
"node hip   0  -20  8 #5DCAA5\n"
"node lk  -22   32  6 #888780\n"
"node rk   22   32  6 #888780\n"
"node lf  -27   85  6 #444441\n"
"node rf   27   85  6 #444441\n"
"bone head neck  4 #888780\n"
"bone neck body  6 #888780\n"
"bone neck ls    5 #888780\n"
"bone neck rs    5 #888780\n"
"bone ls le      4 #B4B2A9\n"
"bone rs re      4 #B4B2A9\n"
"bone le lh      3 #B4B2A9\n"
"bone re rh      3 #B4B2A9\n"
"bone body hip   6 #888780\n"
"bone hip lk     5 #B4B2A9\n"
"bone hip rk     5 #B4B2A9\n"
"bone lk lf      4 #B4B2A9\n"
"bone rk rf      4 #B4B2A9\n\n"
"# خطوة 1\n"
"frame 110\n"
"pose lk -35 55\n"
"pose lf -45 115\n"
"pose rk  15 80\n"
"pose rf  20 130\n"
"pose le -60 -50\n"
"pose lh -65 -15\n"
"pose re  50 -30\n"
"pose rh  55   0\n\n"
"# خطوة 2\n"
"frame 110\n"
"pose lk -15 80\n"
"pose lf -20 130\n"
"pose rk  30 55\n"
"pose rf  35 115\n"
"pose le -50 -30\n"
"pose lh -55   0\n"
"pose re  60 -50\n"
"pose rh  65 -15\n\n"
"# خطوة 3 (العودة)\n"
"frame 110\n"
"pose lk -22 32\n"
"pose lf -27 85\n"
"pose rk  22 32\n"
"pose rf  27 85\n"
"pose le -52 -40\n"
"pose lh -58  -8\n"
"pose re  52 -40\n"
"pose rh  58  -8\n"
    );
}

void CodeEditorDialog::runCode()
{
    if (!m_proj) { m_status->setText("❌ لا يوجد مشروع مفتوح"); return; }

    QString code = m_editor->toPlainText();
    QStringList lines = code.split('\n');

    AnimProject newProj;
    newProj.background = m_proj->background;
    newProj.fps        = m_proj->fps;
    newProj.loopMode   = m_proj->loopMode;
    newProj.name       = m_proj->name;

    QList<Figure> currentFigs;
    Figure *curFig = nullptr;
    Frame   curFrame;
    bool    inFrame = false;

    auto commitFrame = [&](){
        if (inFrame) {
            curFrame.figures = currentFigs;
            newProj.frames.append(curFrame);
            curFrame = Frame();
            inFrame = false;
        }
    };

    auto stripQuotes = [](const QString &s) -> QString {
        QString r = s.trimmed();
        if ((r.startsWith('"')&&r.endsWith('"'))||(r.startsWith('\'')&&r.endsWith('\'')))
            return r.mid(1,r.length()-2);
        return r;
    };

    int lineNum = 0;
    for (const QString &rawLine : lines) {
        lineNum++;
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) continue;

        // Remove inline comments
        int ci = line.indexOf("//"); if(ci>=0) line=line.left(ci).trimmed();
        ci = line.indexOf('#'); if(ci>=0 && ci>0) line=line.left(ci).trimmed();

        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;

        QString cmd = parts[0].toLower();

        if (cmd == "background" && parts.size()>=2) {
            newProj.background = QColor(parts[1]); continue;
        }
        if (cmd == "anim_fps" && parts.size()>=2) {
            newProj.fps = parts[1].toInt(); continue;
        }
        if (cmd == "anim_loop" && parts.size()>=2) {
            newProj.loopMode = parts[1]; continue;
        }

        if (cmd == "figure") {
            commitFrame();
            Figure f;
            f.id   = uniqueId();
            f.name = parts.size()>=2 ? stripQuotes(parts.mid(1).join(' ')) : QString("شخصية");
            currentFigs.append(f);
            curFig = &currentFigs.last();
            continue;
        }

        if (cmd == "node" && parts.size()>=4) {
            if (!curFig) {
                Figure f; f.id=uniqueId(); f.name="شخصية"; currentFigs.append(f); curFig=&currentFigs.last();
            }
            Node n;
            n.id    = parts[1];
            n.x     = parts[2].toDouble();
            n.y     = parts[3].toDouble();
            n.size  = parts.size()>=5 ? parts[4].toDouble() : 8;
            n.color = parts.size()>=6 ? QColor(parts[5]) : QColor(55,138,221);
            curFig->nodes.append(n);
            continue;
        }

        if (cmd == "bone" && parts.size()>=3) {
            if (!curFig) continue;
            Bone b;
            b.fromId = parts[1];
            b.toId   = parts[2];
            b.width  = parts.size()>=4 ? parts[3].toDouble() : 4;
            b.color  = parts.size()>=5 ? QColor(parts[4]) : QColor(136,135,128);
            curFig->bones.append(b);
            continue;
        }

        if (cmd == "frame") {
            commitFrame();
            curFrame.duration = parts.size()>=2 ? parts[1].toInt() : 100;
            curFrame.label    = QString::number(newProj.frames.size()+1);
            // Deep copy current figures as baseline
            curFrame.figures  = currentFigs;
            inFrame = true;
            continue;
        }

        if (cmd == "pose" && parts.size()>=4 && inFrame) {
            QString nid = parts[1];
            double  nx  = parts[2].toDouble();
            double  ny  = parts[3].toDouble();
            for (auto &fig : curFrame.figures) {
                for (auto &n : fig.nodes) {
                    if (n.id == nid) { n.x=nx; n.y=ny; }
                }
            }
            continue;
        }

        m_status->setText(QString("⚠ سطر %1 مجهول: %2").arg(lineNum).arg(cmd));
    }
    commitFrame();

    if (newProj.frames.isEmpty() && !currentFigs.isEmpty()) {
        Frame f; f.duration=100; f.label="1"; f.figures=currentFigs;
        newProj.frames.append(f);
    }

    if (newProj.frames.isEmpty()) {
        m_status->setText("❌ لا توجد إطارات — تأكد من استخدام frame بعد node");
        return;
    }

    *m_proj = newProj;
    m_status->setText(QString("✓ نجح — %1 شخصية، %2 إطار").arg(currentFigs.size()).arg(newProj.frames.size()));
    emit codeApplied();
    close();
}
