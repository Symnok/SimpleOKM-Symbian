# Fills translations/simpleokm_<lang>.ts from the tables below; re-run after lupdate.
# Strings not in a table stay untranslated (English) and are listed. Languages: ru, uk -
# the same set the Windows Phone app ships.
#
#   lupdate -no-obsolete src qml -ts translations/simpleokm_ru.ts translations/simpleokm_uk.ts
#   python tools/translate.py
#   lrelease SimpleOKM.pro
import xml.etree.ElementTree as ET

RU = {
 "About": "О программе", "version %1": "версия %1",
 "An Odnoklassniki messaging client for Symbian Anna and Belle.": "Клиент сообщений Одноклассников для Symbian Anna и Belle.",
 "Signed in as %1": "Вы вошли как %1",
 "TLS: this Qt build has OpenSSL support. Server roots (HARICA) are bundled with the app.": "TLS: эта сборка Qt поддерживает OpenSSL. Корневые сертификаты сервера (HARICA) встроены в приложение.",
 "TLS: NOT available in this Qt build. Install the Qt TLS 1.2 patch from nnproject.cc/qtls.": "TLS: НЕ доступен в этой сборке Qt. Установите патч Qt TLS 1.2 с nnproject.cc/qtls.",
 "Derived from SimpleOKM (Android) and OKLumessenger (Windows Phone). GPL-2.0-or-later.": "Основано на SimpleOKM (Android) и OKLumessenger (Windows Phone). GPL-2.0-or-later.",
 "Source on GitHub": "Исходный код на GitHub",
 "The language changes the next time the app starts.": "Язык изменится при следующем запуске приложения.",
 "Enter your login and password.": "Введите логин и пароль.",
 "OK wants a one-time verification before it lets this device in. Finish it in the browser, then sign in again.": "ОК требует одноразовую проверку, прежде чем впустить это устройство. Завершите проверку в браузере и войдите снова.",
 "The saved session could not be restored: %1": "Не удалось восстановить сохранённую сессию: %1",
 "OK refused the sign-in: %1": "ОК отклонил вход: %1",
 "The session has expired: %1": "Сессия истекла: %1",
 "Could not delete: %1": "Не удалось удалить: %1",
 "Not sent: %1": "Не отправлено: %1",
 "Copied": "Скопировано", "Choose a photo": "Выберите фото", "Images (*.jpg *.jpeg *.png)": "Изображения (*.jpg *.jpeg *.png)",
 "Saving...": "Сохранение...", "Could not save: %1": "Не удалось сохранить: %1", "Saved to %1": "Сохранено в %1",
 "Load earlier messages": "Загрузить более ранние", "Open profile": "Открыть профиль", "Reply": "Ответить",
 "Copy text": "Копировать текст", "Save image": "Сохранить изображение", "Open in browser": "Открыть в браузере",
 "Retry": "Повторить", "Delete": "Удалить", "Notes to myself": "Заметки для себя", "Loading...": "Загрузка...",
 "Load earlier": "Загрузить ранее",
 "OK's messaging API cannot read the chat with yourself. Open it in the browser instead.": "API сообщений ОК не может прочитать чат с самим собой. Откройте его в браузере.",
 "No messages yet.": "Сообщений пока нет.", "Reply: ": "Ответ: ", "message": "сообщение", "Send": "Отпр.",
 "Settings": "Настройки", "Sign out": "Выйти",
 "Sign out? The saved token will be removed from this phone.": "Выйти? Сохранённый токен будет удалён с этого телефона.",
 "Cancel": "Отмена", "Delete chat": "Удалить чат", "Delete \"%1\"?": "Удалить «%1»?", "Chats": "Чаты",
 "new chat": "новый чат", "Not updating: %1": "Не обновляется: %1",
 "No conversations yet. Tap + to start one.": "Чатов пока нет. Нажмите +, чтобы начать.",
 "Save": "Сохранить", "Browser": "Браузер", "Could not load the photo": "Не удалось загрузить фото",
 "sign in to Odnoklassniki": "Вход в Одноклассники", "login (phone or e-mail)": "логин (телефон или почта)",
 "login": "логин", "password": "пароль", "signing in...": "выполняется вход...", "sign in": "Войти",
 "open verification in the browser": "открыть проверку в браузере",
 "The password is sent once; afterwards a token kept on this phone is used.": "Пароль отправляется один раз; затем используется токен, сохранённый на этом телефоне.",
 "This Qt build has no TLS support - install the Qt TLS patch (nnproject.cc/qtls) first.": "Эта сборка Qt не поддерживает TLS - сначала установите патч Qt TLS (nnproject.cc/qtls).",
 "loading...": "загрузка...", "photo unavailable": "фото недоступно", "tap to load photo": "нажмите, чтобы загрузить фото",
 "Video": "Видео", "Voice message": "Голосовое сообщение", "Link: %1": "Ссылка: %1", "edited": "изменено",
 "Today": "Сегодня", "Yesterday": "Вчера", "Chat": "Чат", "Could not read the image": "Не удалось прочитать изображение",
 "Photo": "Фото", "(attachment)": "(вложение)", "New chat": "Новый чат", "online": "в сети", "No friends found.": "Друзья не найдены.",
 "timed out": "время ожидания истекло", "login returned no session": "вход не вернул сессию",
 "session renewal needs verification": "для продления сессии нужна проверка", "not logged in": "вход не выполнен",
 "no image data": "нет данных изображения", "getUploadUrl returned no upload_url": "getUploadUrl не вернул upload_url",
 "upload returned no photo token": "загрузка не вернула токен фото",
 "Exit": "Выход", "App language": "Язык приложения", "System default": "Как в системе", "Load photos and avatars": "Загружать фото и аватары",
 "Off, photos in chats are loaded only when tapped - useful on a slow or metered connection.": "Если выключено, фото в чатах загружаются только по нажатию - полезно при медленном или платном соединении.",
}

UK = {
 "About": "Про програму", "version %1": "версія %1",
 "An Odnoklassniki messaging client for Symbian Anna and Belle.": "Клієнт повідомлень Однокласників для Symbian Anna та Belle.",
 "Signed in as %1": "Ви увійшли як %1",
 "TLS: this Qt build has OpenSSL support. Server roots (HARICA) are bundled with the app.": "TLS: ця збірка Qt підтримує OpenSSL. Кореневі сертифікати сервера (HARICA) вбудовано в застосунок.",
 "TLS: NOT available in this Qt build. Install the Qt TLS 1.2 patch from nnproject.cc/qtls.": "TLS: НЕ доступний у цій збірці Qt. Встановіть патч Qt TLS 1.2 з nnproject.cc/qtls.",
 "Derived from SimpleOKM (Android) and OKLumessenger (Windows Phone). GPL-2.0-or-later.": "Засновано на SimpleOKM (Android) та OKLumessenger (Windows Phone). GPL-2.0-or-later.",
 "Source on GitHub": "Код на GitHub",
 "The language changes the next time the app starts.": "Мова зміниться під час наступного запуску.",
 "Enter your login and password.": "Введіть логін і пароль.",
 "OK wants a one-time verification before it lets this device in. Finish it in the browser, then sign in again.": "ОК вимагає одноразову перевірку, перш ніж впустити цей пристрій. Завершіть перевірку у браузері та увійдіть знову.",
 "The saved session could not be restored: %1": "Не вдалося відновити збережену сесію: %1",
 "OK refused the sign-in: %1": "ОК відхилив вхід: %1",
 "The session has expired: %1": "Сесія завершилась: %1",
 "Could not delete: %1": "Не вдалося видалити: %1",
 "Not sent: %1": "Не надіслано: %1",
 "Copied": "Скопійовано", "Choose a photo": "Виберіть фото", "Images (*.jpg *.jpeg *.png)": "Зображення (*.jpg *.jpeg *.png)",
 "Saving...": "Збереження...", "Could not save: %1": "Не вдалося зберегти: %1", "Saved to %1": "Збережено в %1",
 "Load earlier messages": "Завантажити давніші", "Open profile": "Відкрити профіль", "Reply": "Відповісти",
 "Copy text": "Копіювати текст", "Save image": "Зберегти зображення", "Open in browser": "Відкрити у браузері",
 "Retry": "Повторити", "Delete": "Видалити", "Notes to myself": "Нотатки для себе", "Loading...": "Завантаження...",
 "Load earlier": "Завантажити давніші",
 "OK's messaging API cannot read the chat with yourself. Open it in the browser instead.": "API повідомлень ОК не може прочитати чат із самим собою. Відкрийте його у браузері.",
 "No messages yet.": "Повідомлень ще немає.", "Reply: ": "Відповідь: ", "message": "повідомлення", "Send": "Надісл.",
 "Settings": "Налаштування", "Sign out": "Вийти",
 "Sign out? The saved token will be removed from this phone.": "Вийти? Збережений токен буде видалено з цього телефону.",
 "Cancel": "Скасувати", "Delete chat": "Видалити чат", "Delete \"%1\"?": "Видалити «%1»?", "Chats": "Чати",
 "new chat": "новий чат", "Not updating: %1": "Не оновлюється: %1",
 "No conversations yet. Tap + to start one.": "Чатів ще немає. Натисніть +, щоб почати.",
 "Save": "Зберегти", "Browser": "Браузер", "Could not load the photo": "Не вдалося завантажити фото",
 "sign in to Odnoklassniki": "Вхід в Однокласники", "login (phone or e-mail)": "логін (телефон або пошта)",
 "login": "логін", "password": "пароль", "signing in...": "виконується вхід...", "sign in": "Увійти",
 "open verification in the browser": "відкрити перевірку у браузері",
 "The password is sent once; afterwards a token kept on this phone is used.": "Пароль надсилається один раз; далі використовується токен, збережений на цьому телефоні.",
 "This Qt build has no TLS support - install the Qt TLS patch (nnproject.cc/qtls) first.": "Ця збірка Qt не підтримує TLS - спочатку встановіть патч Qt TLS (nnproject.cc/qtls).",
 "loading...": "завантаження...", "photo unavailable": "фото недоступне", "tap to load photo": "натисніть, щоб завантажити фото",
 "Video": "Відео", "Voice message": "Голосове повідомлення", "Link: %1": "Посилання: %1", "edited": "змінено",
 "Today": "Сьогодні", "Yesterday": "Вчора", "Chat": "Чат", "Could not read the image": "Не вдалося прочитати зображення",
 "Photo": "Фото", "(attachment)": "(вкладення)", "New chat": "Новий чат", "online": "у мережі", "No friends found.": "Друзів не знайдено.",
 "timed out": "час очікування вичерпано", "login returned no session": "вхід не повернув сесію",
 "session renewal needs verification": "для продовження сесії потрібна перевірка", "not logged in": "вхід не виконано",
 "no image data": "немає даних зображення", "getUploadUrl returned no upload_url": "getUploadUrl не повернув upload_url",
 "upload returned no photo token": "завантаження не повернуло токен фото",
 "Exit": "Вихід", "App language": "Мова застосунку", "System default": "Як у системі", "Load photos and avatars": "Завантажувати фото та аватари",
 "Off, photos in chats are loaded only when tapped - useful on a slow or metered connection.": "Якщо вимкнено, фото в чатах завантажуються лише після натискання - корисно на повільному або платному з'єднанні.",
}


def fill(path, table):
    tree = ET.parse(path)
    missing = []
    for ctx in tree.getroot().findall('context'):
        for m in ctx.findall('message'):
            src = m.find('source').text
            tr = m.find('translation')
            if src in table:
                tr.text = table[src]
                tr.attrib.pop('type', None)
            else:
                missing.append(src)
    tree.write(path, encoding='utf-8', xml_declaration=True)
    return missing


if __name__ == '__main__':
    import os
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    for lang, table in (("ru", RU), ("uk", UK)):
        miss = fill(os.path.join(root, "translations", "simpleokm_%s.ts" % lang), table)
        print(lang, "untranslated:", len(miss), miss)
