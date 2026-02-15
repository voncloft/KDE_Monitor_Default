#include <QApplication>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

namespace
{
QString friendlyNameForOutput(const QString& outputName)
{
    const QString n = outputName.toUpper();
    if (n == QStringLiteral("DP-5"))
    {
        return QStringLiteral("Left monitor");
    }
    if (n == QStringLiteral("DP-1"))
    {
        return QStringLiteral("Right monitor");
    }
    if (n == QStringLiteral("HDMI-0"))
    {
        return QStringLiteral("Projector");
    }
    return outputName;
}

class MonitorWindow : public QWidget
{
public:
    MonitorWindow()
    {
        setWindowTitle(QStringLiteral("Monitor Primary Selector"));
        resize(540, 280);

        auto* layout = new QVBoxLayout(this);
        m_hintLabel = new QLabel(QStringLiteral("Select a row, then click the button to set it as primary."), this);
        m_statusLabel = new QLabel(this);

        m_table = new QTableWidget(this);
        m_table->setColumnCount(2);
        m_table->setHorizontalHeaderLabels({
            QStringLiteral("Primary"),
            QStringLiteral("Name")
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_table->verticalHeader()->setVisible(false);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);

        m_setPrimaryButton = new QPushButton(QStringLiteral("Set Selected As Primary"), this);

        layout->addWidget(m_hintLabel);
        layout->addWidget(m_table);
        layout->addWidget(m_setPrimaryButton);
        layout->addWidget(m_statusLabel);

        auto refresh = [this]() { refreshUi(); };
        connect(qApp, &QGuiApplication::primaryScreenChanged, this, [refresh](QScreen*) { refresh(); });
        connect(qApp, &QGuiApplication::screenAdded, this, [refresh](QScreen*) { refresh(); });
        connect(qApp, &QGuiApplication::screenRemoved, this, [refresh](QScreen*) { refresh(); });
        connect(m_setPrimaryButton, &QPushButton::clicked, this, [this]() { setSelectedPrimary(); });
        connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { setSelectedPrimary(); });

        refreshUi();
        QTimer::singleShot(0, this, [this]() { fitWindowToContentOnce(); });
    }

private:
    void refreshUi()
    {
        const QList<QScreen*> screens = QGuiApplication::screens();
        QScreen* primary = QGuiApplication::primaryScreen();
        m_table->setRowCount(screens.size());

        for (int i = 0; i < screens.size(); ++i)
        {
            QScreen* s = screens.at(i);
            const bool isPrimary = (s == primary);
            m_table->setItem(i, 0, new QTableWidgetItem(isPrimary ? QStringLiteral("Y") : QStringLiteral("N")));
            auto* nameItem = new QTableWidgetItem(friendlyNameForOutput(s->name()));
            nameItem->setData(Qt::UserRole, s->name());
            m_table->setItem(i, 1, nameItem);
        }

        if (m_table->rowCount() > 0 && m_table->currentRow() < 0)
        {
            m_table->selectRow(0);
        }

        m_table->resizeRowsToContents();
    }

    void fitWindowToContentOnce()
    {
        if (m_initialFitDone)
        {
            return;
        }

        int rowsHeight = 0;
        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            rowsHeight += m_table->rowHeight(row);
        }

        const int tableHeight = m_table->horizontalHeader()->height()
            + rowsHeight
            + (m_table->frameWidth() * 2);
        const int nonTableHeight = height() - m_table->height();

        int targetWidth = std::max(540, sizeHint().width());
        int targetHeight = tableHeight + nonTableHeight + 20;

        if (QScreen* screen = QGuiApplication::primaryScreen())
        {
            const QRect available = screen->availableGeometry();
            targetWidth = std::min(targetWidth, (available.width() * 9) / 10);
            targetHeight = std::min(targetHeight, (available.height() * 9) / 10);
        }

        resize(targetWidth, std::max(targetHeight, 260));
        m_initialFitDone = true;
    }

    void setSelectedPrimary()
    {
        const int row = m_table->currentRow();
        if (row < 0)
        {
            m_statusLabel->setText(QStringLiteral("Select a monitor first."));
            return;
        }

        QTableWidgetItem* nameItem = m_table->item(row, 1);
        if (!nameItem)
        {
            m_statusLabel->setText(QStringLiteral("Invalid selection."));
            return;
        }

        const QString outputName = nameItem->data(Qt::UserRole).toString();
        if (outputName.isEmpty())
        {
            m_statusLabel->setText(QStringLiteral("Missing output name for selection."));
            return;
        }

        if (m_table->item(row, 0) && m_table->item(row, 0)->text() == QStringLiteral("Y"))
        {
            m_statusLabel->setText(QStringLiteral("%1 is already primary.").arg(nameItem->text()));
            return;
        }

        const QString tool = QStandardPaths::findExecutable(QStringLiteral("kscreen-doctor"));
        if (tool.isEmpty())
        {
            m_statusLabel->setText(QStringLiteral("kscreen-doctor not found. Install kdeplasma-addons/kscreen."));
            return;
        }

        const QStringList args = {
            QStringLiteral("output.%1.primary").arg(outputName)
        };

        QProcess process;
        process.start(tool, args);
        if (!process.waitForFinished(4000))
        {
            process.kill();
            m_statusLabel->setText(QStringLiteral("Timed out setting primary monitor."));
            return;
        }

        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        {
            const QString err = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
            m_statusLabel->setText(err.isEmpty()
                ? QStringLiteral("Failed to set primary monitor.")
                : QStringLiteral("Failed: %1").arg(err));
            return;
        }

        m_statusLabel->setText(QStringLiteral("Primary monitor set to %1 (%2).")
            .arg(nameItem->text())
            .arg(outputName));
        QTimer::singleShot(200, this, [this]() { refreshUi(); });
    }

    QLabel* m_hintLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_setPrimaryButton = nullptr;
    bool m_initialFitDone = false;
};
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MonitorWindow window;
    window.show();

    return app.exec();
}
