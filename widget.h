#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    void updateEquation(QString str = {});
    void backspaceEquation(QString str = {});
    void clearEquation();
    void calculate();

private:
    Ui::Widget *ui;
    QString m_equation;
};
#endif // WIDGET_H
