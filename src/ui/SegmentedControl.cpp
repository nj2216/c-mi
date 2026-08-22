#include "SegmentedControl.h"

namespace cmi {

SegmentedControl::SegmentedControl(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("segmentedControl"));
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(2);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    connect(m_group, &QButtonGroup::idClicked, this, [this](int id) {
        emit currentIndexChanged(id);
        if (id >= 0 && id < static_cast<int>(m_segmentData.size())) {
            emit currentDataChanged(m_segmentData[id]);
        }
    });

    setStyleSheet(QStringLiteral(
        "QWidget#segmentedControl {"
        "  background: rgba(0, 0, 0, 0.06);"
        "  border-radius: 9px;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #86868b;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  padding: 6px 4px;"
        "  border-radius: 7px;"
        "  min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "  color: #1d1d1f;"
        "}"
        "QPushButton:checked {"
        "  background: #ffffff;"
        "  color: #1d1d1f;"
        "  border: 1px solid rgba(0, 0, 0, 0.04);"
        "}"
    ));
}

void SegmentedControl::addSegment(const QString &text, const QVariant &data)
{
    int index = static_cast<int>(m_segmentData.size());
    m_segmentData.push_back(data);

    auto *btn = new QPushButton(text, this);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    m_group->addButton(btn, index);
    m_layout->addWidget(btn);

    if (index == 0) {
        btn->setChecked(true);
    }
}

void SegmentedControl::setCurrentIndex(int index)
{
    if (QAbstractButton *btn = m_group->button(index)) {
        btn->setChecked(true);
    }
}

int SegmentedControl::currentIndex() const
{
    return m_group->checkedId();
}

QVariant SegmentedControl::currentData() const
{
    int id = currentIndex();
    if (id >= 0 && id < static_cast<int>(m_segmentData.size())) {
        return m_segmentData[id];
    }
    return QVariant();
}

QString SegmentedControl::currentText() const
{
    if (QAbstractButton *btn = m_group->checkedButton()) {
        return btn->text();
    }
    return QString();
}

void SegmentedControl::clear()
{
    m_segmentData.clear();
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            m_group->removeButton(qobject_cast<QAbstractButton *>(w));
            w->deleteLater();
        }
        delete item;
    }
}

} // namespace cmi
