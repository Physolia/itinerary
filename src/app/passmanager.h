/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PASSMANAGER_H
#define PASSMANAGER_H

#include <QAbstractListModel>

/** Holds time-less pass or program membership elements.
 *  Not to be confused with PkPassManager, which handles storage
 *  and updates of Apple Wallet pass files.
 */
class PassManager : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PassManager(QObject *parent = nullptr);
    ~PassManager();

    enum {
        PassRole = Qt::UserRole,
        PassIdRole,
        PassTypeRole,
        PassDataRole,
        NameRole,
    };

    enum PassType {
        ProgramMembership,
        PkPass
    };
    Q_ENUM(PassType)

    bool import(const QVariant &pass, const QString &id = {});
    bool import(const QVector<QVariant> &passes);

    /** Returns the pass id for the pass that is the closest match
     *  to the given ProgramMembership object.
     *  This is useful for finding the membership pass based on potentially
     *  incomplete data from a ticket.
     *  An empty string is returned if no matching membership is found.
     */
    Q_INVOKABLE QString findMatchingPass(const QVariant &pass) const;
    /** Returns the pass object for @p passId. */
    Q_INVOKABLE QVariant pass(const QString &passId) const;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool remove(const QString &passId);

    Q_INVOKABLE bool removeRow(int row, const QModelIndex &parent = QModelIndex()); // not exported to QML in Qt5 yet
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = {}) override;

private:
    struct Entry {
        QString id;
        QVariant data;

        bool operator<(const Entry &other) const;
    };
    mutable std::vector<Entry> m_entries;

    void load();
    void ensureLoaded(Entry &entry) const;
    QByteArray rawData(const Entry &entry) const;

    static QString basePath();
};

#endif // PASSMANAGER_H
