// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; see the LICENSE file. If not, see <https://www.gnu.org/licenses/>.
//
// okm-cli: a headless harness for the protocol core, built with desktop Qt. It proves login,
// a signed call, the conversation list, a history page and the friend list - every read path
// the phone UI uses - against the live server, from a PC. Read-only: it never sends.
//
//   okm-cli <creds.txt>        login on line 1, password on line 2; saves okm.token
//   okm-cli --token okm.token  log in with the saved token instead
//   OKM_PROXY=http://127.0.0.1:8080/  routes https:// through a local forwarder (see
//                              tools/tls-forwarder.py) when the Qt build has no usable TLS.
#include "oknetwork.h"
#include "okapi.h"
#include "oksession.h"
#include "okjson.h"

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <cstdio>

static QTextStream out(stdout);

class Harness : public QObject
{
    Q_OBJECT
public:
    Harness(OkSession *s, const QString &tokenFile) : m_s(s), m_tokenFile(tokenFile), m_step(0)
    {
        connect(s, SIGNAL(loginFinished(bool,QString,QString)), this, SLOT(onLogin(bool,QString,QString)));
        connect(s, SIGNAL(conversationsRefreshed(bool,QString)), this, SLOT(onConversations(bool,QString)));
        connect(s, SIGNAL(historyLoaded(QString,QString,OkHistoryPage)), this, SLOT(onHistory(QString,QString,OkHistoryPage)));
        connect(s, SIGNAL(historyFailed(QString,QString,QString)), this, SLOT(onHistoryFailed(QString,QString,QString)));
        connect(s, SIGNAL(friendsLoaded(bool,QList<OkUser>,QString)), this, SLOT(onFriends(bool,QList<OkUser>,QString)));
        connect(s, SIGNAL(credentialsChanged()), this, SLOT(onCredentials()));
    }

private slots:
    void onCredentials()
    {
        QFile f(m_tokenFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(m_s->credentials().token.toUtf8());
    }

    void onLogin(bool ok, const QString &error, const QString &verificationUrl)
    {
        if (!ok) {
            if (!verificationUrl.isEmpty()) out << "VERIFICATION REQUIRED: " << verificationUrl << endl;
            else out << "LOGIN FAILED: " << error << endl;
            out.flush();
            qApp->exit(2);
            return;
        }
        out << "login ok: uid=" << m_s->credentials().uid << " api_server=" << m_s->credentials().apiServer
            << " token=" << m_s->credentials().token.left(6) << "..." << endl;
        if (!qgetenv("OKM_UPLOAD_URL").isEmpty()) {
            // Diagnostics: the shape of the photo upload url (no upload happens).
            OkSessionCall *c = m_s->sessionCall(QLatin1String("api/photosV2/getUploadUrl"), OkParams());
            connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onUploadUrl(OkSessionCall*)));
            return;
        }
        m_s->refreshConversations(20);
    }

    void onUploadUrl(OkSessionCall *c)
    {
        if (!c->ok()) { out << "getUploadUrl FAILED: " << c->errorMessage() << endl; out.flush(); qApp->exit(4); return; }
        const QString u = OkJson::str(c->result(), "upload_url");
        QString masked = u;
        out << "upload_url: " << masked.replace(QRegExp("=([^&]{4})[^&]*"), "=\1***") << " (contains %: " << u.contains(QLatin1Char('%')) << ")" << endl;

        // OKM_UPLOAD_TEST=<image file>: upload it (the reply is a photo token that expires
        // unused - no message is sent) to prove the multipart step end to end.
        const QByteArray file = qgetenv("OKM_UPLOAD_TEST");
        if (file.isEmpty()) { out.flush(); qApp->exit(0); return; }
        QFile f(QString::fromLocal8Bit(file));
        if (!f.open(QIODevice::ReadOnly)) { out << "cannot read " << file << endl; out.flush(); qApp->exit(1); return; }
        OkReply *r = m_s->api()->uploadFile(u, f.readAll(), QLatin1String("image/jpeg"), QLatin1String("pic1"), QLatin1String("photo.jpg"), true);
        connect(r, SIGNAL(finished(OkReply*)), this, SLOT(onUploaded(OkReply*)));
    }

    void onUploaded(OkReply *r)
    {
        if (!r->ok()) out << "UPLOAD FAILED: http " << r->httpStatus() << " " << r->errorMessage() << endl;
        else {
            const QVariantMap photos = OkJson::object(r->result(), "photos");
            QString token;
            for (QVariantMap::const_iterator it = photos.constBegin(); it != photos.constEnd() && token.isEmpty(); ++it)
                token = OkJson::str(it.value().toMap(), "token");
            out << "upload ok: photos=" << photos.size() << " token=" << (token.isEmpty() ? "NONE" : token.left(8) + "...") << endl;
        }
        out.flush();
        qApp->exit(r->ok() ? 0 : 5);
    }

    void onConversations(bool ok, const QString &error)
    {
        if (!ok) { out << "getList FAILED: " << error << endl; out.flush(); qApp->exit(3); return; }
        const QList<OkDialog> &d = m_s->dialogs();
        out << "conversations: " << d.size() << endl;
        for (int i = 0; i < d.size() && i < 8; ++i) {
            const OkDialog &c = d.at(i);
            out << "  [" << (c.isChat ? "chat" : "1:1 ") << "] " << c.title
                << "  unread=" << c.unreadCount << "  " << c.lastTime.toLocalTime().toString(Qt::ISODate)
                << "  avatar=" << (c.user.photo.isEmpty() ? "-" : "yes")
                << "  \"" << c.preview.left(40).replace('\n', ' ') << "\"" << endl;
        }
        out << "me: " << m_s->me().displayName() << endl;
        // Skip the chat-with-yourself entry (PRIVATE:<own uid>, no peer): OK's API cannot read
        // it (OK-Messenger-API.md, section 12).
        for (int i = 0; i < d.size() && m_conv.isEmpty(); ++i)
            if (d.at(i).isChat || !d.at(i).peerUid.isEmpty()) m_conv = d.at(i).conversationId;
        if (m_conv.isEmpty()) { m_s->loadFriends(); return; }
        m_s->loadHistory(m_conv, QString(), 15);
    }

    void onHistory(const QString &conv, const QString &anchor, const OkHistoryPage &page)
    {
        out << "history " << (anchor.isEmpty() ? "latest" : "earlier") << ": " << page.messages.size()
            << " messages, has_more=" << page.hasMore << " anchor=" << page.anchor.left(20) << endl;
        for (int i = 0; i < page.messages.size(); ++i) {
            const OkMessage &m = page.messages.at(i);
            out << "  " << m.date.toLocalTime().toString("dd.MM HH:mm") << " " << (m.out ? "me" : m.senderName)
                << (m.isSystem ? " [system]" : "") << (m.replyToId.isEmpty() ? "" : " [reply]") << ": "
                << m.body.left(60).replace('\n', ' ');
            for (int a = 0; a < m.attachments.size(); ++a) {
                const OkAttachment &at = m.attachments.at(a);
                out << " {" << at.type << (at.isImage() ? " img=" + at.previewUrl.left(50) : QString())
                    << (at.isVideo() || at.isAudio() ? " url=" + at.url.left(50) : QString())
                    << (at.duration ? " " + at.durationText() : QString()) << "}";
            }
            out << endl;
        }
        if (anchor.isEmpty() && page.hasMore && !page.anchor.isEmpty() && m_step++ == 0) {
            m_s->loadHistory(conv, page.anchor, 5);
            return;
        }
        m_s->loadFriends();
    }

    void onHistoryFailed(const QString &, const QString &, const QString &error)
    {
        out << "history FAILED: " << error << endl;
        m_s->loadFriends();
    }

    void onFriends(bool ok, const QList<OkUser> &friends, const QString &error)
    {
        if (!ok) out << "friends FAILED: " << error << endl;
        else {
            out << "friends: " << friends.size() << endl;
            for (int i = 0; i < friends.size() && i < 5; ++i)
                out << "  " << friends.at(i).displayName() << (friends.at(i).online ? " (online)" : "") << endl;
        }
        out << "DONE" << endl;
        out.flush();
        qApp->exit(0);
    }

private:
    OkSession *m_s;
    QString m_tokenFile;
    QString m_conv;
    int m_step;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    out.setCodec("UTF-8");
    const QStringList args = app.arguments();
    if (args.size() < 2) {
        out << "usage: okm-cli <creds.txt> | --token <tokenfile>" << endl;
        return 1;
    }

    const QByteArray proxy = qgetenv("OKM_PROXY");
    if (!proxy.isEmpty()) OkNetwork::setUrlRewritePrefix(QString::fromLatin1(proxy));
    out << "ssl supported by this Qt build: " << OkNetwork::sslSupported() << endl;

    OkNetwork net;
    OkApi api(&net);
    OkSession session(&api);
    Harness h(&session, QLatin1String("okm.token"));

    if (args.at(1) == QLatin1String("--token")) {
        QFile f(args.value(2, QLatin1String("okm.token")));
        if (!f.open(QIODevice::ReadOnly)) { out << "cannot read token file" << endl; return 1; }
        session.loginByToken(QString::fromUtf8(f.readAll()).trimmed());
    } else {
        QFile f(args.at(1));
        if (!f.open(QIODevice::ReadOnly)) { out << "cannot read " << args.at(1) << endl; return 1; }
        const QStringList lines = QString::fromUtf8(f.readAll()).split(QRegExp("\r?\n"));
        if (lines.size() < 2) { out << "creds file needs login on line 1 and password on line 2" << endl; return 1; }
        session.loginByPassword(lines.at(0).trimmed(), lines.at(1).trimmed());
    }

    QTimer::singleShot(120 * 1000, &app, SLOT(quit()));
    return app.exec();
}

#include "main.moc"
