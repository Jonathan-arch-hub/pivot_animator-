#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include "types.h"

class CodeHighlighter : public QSyntaxHighlighter {
public:
    CodeHighlighter(QTextDocument* doc) : QSyntaxHighlighter(doc) {
        QTextCharFormat kw; kw.setForeground(QColor("#C792EA")); kw.setFontWeight(QFont::Bold);
        QTextCharFormat fn; fn.setForeground(QColor("#82AAFF"));
        QTextCharFormat str; str.setForeground(QColor("#C3E88D"));
        QTextCharFormat num; num.setForeground(QColor("#F78C6C"));
        QTextCharFormat cmt; cmt.setForeground(QColor("#546E7A")); cmt.setFontItalic(true);
        QTextCharFormat hex; hex.setForeground(QColor("#FFCB6B"));

        rules.append({QRegularExpression(R"(\b(figure|node|bone|frame|pose)\b)"), fn});
        rules.append({QRegularExpression(R"(\b(\d+\.?\d*)\b)"), num});
        rules.append({QRegularExpression(R"(\"[^\"]*\")"), str});
        rules.append({QRegularExpression(R"(#[0-9A-Fa-f]{3,8})"), hex});
        rules.append({QRegularExpression(R"(//[^\n]*)"), cmt});
    }
protected:
    struct Rule { QRegularExpression re; QTextCharFormat fmt; };
    QVector<Rule> rules;
    void highlightBlock(const QString& text) override {
        for (auto& r : rules) {
            auto it = r.re.globalMatch(text);
            while (it.hasNext()) {
                auto m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), r.fmt);
            }
        }
    }
};

class CodePanel : public QWidget {
    Q_OBJECT
public:
    explicit CodePanel(QWidget* parent=nullptr) : QWidget(parent) {
        setStyleSheet("background:#1a1a2e;");
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0,0,0,0);
        lay->setSpacing(0);

        // top bar
        auto* bar = new QWidget;
        bar->setStyleSheet("background:#16213e;border-bottom:1px solid #0f3460;");
        bar->setFixedHeight(36);
        auto* blay = new QHBoxLayout(bar);
        blay->setContentsMargins(10,4,10,4); blay->setSpacing(6);
        auto* title = new QLabel("محرر الكود — أنشئ أنيميشن بالكود");
        title->setStyleSheet("color:#a9b7d0;font-size:12px;font-weight:600;");
        runBtn  = makeBtn("▶ تشغيل","#185FA5");
        sampBtn = makeBtn("مثال","#0f3460");
        clrBtn  = makeBtn("✕","#3d0000");
        blay->addWidget(title); blay->addStretch();
        blay->addWidget(sampBtn); blay->addWidget(runBtn); blay->addWidget(clrBtn);
        lay->addWidget(bar);

        // editor
        editor = new QPlainTextEdit;
        editor->setStyleSheet(
            "QPlainTextEdit{background:#0d1117;color:#a9b7d0;"
            "font-family:'Fira Code','Courier New',monospace;font-size:13px;"
            "border:none;padding:10px;line-height:1.6;}");
        editor->setPlaceholderText(
            "// API المتاح:\n"
            "// figure(\"name\")                  — شخصية جديدة\n"
            "// node(\"id\", x, y, size, \"#color\") — مفصل\n"
            "// bone(\"from\",\"to\", width, \"#color\") — عظمة\n"
            "// frame(duration_ms)              — إطار جديد\n"
            "// pose({\"id\":{x,y}, ...})         — غيّر مواضع\n");
        new CodeHighlighter(editor->document());
        lay->addWidget(editor);

        // status
        statusLbl = new QLabel("جاهز");
        statusLbl->setStyleSheet("color:#546E7A;font-size:11px;padding:4px 10px;background:#0d1117;border-top:1px solid #1e2a3a;");
        lay->addWidget(statusLbl);

        connect(runBtn,  &QPushButton::clicked, this, &CodePanel::runRequested);
        connect(sampBtn, &QPushButton::clicked, this, &CodePanel::loadSample);
        connect(clrBtn,  &QPushButton::clicked, this, &QWidget::hide);
    }

    QString code() const { return editor->toPlainText(); }
    void setStatus(const QString& s, bool ok=true) {
        statusLbl->setStyleSheet(QString("color:%1;font-size:11px;padding:4px 10px;background:#0d1117;border-top:1px solid #1e2a3a;").arg(ok?"#C3E88D":"#FF5370"));
        statusLbl->setText(s);
    }

signals:
    void runRequested();

private:
    QPlainTextEdit* editor;
    QPushButton*    runBtn;
    QPushButton*    sampBtn;
    QPushButton*    clrBtn;
    QLabel*         statusLbl;

    QPushButton* makeBtn(const QString& txt, const QString& bg) {
        auto* b = new QPushButton(txt);
        b->setStyleSheet(QString("QPushButton{background:%1;color:#a9b7d0;border:none;border-radius:4px;padding:3px 10px;font-size:11px;}"
                                 "QPushButton:hover{background:#378ADD;color:#fff;}").arg(bg));
        return b;
    }

    void loadSample() {
        editor->setPlainText(
R"(// مثال: رجل يمشي
figure("walker");
node("head",  0, -120, 14, "#378ADD");
node("neck",  0,  -90,  7, "#5DCAA5");
node("body",  0,  -20,  8, "#5DCAA5");
node("ls",  -30,  -75,  7, "#888780");
node("rs",   30,  -75,  7, "#888780");
node("le",  -55,  -40,  6, "#888780");
node("re",   55,  -40,  6, "#888780");
node("lh",  -60,  -10,  5, "#888780");
node("rh",   60,  -10,  5, "#888780");
node("hip",   0,   20,  8, "#5DCAA5");
node("lk",  -20,   70,  6, "#888780");
node("rk",   20,   70,  6, "#888780");
node("lf",  -25,  120,  6, "#444441");
node("rf",   25,  120,  6, "#444441");
bone("head","neck", 4,"#888780");
bone("neck","body", 6,"#888780");
bone("neck","ls",   5,"#888780");
bone("neck","rs",   5,"#888780");
bone("ls","le",     4,"#B4B2A9");
bone("rs","re",     4,"#B4B2A9");
bone("le","lh",     3,"#B4B2A9");
bone("re","rh",     3,"#B4B2A9");
bone("body","hip",  6,"#888780");
bone("hip","lk",    5,"#B4B2A9");
bone("hip","rk",    5,"#B4B2A9");
bone("lk","lf",     4,"#B4B2A9");
bone("rk","rf",     4,"#B4B2A9");

frame(120);
pose({"lk":{-35,55},"lf":{-45,115},"rk":{15,80},"rf":{20,130},"le":{-60,-50},"lh":{-65,-15},"re":{50,-30},"rh":{55,0}});

frame(120);
pose({"lk":{-15,80},"lf":{-20,130},"rk":{30,55},"rf":{35,115},"le":{-50,-30},"lh":{-55,0},"re":{60,-50},"rh":{65,-15}});

frame(120);
pose({"lk":{-20,70},"lf":{-25,120},"rk":{20,70},"rf":{25,120},"le":{-55,-40},"lh":{-60,-10},"re":{55,-40},"rh":{60,-10}});
)");
    }
};
