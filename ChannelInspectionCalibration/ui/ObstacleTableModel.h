// =============================================================================
// ObstacleTableModel.h — QAbstractTableModel for obstacle data editor
// =============================================================================
#pragma once
#include <QAbstractTableModel>
#include <QVector>
#include <QStyledItemDelegate>
#include <QComboBox>
#include "../models/DeviceState.h"

// Delegate that shows a QComboBox for editing
class ComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ComboBoxDelegate(const QStringList& items, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_items(items) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override {
        auto* cb = new QComboBox(parent);
        cb->addItems(m_items);
        return cb;
    }
    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (cb) cb->setCurrentText(index.data(Qt::EditRole).toString());
    }
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (cb) model->setData(index, cb->currentText(), Qt::EditRole);
    }
private:
    QStringList m_items;
};

class ObstacleTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColId = 0, ColType, ColStart, ColEnd, ColSeverity, ColNotes, ColLength, ColCount };

    explicit ObstacleTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    void setObstacles(const QVector<Obstacle>& obs);
    QVector<Obstacle> obstacles() const { return m_obstacles; }
    void addRow();
    void removeRow(int row);
    const Obstacle& obstacleAt(int row) const;
    ComboBoxDelegate* typeDelegate()     const { return m_typeDelegate; }
    ComboBoxDelegate* severityDelegate() const { return m_severityDelegate; }

signals:
    void dataModified();

private:
    QVector<Obstacle> m_obstacles;
    QStringList m_types;
    QStringList m_severities;
    ComboBoxDelegate* m_typeDelegate     = nullptr;
    ComboBoxDelegate* m_severityDelegate = nullptr;
};
