#pragma once
#include <QString>
#include <QVector>
#include <QRegularExpression>
#include <QColor>
#include <QMap>
#include <cmath>
#include "types.h"

struct CodeError { int line; QString msg; };

class CodeInterpreter {
public:
    QVector<Frame> frames;
    QVector<Figure> figures;
    QVector<CodeError> errors;

    bool run(const QString& code) {
        frames.clear(); figures.clear(); errors.clear();
        Figure* curFig = nullptr;
        QMap<QString,Node*> nodeMap;

        QStringList lines = code.split('\n');
        for (int li=0; li<lines.size(); li++) {
            QString line = lines[li].trimmed();
            if (line.isEmpty() || line.startsWith("//")) continue;

            // figure("name")
            if (line.startsWith("figure(")) {
                QString name = extractStr(line, 0);
                if (name.isNull()) name = QString("شخصية %1").arg(figures.size()+1);
                Figure fig; fig.name = name;
                figures.append(fig);
                curFig = &figures.last();
                nodeMap.clear();
                continue;
            }

            // node("id", x, y, size, "#color")
            if (line.startsWith("node(")) {
                if (!curFig) { Figure fig; fig.name="شخصية 1"; figures.append(fig); curFig=&figures.last(); }
                QVector<QString> args = splitArgs(line);
                if (args.size() < 3) { errors.append({li+1,"node: يحتاج id,x,y"}); continue; }
                Node n;
                n.id    = stripQuotes(args[0]);
                n.x     = args[1].toDouble();
                n.y     = args[2].toDouble();
                n.size  = args.size()>3 ? args[3].toDouble() : 8;
                n.color = args.size()>4 ? QColor(stripQuotes(args[4])) : QColor("#378ADD");
                n.visible = true;
                curFig->nodes.append(n);
                nodeMap[n.id] = &curFig->nodes.last();
                continue;
            }

            // bone("from","to", width, "#color")
            if (line.startsWith("bone(")) {
                if (!curFig) { errors.append({li+1,"bone: يجب تعريف figure أولاً"}); continue; }
                QVector<QString> args = splitArgs(line);
                if (args.size() < 2) { errors.append({li+1,"bone: يحتاج from,to"}); continue; }
                Bone b;
                b.from  = stripQuotes(args[0]);
                b.to    = stripQuotes(args[1]);
                b.width = args.size()>2 ? args[2].toDouble() : 4;
                b.color = args.size()>3 ? QColor(stripQuotes(args[3])) : QColor("#888780");
                b.rounded = false;
                curFig->bones.append(b);
                continue;
            }

            // frame(duration)
            if (line.startsWith("frame(")) {
                QVector<QString> args = splitArgs(line);
                Frame f;
                f.duration = args.isEmpty() ? 100 : args[0].toInt();
                f.label = QString::number(frames.size()+1);
                f.figures = figures;
                frames.append(f);
                continue;
            }

            // pose({"id":{x,y}, ...})
            if (line.startsWith("pose(")) {
                if (frames.isEmpty()) { errors.append({li+1,"pose: يجب وجود frame أولاً"}); continue; }
                Frame& lastFrame = frames.last();
                parsePose(line, lastFrame);
                continue;
            }

            // unknown
            if (!line.isEmpty())
                errors.append({li+1, QString("سطر غير معروف: %1").arg(line.left(30))});
        }

        if (frames.isEmpty() && !figures.isEmpty()) {
            Frame f; f.duration=100; f.label="1"; f.figures=figures;
            frames.append(f);
        }

        return errors.isEmpty();
    }

private:
    QString extractStr(const QString& line, int skip) {
        QRegularExpression re("\"([^\"]*)\"");
        auto it = re.globalMatch(line);
        int i=0;
        while (it.hasNext()) {
            auto m=it.next();
            if (i==skip) return m.captured(1);
            i++;
        }
        return QString();
    }

    QString stripQuotes(const QString& s) {
        QString t = s.trimmed();
        if ((t.startsWith('"')&&t.endsWith('"'))||(t.startsWith('\'')&&t.endsWith('\'')))
            return t.mid(1,t.length()-2);
        return t;
    }

    QVector<QString> splitArgs(const QString& line) {
        int start = line.indexOf('(');
        int end   = line.lastIndexOf(')');
        if (start<0||end<0) return {};
        QString inner = line.mid(start+1, end-start-1).trimmed();

        QVector<QString> args;
        int depth=0; QString cur;
        bool inStr=false; char strCh=0;
        for (QChar c : inner) {
            if (!inStr && (c=='"'||c=='\'')) { inStr=true; strCh=c.toLatin1(); cur+=c; }
            else if (inStr && c==strCh)     { inStr=false; cur+=c; }
            else if (!inStr && c=='{')      { depth++; cur+=c; }
            else if (!inStr && c=='}')      { depth--; cur+=c; }
            else if (!inStr && c==',' && depth==0) { args.append(cur.trimmed()); cur.clear(); }
            else cur+=c;
        }
        if (!cur.trimmed().isEmpty()) args.append(cur.trimmed());
        return args;
    }

    // pose({"id":{x,y},...}) or pose({"id":{x:val,y:val},...})
    void parsePose(const QString& line, Frame& frame) {
        // extract pairs  "nodeid":{x,y}
        QRegularExpression re("\"([^\"]+)\"\\s*:\\s*\\{([^}]+)\\}");
        auto it = re.globalMatch(line);
        while (it.hasNext()) {
            auto m = it.next();
            QString nid = m.captured(1);
            QString coords = m.captured(2);

            // try x:val,y:val
            QRegularExpression named("([xy])\\s*[=:]\\s*(-?[\\d.]+)");
            auto ni = named.globalMatch(coords);
            double nx=0,ny=0; bool hasX=false,hasY=false;
            while (ni.hasNext()) {
                auto nm=ni.next();
                if (nm.captured(1)=="x"){ nx=nm.captured(2).toDouble(); hasX=true; }
                else                    { ny=nm.captured(2).toDouble(); hasY=true; }
            }
            // fallback: positional
            if (!hasX && !hasY) {
                QStringList parts=coords.split(',');
                if (parts.size()>=2){ nx=parts[0].trimmed().toDouble(); hasX=true; ny=parts[1].trimmed().toDouble(); hasY=true; }
            }

            // apply to all figures in this frame
            for (auto& fig : frame.figures) {
                Node* n = fig.findNode(nid);
                if (!n) continue;
                if (hasX) n->x=nx;
                if (hasY) n->y=ny;
            }
        }
    }
};
