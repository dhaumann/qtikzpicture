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
    QString toTikzPath(const QRectF & rect) const;
};

QString QTikzPicturePrivate::toCoord(const QPointF & pt) const
{
    QLocale l = QLocale::c();

    return "(" + l.toString(pt.x()) + ", " + l.toString(pt.y()) + ")";
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

QString QTikzPicturePrivate::toTikzPath(const QRectF & rect) const
{
    if (rect.isEmpty()) return QString();

    const QString path = toCoord(rect.topLeft()) + " rectangle "
                       + toCoord(rect.bottomRight());
    return path;
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
    if (!d->ts) return;

    const QString tikzPath = d->toTikzPath(path);
    if (tikzPath.isEmpty()) return;

    const QString tikzCommand = options.isEmpty() ? QString("\\path ")
                                                  : ("\\path[" + options + "] ");

    (*d->ts) << tikzCommand << tikzPath << ";\n";
}

void QTikzPicture::path(const QRectF& rect, const QString& options)
{
    if (!d->ts) return;

    const QString tikzPath = d->toTikzPath(rect);
    if (tikzPath.isEmpty()) return;

    const QString tikzCommand = options.isEmpty() ? QString("\\path ")
                                                  : ("\\path[" + options + "] ");

    (*d->ts) << tikzCommand << tikzPath << ";\n";
}

void QTikzPicture::clip(const QPainterPath& path)
{
    if (!d->ts) return;

    const QString tikzPath = d->toTikzPath(path);
    if (tikzPath.isEmpty()) return;

    (*d->ts) << "\\clip " << tikzPath << ";\n";
}

void QTikzPicture::clip(const QRectF& rect)
{
    if (!d->ts) return;

    const QString tikzPath = d->toTikzPath(rect);
    if (tikzPath.isEmpty()) return;

    (*d->ts) << "\\clip " << tikzPath << ";\n";
}

void QTikzPicture::circle(const QPointF& center, qreal radius, const QString& options)
{
    if (!d->ts || radius <= 0) return;

    (*d->ts) << "\\draw";
    if (!options.isEmpty()) {
        (*d->ts) << "[" << options << "]";
    }
    (*d->ts) << " (" << center.x() << ", " << center.y() << ") circle (" << radius << "cm);\n";
}

void QTikzPicture::line(const QPointF& p, const QPointF& q, const QString& options)
{
    if (!d->ts) return;

    (*d->ts) << "\\draw";
    if (!options.isEmpty()) {
        (*d->ts) << "[" << options << "]";
    }
    (*d->ts) << " (" << p.x() << ", " << p.y() << ") -- (" << q.x() << ", " << q.y() << ");\n";
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
