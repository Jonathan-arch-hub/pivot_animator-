#pragma once
#include <QString>
#include <QColor>
#include <QPointF>
#include <QVector>
#include <QUuid>

struct Node {
    QString id;
    qreal x = 0, y = 0;
    qreal size = 8;
    QColor color = QColor("#378ADD");
    bool visible = true;

    Node() : id(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)) {}
    Node(qreal x, qreal y, qreal size=8, QColor col=QColor("#378ADD"))
        : id(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)),
          x(x), y(y), size(size), color(col) {}
};

struct Bone {
    QString from, to;
    qreal width = 4;
    QColor color = QColor("#888780");
    bool rounded = false;
};

struct Figure {
    QString id;
    QString name;
    QVector<Node> nodes;
    QVector<Bone> bones;
    bool visible = true;

    Figure() : id(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)) {}

    Node* findNode(const QString& nid) {
        for (auto& n : nodes) if (n.id == nid) return &n;
        return nullptr;
    }
    const Node* findNode(const QString& nid) const {
        for (const auto& n : nodes) if (n.id == nid) return &n;
        return nullptr;
    }
};

struct Frame {
    QVector<Figure> figures;
    int duration = 100; // ms
    QString label;
};
