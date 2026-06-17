#pragma once
#include <QString>
#include <QColor>
#include <QPointF>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

// ─── Node (joint/pivot point) ───────────────────────────────────────────────
struct Node {
    QString id;
    double  x = 0, y = 0;
    double  size = 8;
    QColor  color = QColor(55, 138, 221);
    bool    visible = true;

    QJsonObject toJson() const;
    static Node fromJson(const QJsonObject &o);
};

// ─── Bone (segment between two nodes) ───────────────────────────────────────
struct Bone {
    QString fromId, toId;
    double  width = 4;
    QColor  color = QColor(136, 135, 128);
    int     round = 0;   // cap rounding 0-20
    bool    visible = true;

    QJsonObject toJson() const;
    static Bone fromJson(const QJsonObject &o);
};

// ─── Figure (collection of nodes + bones) ───────────────────────────────────
struct Figure {
    QString     id;
    QString     name;
    QList<Node> nodes;
    QList<Bone> bones;
    bool        visible = true;

    Node*  nodeById(const QString &id);
    QJsonObject toJson() const;
    static Figure fromJson(const QJsonObject &o);
};

// ─── Frame ──────────────────────────────────────────────────────────────────
struct Frame {
    QList<Figure> figures;
    int           duration = 100;  // ms
    QString       label;

    QJsonObject toJson() const;
    static Frame fromJson(const QJsonObject &o);
};

// ─── Animation project ──────────────────────────────────────────────────────
struct AnimProject {
    QList<Frame> frames;
    QColor       background = QColor(30, 30, 40);
    int          fps = 12;
    QString      loopMode = "loop";   // loop | once | pingpong
    QString      name = "Untitled";

    QJsonObject toJson() const;
    static AnimProject fromJson(const QJsonObject &o);
};
