#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QVBoxLayout>

#include "ControllerTab.h"

ControllerTab::ControllerTab(QWidget *parent) : QWidget(parent)
{
    setupModel();
    setupUI();
}

void ControllerTab::selectionChanged()
{
    QModelIndex i = tableModel->index(tableView->currentIndex().row(), 1);
    int controllerID = tableModel->data(i, Qt::EditRole).toInt();
    qDebug(QString("Controller %1 selected").arg(controllerID).toLatin1());
    emit currentControllerIDChanged(controllerID);
}

void ControllerTab::setupModel()
{
    tableModel = new QSqlRelationalTableModel(this, db.getDatabase());
    tableModel->setTable("controller");
    tableModel->setRelation(3, QSqlRelation("classCode", "id", "className"));

    tableModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Serial #"));
    tableModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID"));
    tableModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Name"));
    tableModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Class/Type"));
    tableModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Description"));

    tableModel->select();
}

void ControllerTab::setupUI()
{
    tableView = new QTableView;
    tableView->setModel(tableModel);
    tableView->setItemDelegate(new QSqlRelationalDelegate(tableView));

    connect(tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectionChanged()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(tableView);
    this->setLayout(layout);
}

