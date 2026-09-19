# SimpleOKM for Symbian

An Odnoklassniki (OK) messaging client for **Symbian Anna and Belle** (Nokia N8, E7, C7, X7,
700, 701, 808 PureView, ...), written in Qt 4.7.4 / Qt Quick 1.1 with the Symbian Qt Quick
Components.

Derived from [SimpleOKM](https://github.com/infodimus/SimpleOKM) (Android) and
[OKLumessenger](https://github.com/infodimus/OKLumessenger) (Windows Phone 8.1), GPL-2.0-or-later.
The protocol core is SimpleOKM's, ported from C# to Qt C++; the host, transport and UI are new.
The protocol itself is documented in [OK-Messenger-API.md](OK-Messenger-API.md).

---

## Why it needs the Qt TLS patch

A 2011 phone cannot talk to ok.ru out of the box: the stock Qt 4.7 networking on Symbian
speaks TLS 1.0 only, and the phone's certificate store has no path to the CAs OK's hosts chain
to today (`HARICA TLS RSA Root CA 2021`). The app solves the two halves separately:

- **TLS 1.2** comes from the **Qt TLS patch** (https://nnproject.cc/qtls, sources at
  [shinovon/qt-patches](https://github.com/shinovon/qt-patches)) - a `QtNetwork.dll` with
  OpenSSL 1.0.2u linked in, installed on the phone once. Without it the sign-in page says so
  (`QSslSocket::supportsSsl()` is false) and nothing will connect.
- **Trust** is the app's own: `certs/ok-roots.pem` (the HARICA roots plus the Russian Trusted
  Root/Sub CA as a fallback) is compiled in and stamped on every request - API calls, uploads
  and the QML `Image` loads of avatars and photos - through one `QNetworkAccessManager`
  subclass (`OkNetwork`). The system store is not consulted.

Everything goes through `QNetworkAccessManager`, so the app itself carries no crypto.

## What it does

- **Sign in** by password or by the reusable token OK hands out (no OAuth). The token is kept
  in the app's private settings and reused on later starts; a dead session is renewed
  silently with the token, once, and the call repeated. OK's one-time verification
  (captcha / device confirm) opens in the browser.
- **Conversations** list with avatars, unread badges, last-message previews, newest first;
  refreshed every 20 s while it is the visible page. Long-press: open profile, delete chat.
- **Chat**: history with "load earlier", a 10 s poll that merges new and edited messages in
  place, jump to the first unread on open, day separators. Send text; **reply** (long-press ->
  Reply, quote bar over the composer, quoted line in the bubble); copy text; delete own
  messages; a failed send can be retried.
- **Photos**: received photos inline (tap for full screen, save to the phone's Images folder,
  open in the browser); send a photo from the phone (picked with a file dialog, scaled to
  1280 px and re-encoded as JPEG before upload). A **video** or **voice message** shows as a
  tile that opens in the browser / media player; links show their title.
- **New chat** from the friend list.
- **Settings**: load photos and avatars on/off (data saver - photos are then loaded only when
  tapped), app language (system / English / Russian / Ukrainian), sign out.
- Chat with yourself (`PRIVATE:<own uid>`) is shown as "Notes to myself" with an explanation:
  OK's messaging API cannot read it (see the API doc, section 12).

### Not in this port

- Background notifications. A self-signed Symbian app has no background service; the app
  polls only while open.
- Sending video / voice. OK's video upload blanks the whole conversation while it transcodes
  (see the API doc); voice recording is left for later.
- End-to-end encryption (the Android app's optional Komet-compatible scheme).

## Layout

```
SimpleOKM.pro                the phone app (Symbian and Qt Simulator kits)
core.pri                     the protocol core, shared with the desktop harness
src/core/
  okjson.*                   JSON reader/writer on QVariant (Qt 4.7 has none)
  okconstants.h              app key / secret (from credentials.cfg), method paths, field lists
  oksigning.*                the MD5 signature recipe
  oknetwork.*                the one QNetworkAccessManager: trust bundle, User-Agent, bearer
  okapi.*                    signed calls and multipart uploads; OkReply = one call in flight
  oksession.*  oksession_p.h login, conversations, history, send, upload, friends, delete
  okparser.*  okmodels.h     JSON -> OkDialog / OkMessage / OkAttachment / OkUser
src/app/
  appcontroller.*            "app" in QML: network session, token, timers, host services
  conversationsmodel.*       the list, newest first
  messagesmodel.*            the open chat: pages, poll/merge, pending bubbles, reply, delete
  friendsmodel.*             the New-chat list
src/main.cpp                 QDeclarativeView + Qt Quick Components, translations
qml/                         main, Login, Conversations, Chat (+MessageDelegate), NewChat,
                             Image, Settings, About, Avatar
certs/ok-roots.pem           the trust bundle (compiled in via certs.qrc)
translations/                ru, uk (.ts sources and compiled .qm); tools/translate.py fills them
tools/okm-cli/               desktop harness for the core (read-only, against the live server)
tools/tls-forwarder.py       local HTTP->HTTPS forwarder for desktop Qt builds without TLS
build-symbian.cmd            command-line phone build (in-source; see below)
```

## Building

### Credentials

Copy `credentials.cfg.template` to `credentials.cfg` and fill in the OK application key and
secret (the values are the official Windows Phone client's; see the API doc). `core.pri`
reads the file at qmake time; the secret never lives in a committed source file. The
User-Agent is a constant in `okconstants.h`, matching the client the key is registered to.

### Phone (Symbian Anna / Belle)

Tooling: Qt SDK 1.2.1 with the **Qt 4.7.4 for Symbian Anna/Belle** target
(`Symbian\SDKs\SymbianSR1Qt474`), GCCE 4.4.1 and SBSv2 - all part of the SDK. The project
must live on the same drive as the SDK (Symbian's `EPOCROOT` is drive-relative), and Qt for
Symbian supports **in-source builds only** (a shadow build breaks the `$$PWD` paths of
included `.pri` files); `build-symbian.cmd` runs in the project directory and the generated
files (`bld.inf`, `*.mmp`, `*.pkg`, `*.rss`, `*.loc`, `*.sis`) are in `.gitignore`.

```
build-symbian.cmd              -> SimpleOKM.sis            (self-signed, Belle)
build-symbian.cmd installer    -> SimpleOKM_installer.sis  (Smart Installer wrapper, for Anna)
build-symbian.cmd clean
```

In Qt Creator: open `SimpleOKM.pro`, choose the Symbian Device kit with Qt 4.7.4, build
Release; "Deploy" creates the same `.sis`.

The package is **self-signed** (UID `0xE31A0C4B`, unprotected range) and asks for
`NetworkServices ReadUserData WriteUserData` - all user-grantable, so it installs on a phone
that accepts self-signed packages (a "hacked" phone or one with the installer patch), which
the Qt TLS patch already requires.

Belle has Qt 4.7.4 and Qt Quick Components built in - install `SimpleOKM.sis` directly.
Anna needs the components, which the Smart Installer package fetches; if the Nokia
repository is gone, install the `Qt Quick components for Symbian` sis from the SDK
(`Symbian\sis\Symbian_Anna`) first and then `SimpleOKM.sis`.

### Qt Simulator (run it on the PC)

Build `SimpleOKM.pro` with the Simulator kit (`D:\QtSDK\Simulator\Qt\mingw\bin\qmake`,
MinGW), start `Simulator\Application\simulator.exe`, then run the exe. For a live session put
OpenSSL 1.0.x `libeay32.dll`/`ssleay32.dll` next to it (the SDK's own are 0.9.8 and cannot
reach ok.ru), or use `tools/tls-forwarder.py` with `OKM_PROXY`. `OKM_CREDS_FILE=<file>`
(login on line 1, password on line 2) signs in automatically; this hook is compiled out of
the phone build.

### Desktop harness

```
cd tools/okm-cli && qmake && mingw32-make release
okm-cli creds.txt          # login, getList, a history page, friends; saves okm.token
okm-cli --token okm.token  # the token path
OKM_TRACE=1 okm-cli ...    # log every request line (password masked) and reply status
```

It only reads; it never sends.

## Notes for the phone

- **Access point**: the app opens a `QNetworkSession` on the default configuration at start
  and hands it to every access manager, so the phone asks for a connection once, not per
  request. Set a default destination / access point in the phone's connectivity settings to
  avoid the prompt entirely.
- **Clock**: certificate validation needs a roughly correct date on the phone. A wrong year
  shows up as "TLS verification failed (... not yet valid / expired)" on the sign-in page.
- **Saved photos** go to `E:\Images\SimpleOKM\` (memory card) or `C:\Data\Images\SimpleOKM\`.
- `Qt: Untested Windows version` on the PC and the SDK headers' warnings are harmless.
