QTikzPicture
============

Easily create PGF/TikZ pictures with Qt/C++.

QTikzPicture provides simple facilities to draw TikZ pictures with Qt.
It is not possible to use QTikzPicture together with QPainter, as
QTikzPicture is not a QPaintDevice.

Instead, the philosophy of QTikzPicture is to give the developer full
control over (i) what should be exported and (ii) how the drawing
should be performed.

Using QTikzPicture works on a QTextStream. Therefore, you typically
follow these steps:

    // 1. create a QTikzPicture
    QTikzPicture tikzPicture;

    // 2. assign a output text stream
    QFile file("picture.tikz");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream textStream(&file);

    tikzPicture.setStream(&textStream);

    // 3. begin tikzpicture
    tikzPicture.begin();

    // 4. call paint methods
    tikzPicture.line(QPointF(0, 0), QPointF(1, 1), "thick, dashed");

    QPainterPath path;
    path.moveTo(0, 0);
    path.moveTo(0, 1);
    path.moveTo(1, 2);
    path.moveTo(2, 1);
    path.moveTo(2, 0);
    path.closeSubpath();

    tikzPicture.path(path, "fill=green!50, draw=green!50!black");

    // 5. end tikzpicture
    tikzPicture.end();
