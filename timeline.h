#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QVector>
#include "types.h"

class FrameThumb : public QWidget {
    Q_OBJECT
public:
    int index;
    bool selected = false;
    Frame* frame = nullptr;
    QColor bgCol;

    FrameThumb(int i, Frame* f, QColor bg, QWidget* parent=nullptr)
        : QWidget(parent), index(i), frame(f), bgCol(bg) {
        setFixedSize(58,72);
        setCursor(Qt::PointingHandCursor);
    }
signals:
    void clicked(int i);
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // border
        p.setPen(QPen(selected ? QColor("#185FA5") : QColor(180,180,180), selected?2:1));
        p.setBrush(Qt::white);
        p.drawRoundedRect(2,2,54,54,4,4);

        // mini preview
        if (frame) {
            p.save();
            p.setClipRect(3,3,52,52);
            p.translate(29,29);
            qreal sc = 0.09;
            p.scale(sc,sc);
            for (auto& fig : frame->figures) {
                if (!fig.visible) continue;
                for (auto& b : fig.bones) {
                    const Node* n1=fig.findNode(b.from);
                    const Node* n2=fig.findNode(b.to);
                    if (!n1||!n2) continue;
                    p.setPen(QPen(b.color, b.width, Qt::SolidLine, Qt::RoundCap));
                    p.drawLine(QPointF(n1->x,n1->y),QPointF(n2->x,n2->y));
                }
                for (auto& n : fig.nodes) {
                    p.setBrush(n.color); p.setPen(Qt::NoPen);
                    p.drawEllipse(QPointF(n.x,n.y),n.size,n.size);
                }
            }
            p.restore();
        }

        // label
        p.setPen(selected ? QColor("#185FA5") : QColor(100,100,100));
        p.setFont(QFont("monospace",8));
        QString lbl = frame ? (frame->label.isEmpty() ? QString::number(index+1) : frame->label) : QString::number(index+1);
        p.drawText(QRect(0,56,58,14), Qt::AlignCenter, lbl);
    }
    void mousePressEvent(QMouseEvent*) override { emit clicked(index); }
};

class Timeline : public QWidget {
    Q_OBJECT
public:
    QVector<FrameThumb*> thumbs;
    int current = 0;

    explicit Timeline(QWidget* parent=nullptr) : QWidget(parent) {
        setFixedHeight(100);
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0,0,0,0);
        lay->setSpacing(0);

        // top bar
        auto* bar = new QWidget;
        bar->setFixedHeight(22);
        auto* blay = new QHBoxLayout(bar);
        blay->setContentsMargins(6,2,6,2);
        blay->setSpacing(4);
        bar->setStyleSheet("background:#e8e8e8;border-top:1px solid #ccc;border-bottom:1px solid #ccc;");
        auto lbl = new QLabel("TIMELINE");
        lbl->setStyleSheet("font-size:10px;font-weight:600;color:#666;letter-spacing:1px;");
        infoLbl = new QLabel("إطار 1 / 1");
        infoLbl->setStyleSheet("font-size:10px;color:#666;");
        timeLbl = new QLabel("0.0 ث");
        timeLbl->setStyleSheet("font-size:10px;color:#666;");
        blay->addWidget(lbl); blay->addWidget(infoLbl);
        blay->addStretch(); blay->addWidget(timeLbl);
        lay->addWidget(bar);

        // scroll area
        scroll = new QScrollArea;
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidgetResizable(false);
        scroll->setStyleSheet("background:#f5f5f5;border:none;");
        container = new QWidget;
        hlay = new QHBoxLayout(container);
        hlay->setContentsMargins(6,4,6,4);
        hlay->setSpacing(4);
        hlay->addStretch();
        scroll->setWidget(container);
        lay->addWidget(scroll);
    }

    void rebuild(const QVector<Frame>& frames, int cur, QColor bg) {
        // clear
        for (auto* t : thumbs) { hlay->removeWidget(t); delete t; }
        thumbs.clear();

        int total = 0;
        for (int i=0;i<frames.size();i++) {
            auto* t = new FrameThumb(i, const_cast<Frame*>(&frames[i]), bg, container);
            t->selected = (i==cur);
            connect(t, &FrameThumb::clicked, this, &Timeline::frameClicked);
            hlay->insertWidget(hlay->count()-1, t);
            thumbs.append(t);
            total += frames[i].duration;
        }
        current = cur;
        infoLbl->setText(QString("إطار %1 / %2").arg(cur+1).arg(frames.size()));
        timeLbl->setText(QString("%1 ث").arg(total/1000.0, 0,'f',1));
    }

    void setCurrentFrame(int i) {
        for (auto* t : thumbs) t->selected = (t->index == i);
        for (auto* t : thumbs) t->update();
        current = i;
        if (i < thumbs.size()) {
            scroll->ensureWidgetVisible(thumbs[i]);
        }
    }

signals:
    void frameClicked(int i);

private:
    QScrollArea* scroll;
    QWidget*     container;
    QHBoxLayout* hlay;
    QLabel*      infoLbl;
    QLabel*      timeLbl;
};
