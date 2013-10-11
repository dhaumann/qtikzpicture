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

#ifndef QT_TIKZ_PICTURE_H
#define QT_TIKZ_PICTURE_H

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVector>

class QTextStream;
class QColor;
class QPointF;
class QRectF;
class QPainterPath;
class QTikzPicturePrivate;

/**
 * @brief Export drawing primitives to PGF/TikZ.
 *
 * @section qtikzpicture_intro Introduction
 *
 * QTikzPicture provides simple facilities to draw TikZ picture with Qt.
 * It is not possible to use QTikzPicture together with QPainter, as
 * QTikzPicture is not a QPaintDevice.
 *
 * Instead, the philosophy of QTikzPicture is to give the developer full
 * control over (i) \e what should be exported and (ii) \e how the drawing
 * should be performed.
 *
 * @section qtikzpicture_usage Using QTikzPicture
 *
 * Using QTikzPicture works on a QTextStream. Therefore, you typically
 * follow these steps:
 * \code
 * // 1. create a QTikzPicture
 * QTikzPicture tikzPicture;
 *
 * // 2. assign a output text stream
 * QFile file("picture.tikz");
 * if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
 *     return;
 * QTextStream textStream(&file);
 *
 * tikzPicture.setStream(&textStream);
 *
 * // 3. begin tikzpicture
 * tikzPicture.begin();
 *
 * // 4. call paint methods
 * tikzPicture.line(QPointF(0, 0), QPointF(1, 1), "thick, dashed");
 *
 * QPainterPath path;
 * path.moveTo(0, 0);
 * path.moveTo(0, 1);
 * path.moveTo(1, 2);
 * path.moveTo(2, 1);
 * path.moveTo(2, 0);
 * path.closeSubpath();
 *
 * tikzPicture.path(path, "fill=green!50, draw=green!50!black");
 *
 * // 5. end tikzpicture
 * tikzPicture.end();
 * \endcode
 *
 * @author Dominik Haumann \<dhaumann@kde.org\>
 */
class QTikzPicture
{
public:
    QTikzPicture();
    virtual ~QTikzPicture();

    /**
     * Set the output text stream to @p textStream.
     * Optionally, a precision of floating point numbers can specified.
     * A value of 2 implies numbers in the format '2.34'.
     *
     * @param textStream output text stream, must be a valid pointer
     * @param precision floating poing precision
     */
    void setStream(QTextStream* textStream, int precision = 2);

    /**
     * PGF/TikZ knows predefined colors such as 'red', 'green' etc.
     * If you want to use arbitrary QColors, you first need to create
     * a color by calling registerColor().
     *
     * The following example draws @p path with the color
     * QColor(100, 200, 0):
     * @code
     * QString col = tikzPicture.registerColor(QColor(100, 200, 0));
     * tikzPicture.path(path, "draw=" + col);
     * @endcode
     *
     * Calling this function for the same color multiple times is
     * supported and returns always the same unique identifier.
     *
     * @param color the color to register
     */
    QString registerColor(const QColor& color);

    /**
     * Calling begin() is required before calling the first painting
     * routine. A call of begin() with optional @p options creates a new TikZ
     * picture environment in terms of
     * @code
     * \begin{tikzpicture}[options]
     * @endcode
     *
     * After the last painting method, you have to call end() once to
     * close the tikzpicture.
     *
     * @param options optional options valid in the scope
     */
    void begin(const QString& options = QString());

    /**
     * Ends the TikZ picture by writing
     * @code
     * \end{tikzpicture}
     * @endcode
     * to the output text stream. Therefore, call this function only once.
     */
    void end();

    /**
     * Calling begin() with optional @p options creates a new TikZ
     * scope in terms of
     * @code
     * \begin{scope}[options]
     * % ...
     * \end{scope}
     * @endcode
     *
     * For each call of beginScope() you have to call endScope() at some
     * point to obtain a valid TikZ picture.
     *
     * @param options optional options valid in the scope
     */
    void beginScope(const QString& options = QString());

    /**
     * Calling endScope() ends a scope started with beginScope().
     */
    void endScope();

    /**
     * Insert @p count newline characters.
     * You can use this to manually structure your TikZ picture.
     *
     * @param count amount of newlines to add
     */
    void newline(int count = 1);

    /**
     * Optionally, you can add comments with this function.
     * Calling comment("Hello World!") will output the text
     * '% Hello World!\n' to the output stream.
     *
     * @param text the comment
     */
    void comment(const QString& text);

    /**
     * Write the painter path @p path to the output stream.
     * Optionally, you can pass @p options for drawing the path.
     *
     * @param path path to draw
     * @param options optional drawing options
     */
    void path(const QPainterPath& path, const QString& options = QString());

    /**
     * Write the rectangle @p rect to the output stream.
     * Optionally, you can pass @p options for drawing the rect.
     *
     * @param rect rect to draw
     * @param options optional drawing options
     */
    void path(const QRectF& rect, const QString& options = QString());

    /**
     * Clip according to the painter path specified in @p path.
     * This function is useful in combination with beginScope() and endScope().
     *
     * @param path clipping path
     */
    void clip(const QPainterPath& path);

    /**
     * Clip according to the rectangle specified in @p rect.
     * This function is useful in combination with beginScope() and endScope().
     *
     * @param path clipping rect
     */
    void clip(const QRectF& rect);

    /**
     * Draw a circle at @p center with radius @p radius and optional @p options.
     *
     * @param center center of the circle
     * @param radius radius of the circle
     * @param options optional drawing options
     */
    void circle(const QPointF& center, qreal radius, const QString& options = QString());

    /**
     * Draw a line from @p p to @p q with optional @p options.
     *
     * @param p starting point of the line
     * @param q end point of the line
     * @param options optional drawing options
     */
    void line(const QPointF& p, const QPointF& q, const QString& options = QString());

    /**
     * Draw the polygonal line @p points with optional @p options.
     *
     * @param points point list defining the polygon
     * @param options optional drawing options
     */
    void line(const QVector<QPointF>& points, const QString& options = QString());

    /**
     * For convenience, use the << operator to write @p text directly to the output
     * stream. This gives full control over the text written to the TikZ picture.
     *
     * @param text text to write
     */
    QTikzPicture& operator<< (const QString& text);

    /**
     * This function is an overload and provided for convenience.
     */
    QTikzPicture& operator<< (const char* text);

    /**
     * This operator writes the floating point number @p number directly
     * to the output stream. The value of @p number is rounded according
     * to the precision set in setStream().
     *
     * @param number number to write
     */
    QTikzPicture& operator<< (double number);

    /**
     * This operator writes the integer @p number directly to the output
     * stream. The value of @p number is rounded according to the precision
     * set in setStream().
     *
     * @param number number to write
     */
    QTikzPicture& operator<< (int number);

private:
    QTikzPicturePrivate * const d;
};

#endif // QT_TIKZ_PICTURE_H

// kate: replace-tabs on; indent-width 4;
