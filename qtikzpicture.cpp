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

#include "tikzexport.h"

#include <QtCore/QTextStream>
#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <QtGui/QPainterPath>
#include <QtGui/QColor>

#include <QDebug>

/**
 * Private data class for QTikzPicture.
 */
class QTikzPicturePrivate
{
public:
    QTextStream* ts;
    QHash<QString, bool> m_colors;
};


QTikzPicture::QTikzPicture()
    : d(new QTikzPicturePrivate())
{
    d->ts = 0;
}

QTikzPicture::~QTikzPicture()
{
    delete d;
}

void QTikzPicture::setStream(QTextStream* textStream, int precision)
{
    ts = textStream;

    if (ts) {
        ts->setRealNumberPrecision(precision);
        ts->setRealNumberNotation(QTextStream::FixedNotation);
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

    if (!m_colors.contains(name)) {
        if (ts) {
            (*ts) << "\\definecolor{" << name << "}{rgb}{"
                  << color.redF() << ", " << color.greenF() << ", " << color.blueF() << "}\n";
        }
        m_colors[name] = true;
    }

    return name;
}

void QTikzPicture::begin(const QString& options)
{
    if (!ts) return;

    if (options.isEmpty()) {
        (*ts) << "\\begin{tikzpicture}\n";
    } else {
        (*ts) << "\\begin{tikzpicture}[" << options << "]\n";
    }
}

void QTikzPicture::end()
{
    if (!ts) return;

    (*ts) << "\\end{tikzpicture}\n";
}

void QTikzPicture::beginScope(const QString& options)
{
    if (!ts) return;

    if (options.isEmpty()) {
        (*ts) << "\\begin{scope}\n";
    } else {
        (*ts) << "\\begin{scope}[" << options << "]\n";
    }
}

void QTikzPicture::endScope()
{
    if (!ts) return;

    (*ts) << "\\end{scope}\n";
}

void QTikzPicture::newline(int count)
{
    if (!ts) return;

    for (int i = 0; i < count; ++i) {
        (*ts) << "\n";
    }
}

void QTikzPicture::comment(const QString& text)
{
    if (!ts) return;

    (*ts) << "% " << text << "\n";
}

void QTikzPicture::path(const QPainterPath& path, const QString& options)
{
    if (!ts || path.isEmpty()) return;

    int i = 0;
    for (i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                (*ts) << " -- cycle;\n";
            }
            (*ts) << "\\draw[" << options << "] (" << element.x << ", " << element.y << ")";
        } else if (element.type == QPainterPath::LineToElement) {
            (*ts) << " -- (" << element.x << ", " << element.y << ")";
        }
    }
    if (i > 0) {
        (*ts) << " -- cycle;\n";
    }
}

void QTikzPicture::path(const QRectF& rect, const QString& options)
{
    if (!ts || rect.isEmpty()) return;

    (*ts) << "\\path";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " (" << rect.left() << ", " << rect.top()
          << ") rectangle (" << rect.right() << ", " << rect.bottom() << ");\n";
}

void QTikzPicture::clip(const QPainterPath& path)
{
    if (!ts || path.isEmpty()) return;

    (*ts) << "\\clip ";

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                (*ts) << " -- cycle";
                (*ts) << "\n      ";
            }
        } else if (element.type == QPainterPath::LineToElement) {
            (*ts) << " -- ";
        } else {
            qWarning() << "QTikzPicture::clip: uknown QPainterPath segment type";
        }
        (*ts) << "(" << element.x << ", " << element.y << ")";
    }

    (*ts) << " -- cycle;\n";
}

void QTikzPicture::clip(const QRectF& rect)
{
    if (!ts || rect.isEmpty()) return;

    (*ts) << "\\clip (" << rect.left() << ", " << rect.top()
          << ") rectangle (" << rect.right() << ", " << rect.bottom() << ");";
}

void QTikzPicture::circle(const QPointF& center, qreal radius, const QString& options)
{
    if (!ts || radius <= 0) return;

    (*ts) << "\\draw";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " (" << center.x() << ", " << center.y() << ") circle (" << radius << "cm);\n";
}

void QTikzPicture::line(const QPointF& p, const QPointF& q, const QString& options)
{
    if (!ts) return;

    (*ts) << "\\draw";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " (" << p.x() << ", " << p.y() << ") -- (" << q.x() << ", " << q.y() << ");\n";
}

void QTikzPicture::line(const QVector<QPointF>& points, const QString& options)
{
    if (!ts || points.size() < 2) return;

    (*ts) << "\\draw";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    const int size = points.size();
    for (int i = 0; i < size - 1; ++i) {
        (*ts) << " (" << points[i].x() << ", " << points[i].y() << ") --";
    }
    (*ts) << " (" << points[size-1].x() << ", " << points[size-1].y() << ");\n";
}

QTikzPicture& QTikzPicture::operator<< (const QString& text)
{
    if (!ts || text.isEmpty()) return *this;
    (*ts) << text;
    return *this;
}

QTikzPicture& QTikzPicture::operator<< (const char* text)
{
    return operator<<(QString(text));
}

QTikzPicture& QTikzPicture::operator<< (double number)
{
    if (ts) (*ts) << number;
    return *this;
}

QTikzPicture& QTikzPicture::operator<< (int number)
{
    if (ts) (*ts) << number;
    return *this;
}

// kate: replace-tabs on; indent-width 4;
