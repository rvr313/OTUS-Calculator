#include "widget.h"
#include "./ui_widget.h"
#include "equation.h"
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    connect(ui->pBtn_Cancel, &QPushButton::clicked, [this]() { clearEquation(); });
    connect(ui->pBtn_Backspace, &QPushButton::clicked, [this]() { backspaceEquation(); });
    connect(ui->pBtn_0, &QPushButton::clicked, [this]() { updateEquation("0"); });
    connect(ui->pBtn_1, &QPushButton::clicked, [this]() { updateEquation("1"); });
    connect(ui->pBtn_2, &QPushButton::clicked, [this]() { updateEquation("2"); });
    connect(ui->pBtn_3, &QPushButton::clicked, [this]() { updateEquation("3"); });
    connect(ui->pBtn_4, &QPushButton::clicked, [this]() { updateEquation("4"); });
    connect(ui->pBtn_5, &QPushButton::clicked, [this]() { updateEquation("5"); });
    connect(ui->pBtn_6, &QPushButton::clicked, [this]() { updateEquation("6"); });
    connect(ui->pBtn_7, &QPushButton::clicked, [this]() { updateEquation("7"); });
    connect(ui->pBtn_8, &QPushButton::clicked, [this]() { updateEquation("8"); });
    connect(ui->pBtn_9, &QPushButton::clicked, [this]() { updateEquation("9"); });
    connect(ui->pBtn_Point, &QPushButton::clicked, [this]() { updateEquation("."); });
    connect(ui->pBtn_OpAdd, &QPushButton::clicked, [this]() { updateEquation(" + "); });
    connect(ui->pBtn_OpSub, &QPushButton::clicked, [this]() { updateEquation(" - "); });
    connect(ui->pBtn_OpMult, &QPushButton::clicked, [this]() { updateEquation(" * "); });
    connect(ui->pBtn_OpDiv, &QPushButton::clicked, [this]() { updateEquation(" / "); });
    connect(ui->pBtn_leftBrace, &QPushButton::clicked, [this]() { updateEquation("("); });
    connect(ui->pBtn_rightBrace, &QPushButton::clicked, [this]() { updateEquation(")"); });
    connect(ui->pBtn_FuncPow2, &QPushButton::clicked, [this]() { updateEquation(" ^ 2"); });
    connect(ui->pBtn_FuncSqrt, &QPushButton::clicked, [this]() { updateEquation("sqrt("); });
    connect(ui->pBtn_Calculate, &QPushButton::clicked, [this]() { calculate(); });
    connect(ui->equationLine, &QLineEdit::textEdited, [this](const QString &str) { m_equation = str; });
}

Widget::~Widget()
{
    delete ui;
}

void Widget::updateEquation(QString str)
{
    m_equation.append(str);
    QSignalBlocker blocker(ui->equationLine);
    ui->equationLine->setText(m_equation);
}

void Widget::backspaceEquation(QString str)
{
    m_equation.chop(1);
    QSignalBlocker blocker(ui->equationLine);
    ui->equationLine->setText(m_equation);
}

void Widget::clearEquation()
{
    m_equation.clear();
    QSignalBlocker blocker(ui->equationLine);
    ui->equationLine->clear();
    ui->errorField->clear();
}

void Widget::calculate()
{
    if (m_equation.isEmpty()) {
        return;
    }

    ui->errorField->clear();

    calc::Result res = calc::calculate(m_equation.toLatin1());
    if (res.ok) {
        QString res_str = QString::number(res.result);
        m_equation.clear();

        QSignalBlocker blocker(ui->equationLine);
        updateEquation(res_str);

    } else {
        ui->errorField->setText(res.what.c_str());
    }
}
