#include "mainwindow.h"
#include <QApplication>
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QStandardItem>
#include <QPushButton>
#include <QDate>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QCalendarWidget>
#include <QTableView>
#include <QLabel>
#include <QSqlDatabase>
#include <QDateTime>
#include <QBrush>

AddEventDialog::AddEventDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add Event");

    QFormLayout *formLayout = new QFormLayout;
    titleEdit = new QLineEdit(this);
    descriptionEdit = new QLineEdit(this);
    priceEdit = new QLineEdit(this);
    placeEdit = new QLineEdit(this);
    timeEdit = new QLineEdit(this);

    priceEdit->setValidator(new QDoubleValidator(0, 10000, 2, this));

    formLayout->addRow("Title:", titleEdit);
    formLayout->addRow("Description:", descriptionEdit);
    formLayout->addRow("Price:", priceEdit);
    formLayout->addRow("Place:", placeEdit);
    formLayout->addRow("Time:", timeEdit);

    saveButton = new QPushButton("Save", this);
    cancelButton = new QPushButton("Cancel", this);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(saveButton, &QPushButton::clicked, this, &AddEventDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &AddEventDialog::reject);
    setFixedSize(1000, 1000);
}

AddEventDialog::~AddEventDialog() {}

QString AddEventDialog::getTitle() const { return titleEdit->text(); }
QString AddEventDialog::getDescription() const { return descriptionEdit->text(); }
double AddEventDialog::getPrice() const { return priceEdit->text().toDouble(); }
QString AddEventDialog::getPlace() const { return placeEdit->text(); }
QString AddEventDialog::getTime() const { return timeEdit->text(); }

EventManager::EventManager(QWidget *parent)
    : QMainWindow(parent),
      calendar(new QCalendarWidget(this)),
      eventListView(new QTableView(this)),
      eventModel(new QStandardItemModel(0, 7, this)), // Adjusted to 7 columns (including the Completed checkbox)
      currentDate(QDate::currentDate())
{
    setWindowTitle("Event Manager");
    setFixedSize(1000, 1000);

    // Создаем основные элементы интерфейса
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(calendar);
    layout->addWidget(eventListView);

    QPushButton *addButton = new QPushButton("Add Event", this);
    QPushButton *removeButton = new QPushButton("Remove Event", this);
    layout->addWidget(addButton);
    layout->addWidget(removeButton);

    totalEarningsLabel = new QLabel("Total Earnings: 0.00", this);
    layout->addWidget(totalEarningsLabel);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    eventModel->setHorizontalHeaderLabels({"Title", "Description", "Price", "Place", "Time", "Event ID", "Completed"});
    eventListView->setModel(eventModel);

    connect(calendar, &QCalendarWidget::clicked, this, &EventManager::onDateSelected);
    connect(addButton, &QPushButton::clicked, this, &EventManager::onAddEvent);
    connect(removeButton, &QPushButton::clicked, this, &EventManager::onRemoveEvent);
    connect(eventModel, &QStandardItemModel::itemChanged, this, &EventManager::onItemChanged);

    setupDatabase();
    loadEventsForDate(currentDate);
}

EventManager::~EventManager()
{
    db.close();
}

void EventManager::setupDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("events.db");

    if (!db.open())
    {
        QMessageBox::critical(this, "Database Error", "Failed to open database: " + db.lastError().text());
        return;
    }

    // Обновленный запрос для создания таблицы с колонкой 'completed'
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS events ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "title TEXT, "
               "description TEXT, "
               "price REAL, "
               "place TEXT, "
               "time TEXT, "
               "eventDate TEXT, "
               "completed INTEGER DEFAULT 0)");

    if (query.lastError().isValid())
    {
        QMessageBox::critical(this, "Database Error", "Failed to create table: " + query.lastError().text());
    }
}

void EventManager::loadEventsForDate(const QDate &date)
{
    eventModel->clear();
    eventModel->setHorizontalHeaderLabels({"Title", "Description", "Price", "Place", "Time", "Event ID", "Completed"});

    // Fetch events from database for the selected date
    QSqlQuery query;
    query.prepare("SELECT id, title, description, price, place, time, eventDate, completed FROM events WHERE eventDate = ? ORDER BY time ASC");
    query.addBindValue(date.toString(Qt::ISODate));
    query.exec();

    while (query.next())
    {
        int eventId = query.value(0).toInt();
        QString title = query.value(1).toString();
        QString description = query.value(2).toString();
        double price = query.value(3).toDouble();
        QString place = query.value(4).toString();
        QString time = query.value(5).toString().trimmed();      // Time
        QString eventDate = query.value(6).toString().trimmed(); // Date
        bool completed = query.value(7).toBool();                // Completed status

        // Combine date and time into QDateTime
        QDateTime eventDateTime = QDateTime::fromString(eventDate + " " + time, "yyyy-MM-dd HH:mm");

        QList<QStandardItem *> row;
        row.append(new QStandardItem(title));
        row.append(new QStandardItem(description));
        row.append(new QStandardItem(QString::number(price)));
        row.append(new QStandardItem(place));
        row.append(new QStandardItem(time)); // Time displayed in the table
        QStandardItem *eventIdItem = new QStandardItem(QString::number(eventId));
        eventIdItem->setData(eventId, Qt::UserRole); // Store event ID in the item data
        row.append(eventIdItem);

        // Check if the event time has passed and if it's not marked as completed
        if (eventDateTime < QDateTime::currentDateTime() && !completed)
        {
            // Highlight row red if event time has passed and it's not completed
            for (QStandardItem *item : row)
            {
                item->setBackground(QBrush(Qt::red)); // Make row red
            }
        }

        if (completed)
        {
            for (QStandardItem *item : row)
            {
                item->setBackground(QBrush(Qt::green)); // Make row green
            }
        }

        // Add a checkbox for the completed status
        QStandardItem *completedItem = new QStandardItem();
        completedItem->setCheckable(true);
        completedItem->setData(completed ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        row.append(completedItem);
        eventModel->appendRow(row);
    }
}

// Метод для обновления общей суммы заработка, учитывая выбранный месяц
void EventManager::updateTotalEarningsForMonth(int year, int month)
{
    // Получаем первый день выбранного месяца
    QDate firstDayOfMonth(year, month, 1);

    // Получаем первый день следующего месяца
    QDate firstDayOfNextMonth = firstDayOfMonth.addMonths(1);

    // Выполняем запрос для получения общей суммы за выбранный месяц с учетом "completed = 1"
    QSqlQuery query;
    query.prepare("SELECT SUM(price) FROM events WHERE completed = 1 AND eventDate >= ? AND eventDate < ?");
    query.addBindValue(firstDayOfMonth.toString(Qt::ISODate));     // Начало месяца
    query.addBindValue(firstDayOfNextMonth.toString(Qt::ISODate)); // Начало следующего месяца

    if (query.exec())
    {
        if (query.next())
        {
            // Получаем сумму и обновляем отображение заработка
            totalEarnings = query.value(0).toDouble();
            totalEarningsLabel->setText(QString("Total Earnings: %1").arg(totalEarnings));
        }
    }
    else
    {
        QMessageBox::critical(this, "Database Error", "Failed to calculate total earnings: " + query.lastError().text());
    }
}
// Вызов метода для обновления общей суммы после изменения состояния чекбокса
void EventManager::onItemChanged(QStandardItem *item)
{
    // Если изменяется колонка с чекбоксом (индекс колонки 6, где хранятся чекбоксы)
    if (item->column() == 6)
    {
        bool isChecked = (item->checkState() == Qt::Checked); // Проверяем, выбран ли чекбокс
        double eventPrice = item->index().sibling(item->index().row(), 2).data(Qt::DisplayRole).toDouble();

        // Получаем ID события
        int eventId = item->index().sibling(item->index().row(), 5).data(Qt::UserRole).toInt();

        // Обновляем поле 'completed' в базе данных
        QSqlQuery query;
        query.prepare("UPDATE events SET completed = ? WHERE id = ?");
        query.addBindValue(isChecked ? 1 : 0); // Если чекбокс выбран, то 1, если нет — 0
        query.addBindValue(eventId);

        if (!query.exec())
        {
            QMessageBox::critical(this, "Database Error", "Failed to update event completion: " + query.lastError().text());
            return;
        }

        // Если чекбокс установлен, добавляем цену в общий заработок
        if (isChecked)
        {
            totalEarnings += eventPrice;
        }
        else
        {
            // Если чекбокс снят, вычитаем цену из общего заработка
            totalEarnings -= eventPrice;
        }

        // Обновляем метку с общей суммой
        updateTotalEarnings(); // Обновляем заработок за месяц

        // Обновляем фон записи на зелёный, если галочка установлена
        if (isChecked)
        {
            // Получаем всю строку
            QModelIndex index = item->index();
            QStandardItemModel *model = qobject_cast<QStandardItemModel *>(item->model());
            if (model)
            {
                QList<QStandardItem *> row = model->takeRow(index.row());
                for (QStandardItem *rowItem : row)
                {
                    rowItem->setBackground(QBrush(Qt::green)); // Устанавливаем зелёный фон для всех элементов в строке
                }
                model->insertRow(index.row(), row); // Вставляем строку обратно в модель
            }
        }
        else
        {
            // Если галочка снята, можно сделать фон нейтральным или вернуть прежний
            QModelIndex index = item->index();
            QStandardItemModel *model = qobject_cast<QStandardItemModel *>(item->model());
            if (model)
            {
                QList<QStandardItem *> row = model->takeRow(index.row());
                for (QStandardItem *rowItem : row)
                {
                    rowItem->setBackground(QBrush(Qt::white)); // Возвращаем фон в обычный
                }
                model->insertRow(index.row(), row); // Вставляем строку обратно в модель
            }
        }
    }
}
void EventManager::updateTotalEarnings()
{
    // Получаем текущую дату и первый день текущего месяца
    QDate currentDate = QDate::currentDate();
    QDate firstDayOfMonth(currentDate.year(), currentDate.month(), 1);

    // Создаем запрос для получения суммы всех выполненных событий за текущий месяц
    QSqlQuery query;
    query.prepare("SELECT SUM(price) FROM events WHERE completed = 1 AND eventDate >= ? AND eventDate < ?");
    query.addBindValue(firstDayOfMonth.toString(Qt::ISODate));          // Начало месяца
    query.addBindValue(currentDate.addMonths(1).toString(Qt::ISODate)); // Начало следующего месяца

    if (query.exec())
    {
        if (query.next())
        {
            // Если запрос успешен, обновляем totalEarnings
            totalEarnings = query.value(0).toDouble();
            totalEarningsLabel->setText(QString("Total Earnings for this month: %1").arg(totalEarnings));
        }
    }
    else
    {
        QMessageBox::critical(this, "Database Error", "Failed to calculate total earnings: " + query.lastError().text());
    }
}

void EventManager::addEventToDatabase(const QString &title, const QString &description, double price, const QString &place, const QString &time)
{
    QSqlQuery query;
    query.prepare("INSERT INTO events (title, description, price, place, time, eventDate) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(title);
    query.addBindValue(description);
    query.addBindValue(price);
    query.addBindValue(place);
    query.addBindValue(time);
    query.addBindValue(currentDate.toString(Qt::ISODate));

    if (!query.exec())
    {
        QMessageBox::critical(this, "Database Error", "Failed to add event: " + query.lastError().text());
    }
}

void EventManager::removeEventFromDatabase(int eventId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM events WHERE id = ?");
    query.addBindValue(eventId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Database Error", "Failed to remove event: " + query.lastError().text());
    }
}

void EventManager::onDateSelected(const QDate &date)
{
    currentDate = date;
    loadEventsForDate(date);

    // Извлекаем год и месяц для выбранной даты
    int year = date.year();
    int month = date.month();

    // Всегда вызываем метод для обновления суммы заработка, независимо от месяца
    updateTotalEarningsForMonth(year, month);
}

void EventManager::onAddEvent()
{
    AddEventDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted)
    {
        QString title = dialog.getTitle();
        QString description = dialog.getDescription();
        double price = dialog.getPrice();
        QString place = dialog.getPlace();
        QString time = dialog.getTime();

        addEventToDatabase(title, description, price, place, time);
        loadEventsForDate(currentDate);
    }
}

void EventManager::onRemoveEvent()
{
    QModelIndex index = eventListView->currentIndex();
    if (index.isValid())
    {
        int eventId = index.siblingAtColumn(6).data(Qt::UserRole).toInt();
        removeEventFromDatabase(eventId);
        loadEventsForDate(currentDate);
    }
    else
    {
        QMessageBox::warning(this, "Warning", "Please select an event to remove.");
    }
}