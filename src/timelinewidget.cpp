#include "timelinewidget.h"
#include <QPainter>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent)
{
    auto *vl = new QVBoxLayout(this);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(0);

    m_scroll = new QScrollArea(this);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFixedHeight(90);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_inner  = new QWidget;
    m_layout = new QHBoxLayout(m_inner);
    m_layout->setContentsMargins(8,6,8,6);
    m_layout->setSpacing(4);
    m_layout->addStretch();

    m_scroll->setWidget(m_inner);
    m_scroll->setWidgetResizable(false);
    vl->addWidget(m_scroll);
}

void TimelineWidget::setProject(AnimProject *p)
{
    m_proj = p;
    m_cur  = 0;
    rebuild();
}

void TimelineWidget::refresh(int currentFrame)
{
    m_cur = currentFrame;
    rebuild();
}

void TimelineWidget::rebuild()
{
    // clear
    QLayoutItem *item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (!m_proj) { m_layout->addStretch(); return; }

    for (int i = 0; i < m_proj->frames.size(); i++) {
        const Frame &fr = m_proj->frames[i];

        QFrame *cell = new QFrame;
        cell->setFixedSize(58, 68);
        cell->setFrameShape(QFrame::Box);
        cell->setLineWidth(i==m_cur ? 2 : 1);
        cell->setStyleSheet(i==m_cur
            ? "QFrame{border:2px solid #378ADD;border-radius:5px;background:#1a2535;}"
            : "QFrame{border:1px solid #3a3a52;border-radius:5px;background:#16161e;}");

        // Mini preview
        QLabel *preview = new QLabel(cell);
        preview->setGeometry(3, 3, 52, 52);
        QPixmap px(52, 52);
        px.fill(m_proj->background);
        QPainter pp(&px);
        pp.setRenderHint(QPainter::Antialiasing);
        pp.translate(26, 26);
        double sc = 0.10;
        pp.scale(sc, sc);
        for (const auto &fig : fr.figures) {
            if (!fig.visible) continue;
            for (const auto &b : fig.bones) {
                const Node *n1=nullptr, *n2=nullptr;
                for (const auto &n : fig.nodes) {
                    if(n.id==b.fromId) n1=&n;
                    if(n.id==b.toId)   n2=&n;
                }
                if(!n1||!n2) continue;
                pp.setPen(QPen(b.color, b.width));
                pp.drawLine(QLineF(n1->x,n1->y,n2->x,n2->y));
            }
            for (const auto &n : fig.nodes) {
                pp.setBrush(n.color);
                pp.setPen(Qt::NoPen);
                pp.drawEllipse(QPointF(n.x,n.y),n.size,n.size);
            }
        }
        preview->setPixmap(px);

        // Label
        QLabel *lbl = new QLabel(fr.label.isEmpty() ? QString::number(i+1) : fr.label, cell);
        lbl->setGeometry(0, 56, 58, 12);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("font-size:9px;color:#9090aa;background:transparent;");

        int idx = i;
        cell->setCursor(Qt::PointingHandCursor);
        cell->installEventFilter(this);
        cell->setProperty("frameIdx", idx);

        connect(new QObject(cell), &QObject::destroyed, []{});
        // Use mouse press via event filter or simple approach:
        auto *clickCatcher = new QLabel(cell);
        clickCatcher->setGeometry(0,0,58,68);
        clickCatcher->setStyleSheet("background:transparent;");
        clickCatcher->setCursor(Qt::PointingHandCursor);
        clickCatcher->installEventFilter(this);
        clickCatcher->setProperty("frameIdx", idx);

        m_layout->addWidget(cell);
    }
    m_layout->addStretch();
    m_inner->adjustSize();

    // scroll to current
    if (m_cur < m_proj->frames.size()) {
        m_scroll->ensureWidgetVisible(m_layout->itemAt(m_cur) ?
            m_layout->itemAt(m_cur)->widget() : m_inner);
    }
}

bool TimelineWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QVariant v = obj->property("frameIdx");
        if (v.isValid()) {
            emit frameSelected(v.toInt());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
