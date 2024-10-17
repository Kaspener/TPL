#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QStack>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_start_clicked();

    void on_loadConfig_clicked();

private:
    Ui::MainWindow *ui;
    QStringList states;
    QStringList alphabet;
    QStringList in_stack;
    QMap<std::tuple<QString, QString, QString>, std::tuple<QString, QString>> transitionFunction;
    QString startState;
    QString startStack;
    QStringList endStates;
    QStandardItemModel *model;

    bool parseJsonFile(const QString& filePath);
    void populateList();
    void openFields();
};
#endif // MAINWINDOW_H
