#include <QFont>
#include <QPainter>

#include "LabelInput.h"
#include "EntityModel.h"

LabelInput::LabelInput(EntityModel *model, QObject *parent)
    : LabelPainter(model, parent)
{

}

void LabelInput::paintHeader(QRectF &rect, QPainter *painter, const QFont &font)
{
    QRectF size;

    QPen pen = painter->pen();

    QFont f(font);
    f.setPointSize(font.pointSize());
    f.setBold(true);
    painter->setFont(f);
    painter->drawText(rect, 0, m_model->data(0, "moduleName").toString(), &size);
    rect.setTop(rect.top() + size.height() + m_padding);
    f.setBold(false);
    painter->setFont(f);
    QString address = QString("Address: %1").arg(m_model->data(0, "address").toInt());
    painter->drawText(rect, 0, address, &size);
    rect.setTop(rect.top() + size.height() + (m_padding * 5));
    pen.setWidth(3);
    painter->setPen(pen);
    painter->drawLine(rect.topLeft(), rect.topRight());
    pen.setWidth(1);
    painter->setPen(pen);
    rect.setTop(rect.top() + m_padding);
}

void LabelInput::paintBody(QRectF &rect, QPainter *painter, const QFont &font)
{
    QPen pen = painter->pen();
    QFont f(font);
    f.setFamily("Courier New");
    f.setPointSize(font.pointSize() - 2);

    f.setBold(false);
    painter->setFont(f);

    QString list[16];

    for(int x = 0; x < m_model->getRowCount(); x++)
    {
        int port = m_model->data(x, "port").toInt();
        QString text = m_model->data(x, "labelName").toString();
        if(text.trimmed().length() == 0)
            text = m_model->data(x, "deviceName").toString();
        list[port] = text;
    }

    for(int x = 0; x < 8; x++)
    {
        paintInput(rect, painter, list[x], x);
    }

    pen.setWidth(3);
    painter->setPen(pen);
    painter->drawLine(rect.topLeft(), rect.topRight());
    pen.setWidth(1);
    painter->setPen(pen);
    rect.setTop(rect.top() + 3);

    for(int x = 8; x < 16; x++)
    {
        paintInput(rect, painter, list[x], x);
    }
}

void LabelInput::paintInput(QRectF &rect, QPainter *painter, const QString &text, int port)
{
    QRectF size;
    if(port > 7)
        port = port - 8;
    QString t;
    if(text.length() > 0)
        t = QString("%1 - %2").arg(port).arg(text);
    else
        t = QString("%1 - (open)").arg(port).arg(text);
    painter->drawText(rect, 0, t, &size);
    rect.setTop(rect.top() + size.height());

    if(port != m_model->rowCount() - 1)
    {
        QPen pen = painter->pen();
        pen.setWidth(3);
        painter->setPen(pen);
        painter->drawLine(rect.topLeft(), rect.topRight());
        pen.setWidth(1);
        painter->setPen(pen);
        rect.setTop(rect.top() + m_padding);
    }
}
