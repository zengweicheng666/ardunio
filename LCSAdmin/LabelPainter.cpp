#include <QPrinter>
#include <QPainter>
#include <QApplication>

#include "LabelPainter.h"
#include "EntityModel.h"
#include "GlobalDefs.h"

LabelPainter::LabelPainter(EntityModel *model, QObject *parent)
    : QObject(parent), m_model(model), m_padding(5)
{
    QFont f = QApplication::font();

    f.setPointSize(12);
    setFont(f);
}

void LabelPainter::setFont(const QFont &font)
{
    if(m_font != font)
    {
        m_font = font;
        emit fontChanged();
    }
}

void LabelPainter::printerPaintRequested(QPrinter *printer)
{
    QPainter painter(printer);
    QRectF r = printer->pageRect();

    QPen pen;
    pen.setColor(Qt::black);
    painter.setPen(pen);

    paintHeader(r, &painter, getFont());
    if(r.y()  > printer->height())
    {
        printer->newPage();
        r = printer->pageRect();
    }
    paintBody(r, &painter, getFont());
    if(r.y()  > printer->height())
    {
        printer->newPage();
        r = printer->pageRect();
    }
}

QString LabelPainter::getDeviceTypeName(int deviceType)
{
    QString ret;
    /*
    DeviceUnknown,
    DeviceTurnout,
    DevicePanelInput,
    DevicePanelOutput,
    DeviceSignal = 4,
    DeviceSemaphore = 5,
    DeviceBlock = 6, */

    switch (deviceType) {
    case DeviceTurnout:
        ret = "Turnout";
        break;
    case DevicePanelInput:
        ret = "Panel Input";
        break;
    case DevicePanelOutput:
        ret = "Panel Output";
        break;
    case DeviceSignal:
        ret = "Signal";
        break;
    case DeviceSemaphore:
        ret = "Semaphore";
        break;
    case DeviceBlock:
        ret = "Block";
        break;
    default:
        break;
    }
    return ret;
}
