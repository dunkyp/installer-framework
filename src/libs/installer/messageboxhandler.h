/**************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
**************************************************************************/

#ifndef QINSTALLER_MESSAGEBOXHANDLER_H
#define QINSTALLER_MESSAGEBOXHANDLER_H

#include <installer_global.h>

#include <QHash>
#include <QObject>

class QWidget;
class QMessageBox : public QObject {
    Q_OBJECT
public:
    enum Icon {
        // keep this in sync with QMessageDialogOptions::Icon
        NoIcon = 0,
        Information = 1,
        Warning = 2,
        Critical = 3,
        Question = 4
    };
    Q_ENUM(Icon)
enum StandardButton {
        // keep this in sync with QDialogButtonBox::StandardButton and QPlatformDialogHelper::StandardButton
        NoButton           = 0x00000000,
        Ok                 = 0x00000400,
        Save               = 0x00000800,
        SaveAll            = 0x00001000,
        Open               = 0x00002000,
        Yes                = 0x00004000,
        YesToAll           = 0x00008000,
        No                 = 0x00010000,
        NoToAll            = 0x00020000,
        Abort              = 0x00040000,
        Retry              = 0x00080000,
        Ignore             = 0x00100000,
        Close              = 0x00200000,
        Cancel             = 0x00400000,
        Discard            = 0x00800000,
        Help               = 0x01000000,
        Apply              = 0x02000000,
        Reset              = 0x04000000,
        RestoreDefaults    = 0x08000000,
        FirstButton        = Ok,                // internal
        LastButton         = RestoreDefaults,   // internal
        YesAll             = YesToAll,          // obsolete
        NoAll              = NoToAll,           // obsolete
        Default            = 0x00000100,        // obsolete
        Escape             = 0x00000200,        // obsolete
        FlagMask           = 0x00000300,        // obsolete
    };
    typedef StandardButton Button;  // obsolete
        Q_DECLARE_FLAGS(StandardButtons, StandardButton)
        Q_FLAG(StandardButtons)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMessageBox::StandardButtons)

namespace QInstaller {

class INSTALLER_EXPORT MessageBoxHandler : public QObject
{
    Q_OBJECT

public:
    enum DefaultAction {
        AskUser,
        Accept,
        Reject,
        Default
    };

    enum MessageType{
        criticalType,
        informationType,
        questionType,
        warningType
    };

    static MessageBoxHandler *instance();
    static QWidget *currentBestSuitParent();

    void setDefaultAction(DefaultAction defaultAction);
    void setAutomaticAnswer(const QString &identifier, QMessageBox::StandardButton answer);

    static QMessageBox::StandardButton critical(QWidget *parent, const QString &identifier,
        const QString &title, const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton button = QMessageBox::NoButton);

    static QMessageBox::StandardButton information(QWidget *parent, const QString &identifier,
        const QString &title, const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton button=QMessageBox::NoButton);

    static QMessageBox::StandardButton question(QWidget *parent, const QString &identifier,
        const QString &title, const QString &text,
        QMessageBox::StandardButtons buttons = static_cast<QMessageBox::StandardButtons>(QMessageBox::Yes | QMessageBox::No),
        QMessageBox::StandardButton button = QMessageBox::NoButton);

    static QMessageBox::StandardButton warning(QWidget *parent, const QString &identifier,
        const QString &title, const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton button = QMessageBox::NoButton);

    Q_INVOKABLE int critical(const QString &identifier, const QString &title, const QString &text,
        int buttons = QMessageBox::Ok, int button = QMessageBox::NoButton);

    Q_INVOKABLE int information(const QString &identifier, const QString &title, const QString &text,
        int buttons = QMessageBox::Ok, int button = QMessageBox::NoButton);

    Q_INVOKABLE int question(const QString &identifier, const QString &title, const QString &text,
        int buttons = QMessageBox::Yes | QMessageBox::No, int button = QMessageBox::NoButton);

    Q_INVOKABLE int warning(const QString &identifier, const QString &title, const QString &text,
        int buttons = QMessageBox::Ok, int button = QMessageBox::NoButton);

    static QList<QMessageBox::Button> orderedButtons();

private Q_SLOTS:
    //this removes the slot from the script area
    virtual void deleteLater() {
        QObject::deleteLater();
    }

private:
    explicit MessageBoxHandler(QObject *parent);

    QMessageBox::StandardButton autoReply(QMessageBox::StandardButtons buttons) const;
    QMessageBox::StandardButton showMessageBox(MessageType messageType, QWidget *parent,
        const QString &identifier, const QString &title, const QString &text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        const QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    bool askAnswerFromUser(QMessageBox::StandardButton &selectedButton,
                           QMessageBox::StandardButtons &availableButtons) const;
    QString availableAnswerOptions(const QFlags<QMessageBox::StandardButton> &flags) const;


private:
    static MessageBoxHandler *m_instance;

    DefaultAction m_defaultAction;
    QList<QMessageBox::Button> m_buttonOrder;
    QHash<QString, QMessageBox::StandardButton> m_automaticAnswers;
};

}
Q_DECLARE_METATYPE(QMessageBox::StandardButton)
Q_DECLARE_METATYPE(QMessageBox::StandardButtons)

#endif // QINSTALLER_MESSAGEBOXHANDLER_H
