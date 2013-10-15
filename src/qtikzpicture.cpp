/*  Copyright (c) 2012-2013, Dominik Haumann <dhaumann@kde.org>
    All rights reserved.

    License: FreeBSD License

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "qtikzpicture.h"

#include <QTextStream>
#include <QPointF>
#include <QRectF>
#include <QLineF>

#include <QPainterPath>
#include <QColor>

#include <QLocale>

#include <QDebug>

/**
 * Private data class for QTikzPicture.
 */
class QTikzPicturePrivate
{
public:
    QTextStream* ts;
    QHash<QString, bool> colors;
    int precision;

public:
    inline QString toCoord(const QPointF & pt) const;
    QString toTikzPath(const QPainterPath & path) const;
    QString toTikzPath(const QPolygonF & polygon) const;
    QString toTikzPath(const QRectF & rect) const;
    QString toTikzPath(const QLineF & line) const;
    QString toTikzPath(const QPointF & circleCenter, qreal radius) const;

    void writePath(const QString & cmd, const QString & options, const QString & path);
};

QString QTikzPicturePrivate::toCoord(const QPointF & pt) const
{
    QLocale l = QLocale::c();

    return "(" + l.toString(pt.x(), 'g', precision) + ", "
               + l.toString(pt.y(), 'g', precision) + ")";
}

QString QTikzPicturePrivate::toTikzPath(const QPainterPath & path) const
{
    if (path.isEmpty()) return QString();

    QStringList pathList;
    QString currentPath;
    int currentControlPoint = 0;

    // convert QPainterPath to a TikZ path string
    for (int i = 0; i < path.elementCount(); i++) {
        const QPainterPath::Element & element = path.elementAt(i);

        switch (element.type) {
            case QPainterPath::MoveToElement: {
                // close current path + append to path list
                if (!currentPath.isEmpty()) {
                    currentPath += " -- cycle";
                    pathList << currentPath;
                }

                // indent with spaces for better readability
                const char * indentString = pathList.count() ? "    " : "";

                // start new path
                currentPath.clear();
                currentPath += indentString + toCoord(element);
                break;
            }
            case QPainterPath::LineToElement: {
                currentPath += " -- " + toCoord(element);
                break;
            }
            case QPainterPath::CurveToElement: {
                currentPath += " .. controls " + toCoord(element);
                currentControlPoint = 1;
                break;
            }
            case QPainterPath::CurveToDataElement: {
                if (currentControlPoint == 1) {
                    currentPath += " and " + toCoord(element);
                    ++currentControlPoint;
                } else if (currentControlPoint == 2) {
                    currentPath += " .. " + toCoord(element);
                    currentControlPoint = 0;
                }
                break;
            }
        }
    }

    return pathList.join("\n");
}

QString QTikzPicturePrivate::toTikzPath(const QPolygonF & polygon) const
{
    if (polygon.isEmpty()) return QString();

    const int end = polygon.size() - polygon.isClosed() ? 1 : 0;

    QString path;
    for (int i = 0; i < end; ++i) {
        if (i == 0) {
            path = toCoord(polygon[i]);
        } else if (i == end - 1) {
            path += " -- cycle";
        } else {
            path += " -- " + toCoord(polygon[i]);
        }
    }

    return path;
}

QString QTikzPicturePrivate::toTikzPath(const QRectF & rect) const
{
    if (rect.isEmpty()) return QString();

    const QString path = toCoord(rect.topLeft()) + " rectangle "
                       + toCoord(rect.bottomRight());
    return path;
}

QString QTikzPicturePrivate::toTikzPath(const QLineF & line) const
{
    const QString path = toCoord(line.p1()) + " -- " + toCoord(line.p2());
    return path;
}

QString QTikzPicturePrivate::toTikzPath(const QPointF & circleCenter, qreal radius) const
{
    if (radius < 0) return QString();

    QLocale l = QLocale::c();
    const QString path = toCoord(circleCenter) + " circle (" + l.toString(radius, 'g', precision) + "cm)";
    return path;
}

void QTikzPicturePrivate::writePath(const QString & cmd, const QString & options, const QString & path)
{
    if (! ts) return;
    if (path.isEmpty()) return;

    (*ts) << cmd;
    if (! options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " " << path << ";\n";
}







QTikzPicture::QTikzPicture()
    : d(new QTikzPicturePrivate())
{
    d->ts = 0;
    d->precision = 2;
}

QTikzPicture::~QTikzPicture()
{
    delete d;
}

void QTikzPicture::setStream(QTextStream* textStream, int precision)
{
    d->ts = textStream;
    d->precision = qMax(0, precision);

    if (d->ts) {
        d->ts->setRealNumberPrecision(precision);
        d->ts->setRealNumberNotation(QTextStream::FixedNotation);
    }
}

QString QTikzPicture::registerColor(const QColor& color)
{
    // some predefined colors
    if (color == Qt::red) return "red";
    if (color == Qt::green) return "green";
    if (color == Qt::blue) return "blue";
    if (color == Qt::black) return "black";
    if (color == Qt::white) return "white";
    if (color == Qt::cyan) return "cyan";
    if (color == Qt::magenta) return "magenta";
    if (color == Qt::yellow) return "yellow";

    QString name = color.name();
    if (name.startsWith('#')) name.remove(0, 1);

    name.replace("0", "q", Qt::CaseInsensitive);
    name.replace("1", "r", Qt::CaseInsensitive);
    name.replace("2", "s", Qt::CaseInsensitive);
    name.replace("3", "t", Qt::CaseInsensitive);
    name.replace("4", "u", Qt::CaseInsensitive);
    name.replace("5", "v", Qt::CaseInsensitive);
    name.replace("6", "w", Qt::CaseInsensitive);
    name.replace("7", "x", Qt::CaseInsensitive);
    name.replace("8", "y", Qt::CaseInsensitive);
    name.replace("9", "z", Qt::CaseInsensitive);

    name = 'c' + name;

    if (!d->colors.contains(name)) {
        if (d->ts) {
            (*d->ts) << "\\definecolor{" << name << "}{rgb}{"
                  << color.redF() << ", " << color.greenF() << ", " << color.blueF() << "}\n";
        }
        d->colors[name] = true;
    }

    return name;
}

void QTikzPicture::begin(const QString& options)
{
    if (!d->ts) return;

    if (options.isEmpty()) {
        (*d->ts) << "\\begin{tikzpicture}\n";
    } else {
        (*d->ts) << "\\begin{tikzpicture}[" << options << "]\n";
    }
}

void QTikzPicture::end()
{
    if (!d->ts) return;

    (*d->ts) << "\\end{tikzpicture}\n";
}

void QTikzPicture::beginScope(const QString& options)
{
    if (!d->ts) return;

    if (options.isEmpty()) {
        (*d->ts) << "\\begin{scope}\n";
    } else {
        (*d->ts) << "\\begin{scope}[" << options << "]\n";
    }
}

void QTikzPicture::endScope()
{
    if (!d->ts) return;

    (*d->ts) << "\\end{scope}\n";
}

void QTikzPicture::newline(int count)
{
    if (!d->ts) return;

    for (int i = 0; i < count; ++i) {
        (*d->ts) << "\n";
    }
}

void QTikzPicture::comment(const QString& text)
{
    if (!d->ts) return;

    (*d->ts) << "% " << text << "\n";
}

void QTikzPicture::path(const QPainterPath& path, const QString& options)
{
    d->writePath("\\path", options, d->toTikzPath(path));
}

void QTikzPicture::path(const QRectF& rect, const QString& options)
{
    d->writePath("\\path", options, d->toTikzPath(rect));
}

void QTikzPicture::path(const QPolygonF& polygon, const QString& options)
{
    d->writePath("\\path", options, d->toTikzPath(polygon));
}

void QTikzPicture::path(const QLineF& line, const QString& options)
{
    d->writePath("\\path", options, d->toTikzPath(line));
}

void QTikzPicture::path(const QPointF& p1, const QPointF & p2, const QString& options)
{
    d->writePath("\\path", options, d->toTikzPath(QLineF(p1, p2)));
}

void QTikzPicture::path(const QPointF& circleCenter, qreal radius, const QString& options)
{
    d->writePath("\\path", options, d->toTikzPath(circleCenter, radius));
}


void QTikzPicture::draw(const QPainterPath& path, const QString& options)
{
    d->writePath("\\draw", options, d->toTikzPath(path));
}

void QTikzPicture::draw(const QRectF& rect, const QString& options)
{
    d->writePath("\\draw", options, d->toTikzPath(rect));
}

void QTikzPicture::draw(const QPolygonF& polygon, const QString& options )
{
    d->writePath("\\draw", options, d->toTikzPath(polygon));
}

void QTikzPicture::draw(const QLineF& line, const QString& options)
{
    d->writePath("\\draw", options, d->toTikzPath(line));
}

void QTikzPicture::draw(const QPointF& p1, const QPointF & p2, const QString& options)
{
    d->writePath("\\draw", options, d->toTikzPath(QLineF(p1, p2)));
}

void QTikzPicture::draw(const QPointF& circleCenter, qreal radius, const QString& options)
{
    d->writePath("\\draw", options, d->toTikzPath(circleCenter, radius));
}


void QTikzPicture::fill(const QPainterPath& path, const QString& options)
{
    d->writePath("\\fill", options, d->toTikzPath(path));
}

void QTikzPicture::fill(const QRectF& rect, const QString& options)
{
    d->writePath("\\fill", options, d->toTikzPath(rect));
}

void QTikzPicture::fill(const QPolygonF& polygon, const QString& options )
{
    d->writePath("\\fill", options, d->toTikzPath(polygon));
}

void QTikzPicture::fill(const QLineF& line, const QString& options)
{
    d->writePath("\\fill", options, d->toTikzPath(line));
}

void QTikzPicture::fill(const QPointF& p1, const QPointF & p2, const QString& options)
{
    d->writePath("\\fill", options, d->toTikzPath(QLineF(p1, p2)));
}

void QTikzPicture::fill(const QPointF& circleCenter, qreal radius, const QString& options)
{
    d->writePath("\\fill", options, d->toTikzPath(circleCenter, radius));
}


void QTikzPicture::clip(const QPainterPath& path)
{
    d->writePath("\\clip", QString(), d->toTikzPath(path));
}

void QTikzPicture::clip(const QRectF& rect)
{
    d->writePath("\\clip", QString(), d->toTikzPath(rect));
}


void QTikzPicture::line(const QVector<QPointF>& points, const QString& options)
{
    if (!d->ts || points.size() < 2) return;

    (*d->ts) << "\\draw";
    if (!options.isEmpty()) {
        (*d->ts) << "[" << options << "]";
    }
    const int size = points.size();
    for (int i = 0; i < size - 1; ++i) {
        (*d->ts) << " (" << points[i].x() << ", " << points[i].y() << ") --";
    }
    (*d->ts) << " (" << points[size-1].x() << ", " << points[size-1].y() << ");\n";
}

QTikzPicture& QTikzPicture::operator<< (const QString& text)
{
    if (!d->ts || text.isEmpty()) return *this;
    (*d->ts) << text;
    return *this;
}

QTikzPicture& QTikzPicture::operator<< (const char* text)
{
    return operator<<(QString(text));
}

QTikzPicture& QTikzPicture::operator<< (double number)
{
    if (d->ts) (*d->ts) << number;
    return *this;
}

QTikzPicture& QTikzPicture::operator<< (int number)
{
    if (d->ts) (*d->ts) << number;
    return *this;
}

// kate: replace-tabs on; indent-width 4;
