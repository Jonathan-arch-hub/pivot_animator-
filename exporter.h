#pragma once
#include <QString>
#include <QVector>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QImage>
#include <QPainter>
#include <QColor>
#include "types.h"

class Exporter {
public:
    static void exportJSON(const QVector<Frame>& frames, QColor bg, int fps, const QString& loop) {
        QString path = QFileDialog::getSaveFileName(nullptr,"حفظ JSON","animation.json","JSON (*.json)");
        if (path.isEmpty()) return;
        QJsonObject root;
        root["fps"] = fps; root["loop"] = loop;
        root["background"] = bg.name();
        QJsonArray jframes;
        for (auto& f : frames) {
            QJsonObject jf;
            jf["duration"] = f.duration;
            jf["label"] = f.label;
            QJsonArray jfigs;
            for (auto& fig : f.figures) {
                QJsonObject jfig;
                jfig["id"]=fig.id; jfig["name"]=fig.name; jfig["visible"]=fig.visible;
                QJsonArray jnodes;
                for (auto& n : fig.nodes) {
                    QJsonObject jn;
                    jn["id"]=n.id; jn["x"]=n.x; jn["y"]=n.y;
                    jn["size"]=n.size; jn["color"]=n.color.name(); jn["visible"]=n.visible;
                    jnodes.append(jn);
                }
                QJsonArray jbones;
                for (auto& b : fig.bones) {
                    QJsonObject jb;
                    jb["from"]=b.from; jb["to"]=b.to; jb["width"]=b.width;
                    jb["color"]=b.color.name(); jb["rounded"]=b.rounded;
                    jbones.append(jb);
                }
                jfig["nodes"]=jnodes; jfig["bones"]=jbones;
                jfigs.append(jfig);
            }
            jf["figures"]=jfigs;
            jframes.append(jf);
        }
        root["frames"]=jframes;
        QFile f(path); f.open(QIODevice::WriteOnly);
        f.write(QJsonDocument(root).toJson());
        QMessageBox::information(nullptr,"تم","تم حفظ JSON بنجاح");
    }

    static void importJSON(QVector<Frame>& frames, QColor& bg, QWidget* parent) {
        QString path = QFileDialog::getOpenFileName(parent,"فتح JSON","","JSON (*.json)");
        if (path.isEmpty()) return;
        QFile f(path); if (!f.open(QIODevice::ReadOnly)) return;
        QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        bg = QColor(root["background"].toString("#f0f0f0"));
        frames.clear();
        for (auto jf : root["frames"].toArray()) {
            QJsonObject jo = jf.toObject();
            Frame frame; frame.duration=jo["duration"].toInt(100); frame.label=jo["label"].toString();
            for (auto jfig : jo["figures"].toArray()) {
                QJsonObject jfo = jfig.toObject();
                Figure fig; fig.id=jfo["id"].toString(); fig.name=jfo["name"].toString(); fig.visible=jfo["visible"].toBool(true);
                for (auto jn : jfo["nodes"].toArray()) {
                    QJsonObject no=jn.toObject();
                    Node n; n.id=no["id"].toString(); n.x=no["x"].toDouble(); n.y=no["y"].toDouble();
                    n.size=no["size"].toDouble(8); n.color=QColor(no["color"].toString("#378ADD")); n.visible=no["visible"].toBool(true);
                    fig.nodes.append(n);
                }
                for (auto jb : jfo["bones"].toArray()) {
                    QJsonObject bo=jb.toObject();
                    Bone b; b.from=bo["from"].toString(); b.to=bo["to"].toString();
                    b.width=bo["width"].toDouble(4); b.color=QColor(bo["color"].toString("#888780")); b.rounded=bo["rounded"].toBool();
                    fig.bones.append(b);
                }
                frame.figures.append(fig);
            }
            frames.append(frame);
        }
    }

    static QImage renderFrame(const Frame& f, int size, QColor bg) {
        QImage img(size,size,QImage::Format_ARGB32);
        img.fill(bg);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(size/2, size/2);
        for (auto& fig : f.figures) {
            if (!fig.visible) continue;
            for (auto& b : fig.bones) {
                const Node* n1=fig.findNode(b.from); const Node* n2=fig.findNode(b.to);
                if (!n1||!n2) continue;
                QPen pen(b.color,b.width,Qt::SolidLine,b.rounded?Qt::RoundCap:Qt::FlatCap);
                p.setPen(pen); p.drawLine(QPointF(n1->x,n1->y),QPointF(n2->x,n2->y));
            }
            for (auto& n : fig.nodes) {
                p.setPen(Qt::NoPen); p.setBrush(n.color);
                p.drawEllipse(QPointF(n.x,n.y),n.size,n.size);
            }
        }
        return img;
    }

    static void exportPNGSequence(const QVector<Frame>& frames, QColor bg) {
        QString dir = QFileDialog::getExistingDirectory(nullptr,"اختر مجلد التصدير");
        if (dir.isEmpty()) return;
        for (int i=0;i<frames.size();i++) {
            QImage img = renderFrame(frames[i],400,bg);
            img.save(QString("%1/frame_%2.png").arg(dir).arg(i+1,4,10,QChar('0')));
        }
        QMessageBox::information(nullptr,"تم",QString("تم تصدير %1 إطار PNG").arg(frames.size()));
    }

    static void exportSpriteSheet(const QVector<Frame>& frames, QColor bg) {
        if (frames.isEmpty()) return;
        int sz=200, cols=qMin(frames.size(),8), rows=(frames.size()+cols-1)/cols;
        QImage sheet(sz*cols,sz*rows,QImage::Format_ARGB32);
        sheet.fill(bg);
        QPainter p(&sheet);
        for (int i=0;i<frames.size();i++) {
            QImage fi=renderFrame(frames[i],sz,bg);
            p.drawImage((i%cols)*sz,(i/cols)*sz,fi);
        }
        QString path=QFileDialog::getSaveFileName(nullptr,"حفظ Sprite Sheet","spritesheet.png","PNG (*.png)");
        if (!path.isEmpty()) { sheet.save(path); QMessageBox::information(nullptr,"تم","تم تصدير Sprite Sheet"); }
    }

    static void exportHTML(const QVector<Frame>& frames, QColor bg) {
        QString path=QFileDialog::getSaveFileName(nullptr,"حفظ HTML","animation.html","HTML (*.html)");
        if (path.isEmpty()) return;
        int total=0; for(auto&f:frames) total+=f.duration;
        QString svg;
        for (int fi=0;fi<frames.size();fi++) {
            svg+=QString("<g id=\"f%1\" style=\"display:%2\">\n").arg(fi).arg(fi==0?"block":"none");
            for (auto& fig:frames[fi].figures) {
                if (!fig.visible) continue;
                for (auto& b:fig.bones) {
                    const Node*n1=fig.findNode(b.from),*n2=fig.findNode(b.to);
                    if(!n1||!n2)continue;
                    svg+=QString("  <line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%4\" stroke=\"%5\" stroke-width=\"%6\" stroke-linecap=\"%7\"/>\n")
                        .arg(n1->x+200).arg(n1->y+200).arg(n2->x+200).arg(n2->y+200)
                        .arg(b.color.name()).arg(b.width).arg(b.rounded?"round":"butt");
                }
                for (auto& n:fig.nodes) {
                    svg+=QString("  <circle cx=\"%1\" cy=\"%2\" r=\"%3\" fill=\"%4\"/>\n")
                        .arg(n.x+200).arg(n.y+200).arg(n.size).arg(n.color.name());
                }
            }
            svg+="</g>\n";
        }
        QString durs; for(int i=0;i<frames.size();i++) durs+=QString::number(frames[i].duration)+(i<frames.size()-1?",":"");
        QString html=QString(
"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Pivot Animation</title>"
"<style>body{margin:0;background:%1;display:flex;justify-content:center;align-items:center;min-height:100vh;}</style></head>"
"<body><svg width=\"400\" height=\"400\" style=\"background:%1\">%2</svg>"
"<script>const fr=document.querySelectorAll('[id^=f]'),d=[%3];let c=0;"
"function nx(){fr[c].style.display='none';c=(c+1)%%fr.length;fr[c].style.display='block';setTimeout(nx,d[c]);}"
"setTimeout(nx,d[0]);</script></body></html>").arg(bg.name()).arg(svg).arg(durs);
        QFile f(path); f.open(QIODevice::WriteOnly); f.write(html.toUtf8());
        QMessageBox::information(nullptr,"تم","تم تصدير HTML");
    }

    static void exportGDScript(const QVector<Frame>& frames) {
        QString path=QFileDialog::getSaveFileName(nullptr,"حفظ GDScript","animation.gd","GDScript (*.gd)");
        if (path.isEmpty()) return;
        QString code="# GDScript Animation - Generated by Pivot Animator Pro\n";
        code+="# استخدم هذا مع Godot\n\n";
        code+="extends Node2D\n\nfunc _ready():\n\tplay_animation()\n\nfunc play_animation():\n";
        code+=QString("\tvar total_frames = %1\n").arg(frames.size());
        for (int fi=0;fi<frames.size();fi++) {
            code+=QString("\n\t# === إطار %1 (مدة: %2ms) ===\n").arg(fi+1).arg(frames[fi].duration);
            for (auto& fig:frames[fi].figures) {
                code+=QString("\t# شخصية: %1\n").arg(fig.name);
                for (auto& n:fig.nodes)
                    code+=QString("\t# node '%1': Vector2(%2, %3) size=%4\n").arg(n.id).arg(n.x,0,'f',1).arg(n.y,0,'f',1).arg(n.size,0,'f',1);
            }
        }
        code+="\n# Example AnimationPlayer usage:\n";
        code+="# var anim = $AnimationPlayer\n";
        code+="# anim.play(\"walk\")\n";
        QFile f(path); f.open(QIODevice::WriteOnly); f.write(code.toUtf8());
        QMessageBox::information(nullptr,"تم","تم تصدير GDScript");
    }

    static void exportCSSAnimation(const QVector<Frame>& frames) {
        QString path=QFileDialog::getSaveFileName(nullptr,"حفظ CSS","animation.css","CSS (*.css)");
        if (path.isEmpty()) return;
        int total=0; for(auto&f:frames) total+=f.duration;
        QString css=QString("/* CSS Animation - Pivot Animator Pro */\n.figure { animation: pivot %1ms infinite; }\n@keyframes pivot {\n").arg(total);
        int elapsed=0;
        for (int fi=0;fi<frames.size();fi++) {
            int pct=qRound(100.0*elapsed/total);
            css+=QString("  %1%% { /* frame %2 */ }\n").arg(pct).arg(fi+1);
            elapsed+=frames[fi].duration;
        }
        css+="  100% { }\n}\n";
        QFile f(path); f.open(QIODevice::WriteOnly); f.write(css.toUtf8());
        QMessageBox::information(nullptr,"تم","تم تصدير CSS Animation");
    }
};
