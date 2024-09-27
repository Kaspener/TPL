#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTableWidget>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->log->hide();
    ui->start->hide();
    ui->slider->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::parseJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл:" << filePath;
        return false;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Ошибка парсинга JSON:" << parseError.errorString();
        return false;
    }

    if (!jsonDoc.isObject()) {
        qWarning() << "JSON не является объектом!";
        return false;
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (jsonObj.contains("states") && jsonObj["states"].isArray()) {
        QJsonArray statesArray = jsonObj["states"].toArray();
        states.clear();
        for (const QJsonValue &value : statesArray) {
            if (value.isString()) {
                states.append(value.toString());
            }
        }
        qDebug() << "States:" << states;
    }

    if (jsonObj.contains("alphabet") && jsonObj["alphabet"].isArray()) {
        QJsonArray alphabetArray = jsonObj["alphabet"].toArray();
        alphabet.clear();
        for (const QJsonValue &value : alphabetArray) {
            if (value.isString()) {
                alphabet.append(value.toString());
            }
        }
        qDebug() << "Alphabet:" << alphabet;
    }

    if (jsonObj.contains("Func") && jsonObj["Func"].isObject()) {
        QJsonObject funcObj = jsonObj["Func"].toObject();
        transitionFunction.clear();
        for (const QString &stateKey : funcObj.keys()) {
            QJsonObject transitions = funcObj[stateKey].toObject();
            QMap<QString, QString> stateTransitions;
            for (const QString &inputKey : transitions.keys()) {
                stateTransitions[inputKey] = transitions[inputKey].toString();
            }
            transitionFunction[stateKey] = stateTransitions;
        }
        qDebug() << "Function (Func):" << transitionFunction;
    }

    if (jsonObj.contains("start") && jsonObj["start"].isString()) {
        startState = jsonObj["start"].toString();
        qDebug() << "Start state:" << startState;
    }

    if (jsonObj.contains("ends") && jsonObj["ends"].isArray()) {
        QJsonArray endsArray = jsonObj["ends"].toArray();
        endStates.clear();
        for (const QJsonValue &value : endsArray) {
            if (value.isString()) {
                endStates.append(value.toString());
            }
        }
        qDebug() << "End states:" << endStates;
    }
    return true;
}

void MainWindow::populateTable()
{
    model = new QStandardItemModel(states.size(), alphabet.size(), this);

    model->setVerticalHeaderLabels(states);
    model->setHorizontalHeaderLabels(alphabet);

    for (int i = 0; i < states.size(); ++i) {
        QString state = states[i];
        for (int j = 0; j < alphabet.size(); ++j) {
            QString symbol = alphabet[j];

            if (transitionFunction.contains(state) && transitionFunction[state].contains(symbol)) {
                model->setItem(i, j, new QStandardItem(transitionFunction[state][symbol]));
            } else {
                model->setItem(i, j, new QStandardItem("λ"));
            }
        }
    }

    ui->table->setModel(model);
    ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::openFields()
{
    ui->start->setEnabled(true);
    ui->loadConfig->setEnabled(true);
    ui->command->setEnabled(true);
}

void MainWindow::on_start_clicked()
{
    ui->start->setEnabled(false);
    ui->loadConfig->setEnabled(false);
    ui->command->setEnabled(false);
    ui->log->clear();
    QString command = ui->command->text();
    QString state = startState;
    while(command.length() > 0){
        int i = states.indexOf(state);
        int j = alphabet.indexOf(command[0]);
        QString string = "<font color='green'>(" + state + ", " + command + ")</font>";
        ui->log->append(string);
        if (i == -1){
            QString error = "<font color='red'>δ(" + state + "," + command[0] + ") -> Состояния {" + state + "} не существует!!</font>";
            ui->log->append(error);
            openFields();
            return;
        }
        if (j == -1){
            QString error = "<font color='red'>δ(" + state + "," + command[0] + ") -> Символ {" + command[0] + "} не входит в алфавит!</font>";
            ui->log->append(error);
            openFields();
            return;
        }
        QStandardItem *itemToColor = model->item(i, j);
        QBrush color = itemToColor->background();
        itemToColor->setBackground(QColor(Qt::green));

        QTime dieTime= QTime::currentTime().addMSecs(ui->slider->value());
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        itemToColor->setBackground(color);
        QString left = "<font color='green'>δ(" + state + "," + command[0] + ") -> ";
        command.removeFirst();
        //if(itemToColor->text() == "λ") break;
        state = itemToColor->text();
        QString right;
        if (command.length() > 0)
            right = "Новое состояние {" + state + "} оставшаяся цепочка - " + command + "</font>";
        else
            right = "Новое состояние {" + state + "} оставшаяся цепочка - λ</font>";
        ui->log->append(left + right);
    }
    if (command.length() > 0){
        QString error = "<font color='red'>В цепочке остались символы: " + command + ". Цепочка не принадлежит заданному ДКА!</font>";
        ui->log->append(error);
        openFields();
        return;
    }
    if (!endStates.contains(state)){
        QString error = "<font color='red'>Cостояние {" + state + "} не является конечным. Цепочка не принадлежит заданному ДКА!</font>";
        ui->log->append(error);
        openFields();
        return;
    }
    ui->log->append("<font color='green'>Цепочка принадлежит заданному ДКА!</font>");
    openFields();
}


void MainWindow::on_loadConfig_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Выбрать файл", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        parseJsonFile(fileName);
        populateTable();
        ui->log->show();
        ui->log->clear();
        ui->start->show();
        ui->slider->show();
    }
}

