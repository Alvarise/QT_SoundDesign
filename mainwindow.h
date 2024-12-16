#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QCalendarWidget>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QDate>
#include <QSqlError>
#include <QMessageBox>
#include <QDateTime>
#include <QCheckBox>
#include <QListView>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleValidator>
#include <QDebug>
#include <QStandardItem>

class AddEventDialog : public QDialog {
    Q_OBJECT

public:
    AddEventDialog(QWidget *parent = nullptr);
    ~AddEventDialog();

    QString getTitle() const;
    QString getDescription() const;
    double getPrice() const;
    QString getPlace() const;
    QString getTime() const;

private:
    QLineEdit *titleEdit;
    QLineEdit *descriptionEdit;
    QLineEdit *priceEdit;
    QLineEdit *placeEdit;
    QLineEdit *timeEdit;
    QPushButton *saveButton;
    QPushButton *cancelButton;

};

class EventManager : public QMainWindow {
    Q_OBJECT

public:
    EventManager(QWidget *parent = nullptr);
    ~EventManager();

private:
    void setupDatabase();
    void loadEventsForDate(const QDate &date);
    void updateTotalEarningsForMonth(int year, int month);
    void onItemChanged(QStandardItem *item);
    void updateTotalEarnings();
    void addEventToDatabase(const QString &title, const QString &description, double price, const QString &place, const QString &time);
    void removeEventFromDatabase(int eventId);
    
    QSqlDatabase db;
    QCalendarWidget *calendar;
    QTableView *eventListView;
    QStandardItemModel *eventModel;
    QLabel *totalEarningsLabel;
    QDate currentDate;
    double totalEarnings = 0.0; 

private slots:
    void onDateSelected(const QDate &date);
    void onAddEvent();
    void onRemoveEvent();
};

#endif // MAINWINDOW_H
