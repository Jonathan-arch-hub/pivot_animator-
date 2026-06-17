#include "animdata.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

// ──────────────────────────── Node ──────────────────────────────────────────
QJsonObject Node::toJson() const {
    return {{"id",id},{"x",x},{"y",y},{"size",size},
            {"color",color.name()},{"visible",visible}};
}
Node Node::fromJson(const QJsonObject &o) {
    Node n;
    n.id      = o["id"].toString();
    n.x       = o["x"].toDouble();
    n.y       = o["y"].toDouble();
    n.size    = o["size"].toDouble(8);
    n.color   = QColor(o["color"].toString("#378ADD"));
    n.visible = o["visible"].toBool(true);
    return n;
}

// ──────────────────────────── Bone ──────────────────────────────────────────
QJsonObject Bone::toJson() const {
    return {{"from",fromId},{"to",toId},{"width",width},
            {"color",color.name()},{"round",round},{"visible",visible}};
}
Bone Bone::fromJson(const QJsonObject &o) {
    Bone b;
    b.fromId  = o["from"].toString();
    b.toId    = o["to"].toString();
    b.width   = o["width"].toDouble(4);
    b.color   = QColor(o["color"].toString("#888780"));
    b.round   = o["round"].toInt(0);
    b.visible = o["visible"].toBool(true);
    return b;
}

// ──────────────────────────── Figure ────────────────────────────────────────
Node* Figure::nodeById(const QString &nid) {
    for (auto &n : nodes) if (n.id == nid) return &n;
    return nullptr;
}
QJsonObject Figure::toJson() const {
    QJsonArray na, ba;
    for (auto &n : nodes) na.append(n.toJson());
    for (auto &b : bones) ba.append(b.toJson());
    return {{"id",id},{"name",name},{"nodes",na},{"bones",ba},{"visible",visible}};
}
Figure Figure::fromJson(const QJsonObject &o) {
    Figure f;
    f.id      = o["id"].toString();
    f.name    = o["name"].toString();
    f.visible = o["visible"].toBool(true);
    for (auto v : o["nodes"].toArray()) f.nodes.append(Node::fromJson(v.toObject()));
    for (auto v : o["bones"].toArray()) f.bones.append(Bone::fromJson(v.toObject()));
    return f;
}

// ──────────────────────────── Frame ─────────────────────────────────────────
QJsonObject Frame::toJson() const {
    QJsonArray fa;
    for (auto &fig : figures) fa.append(fig.toJson());
    return {{"figures",fa},{"duration",duration},{"label",label}};
}
Frame Frame::fromJson(const QJsonObject &o) {
    Frame f;
    f.duration = o["duration"].toInt(100);
    f.label    = o["label"].toString();
    for (auto v : o["figures"].toArray()) f.figures.append(Figure::fromJson(v.toObject()));
    return f;
}

// ──────────────────────────── AnimProject ───────────────────────────────────
QJsonObject AnimProject::toJson() const {
    QJsonArray fa;
    for (auto &fr : frames) fa.append(fr.toJson());
    return {{"frames",fa},{"background",background.name()},
            {"fps",fps},{"loopMode",loopMode},{"name",name}};
}
AnimProject AnimProject::fromJson(const QJsonObject &o) {
    AnimProject p;
    p.background = QColor(o["background"].toString("#1e1e28"));
    p.fps        = o["fps"].toInt(12);
    p.loopMode   = o["loopMode"].toString("loop");
    p.name       = o["name"].toString("Untitled");
    for (auto v : o["frames"].toArray()) p.frames.append(Frame::fromJson(v.toObject()));
    return p;
}
