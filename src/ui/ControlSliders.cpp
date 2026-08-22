#include "ControlSliders.h"
#include "../v4l2/ControlPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace cmi {

ControlSliders::ControlSliders(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(6);
    m_layout->addStretch();
}

void ControlSliders::clear()
{
    m_panel = nullptr;
    m_deviceKey.clear();
    rebuild();
}

void ControlSliders::bind(ControlPanel *panel, const QString &deviceKey)
{
    m_panel = panel;
    m_deviceKey = deviceKey;
    rebuild();
}

void ControlSliders::rebuild()
{
    // Wipe existing rows.
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        if (QLayout *l = item->layout()) {
            while (QLayoutItem *sub = l->takeAt(0)) {
                if (QWidget *sw = sub->widget())
                    sw->deleteLater();
                delete sub;
            }
            delete l;
        }
        delete item;
    }

    if (!m_panel || !m_panel->isOpen()) {
        auto *lbl = new QLabel(QStringLiteral("no device"), this);
        lbl->setObjectName(QStringLiteral("muted"));
        m_layout->addWidget(lbl);
        m_layout->addStretch();
        return;
    }

    for (const ControlPanel::Control &c : m_panel->controls()) {
        auto *row = new QHBoxLayout;
        auto *label = new QLabel(c.name, this);
        label->setStyleSheet(QStringLiteral("color: #1d1d1f; font-size: 11.5px; font-weight: 500;"));
        label->setMinimumWidth(100);
        row->addWidget(label);

        if (c.isBoolean) {
            auto *chk = new QCheckBox(this);
            chk->setChecked(c.value != 0);
            row->addWidget(chk, 1);
            uint32_t id = c.id;
            connect(chk, &QCheckBox::toggled, this, [this, id](bool on) {
                if (m_panel)
                    m_panel->setValue(id, on ? 1 : 0);
            });
        } else if (c.isMenu) {
            auto *combo = new QComboBox(this);
            for (const auto &item : c.menuItems)
                combo->addItem(item.second, item.first);
            int idx = combo->findData(c.value);
            if (idx >= 0)
                combo->setCurrentIndex(idx);
            row->addWidget(combo, 1);
            uint32_t id = c.id;
            connect(combo, &QComboBox::currentIndexChanged,
                    this, [this, id, combo](int i) {
                if (m_panel && i >= 0)
                    m_panel->setValue(id, combo->itemData(i).toInt());
            });
        } else {
            auto *slider = new QSlider(Qt::Horizontal, this);
            slider->setRange(c.minimum, c.maximum);
            slider->setSingleStep(c.step);
            slider->setValue(c.value);
            auto *valueLabel = new QLabel(QString::number(c.value), this);
            valueLabel->setStyleSheet(QStringLiteral("color: #86868b; font-size: 11px; font-weight: 600;"));
            valueLabel->setMinimumWidth(36);
            valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            row->addWidget(slider, 1);
            row->addWidget(valueLabel);
            uint32_t id = c.id;
            connect(slider, &QSlider::valueChanged,
                    this, [this, id, valueLabel](int v) {
                valueLabel->setText(QString::number(v));
                if (m_panel)
                    m_panel->setValue(id, v);
            });
        }
        m_layout->addLayout(row);
    }

    // Preset controls.
    auto *presetRow = new QHBoxLayout;
    auto *saveBtn = new QPushButton(QStringLiteral("Save Preset"), this);
    auto *loadBtn = new QPushButton(QStringLiteral("Load Preset"), this);
    saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(0, 0, 0, 0.05);"
        "  border: 1px solid rgba(0, 0, 0, 0.08);"
        "  border-radius: 6px;"
        "  color: #1d1d1f;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover { background: rgba(0, 0, 0, 0.09); }"
    ));
    loadBtn->setStyleSheet(saveBtn->styleSheet());
    presetRow->addWidget(saveBtn);
    presetRow->addWidget(loadBtn);
    m_layout->addLayout(presetRow);
    connect(saveBtn, &QPushButton::clicked, this, &ControlSliders::onSavePreset);
    connect(loadBtn, &QPushButton::clicked, this, &ControlSliders::onLoadPreset);

    m_layout->addStretch();
}

void ControlSliders::onSavePreset()
{
    if (!m_panel || m_deviceKey.isEmpty())
        return;
    bool ok = false;
    QString name = QInputDialog::getText(this, QStringLiteral("c~mi"),
                                         QStringLiteral("preset name:"),
                                         QLineEdit::Normal, {}, &ok);
    if (ok && !name.isEmpty())
        m_panel->savePreset(m_deviceKey, name);
}

void ControlSliders::onLoadPreset()
{
    if (!m_panel || m_deviceKey.isEmpty())
        return;
    const QStringList list = ControlPanel::presets(m_deviceKey);
    if (list.isEmpty())
        return;
    bool ok = false;
    QString name = QInputDialog::getItem(this, QStringLiteral("c~mi"),
                                         QStringLiteral("load preset:"),
                                         list, 0, false, &ok);
    if (ok && !name.isEmpty()) {
        m_panel->loadPreset(m_deviceKey, name);
        rebuild(); // reflect new values
    }
}

} // namespace cmi
