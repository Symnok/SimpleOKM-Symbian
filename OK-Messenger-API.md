# OK Messenger API

How Odnoklassniki (OK / ok.ru) messaging works at the wire level, as used by `SimpleOKM.Core`.
Reverse-engineered from the official OK **Windows Phone** client (v4.9, 2018, `ok4.9.xap`,
decompiled with ilspycmd) and validated against the live server. This is the legacy REST API at
`api.ok.ru`, the same one the WP/W10M client uses — **not** the TamTam/MAX WebSocket the modern
Android app uses.

Everything here is implemented in `src/SimpleOKM.Core/` (`OkApi`, `OkConstants`, `OkSigning`,
`OkSession`, `OkParser`).

---

## 1. Transport

- **Base URL:** `https://api.ok.ru/` (or the `api_server` a login returns; fall back to the base).
- **Method = path.** The method name *is* the URL path, e.g. `api/messagesV2/getList`. There is
  no `fb.do?method=` indirection.
- **Parameters** go in the query string, form-urlencoded. A query of **≥ 1000 characters** is
  moved into a `POST` body (`application/x-www-form-urlencoded`); otherwise `GET`.
- **Format:** always pass `format=json`. Success returns a bare JSON object or array; failure
  returns `{"error_code": N, "error_msg": "..."}`.
- **User-Agent:** kept consistent with the registered client, e.g.
  `OKWindowsPhone/4.9.1 (Microsoft Windows NT 10.0.15254.0; en-US; ...)`.

### App credentials (WP client, `AppConfigurationProduction`)

| | |
|---|---|
| `application_key` | `CBADGLBBABABABABA` |
| application secret | `DBF70A7BDB74C15B48F8D3DB` (used to sign **login** only) |

---

## 2. Signing

Every call carries `sig`, computed as:

```
sig = lowercase( MD5( concat(sorted("key=value" for each param EXCEPT "url")) + secret ) )
```

- Take every request parameter **except** `url` and `sig` itself.
- Format each as the literal string `key=value`.
- **Sort those strings** ordinally (ascending), then **concatenate** with no separators.
- Append the **secret**, MD5 the whole thing, lowercase the hex.

The **secret** is:

- the **application secret** for `auth/login` / `auth/loginByToken` (no session yet), and
- the **session secret** (`session_secret_key`, returned by login) for every later call.

See `OkSigning.Signature(...)`.

---

## 3. Login

### By password — `api/auth/login`

Params: `application_key`, `user_name`, `password`, `gen_token=true`,
`verification_supported_v=1`, `format=json`, `sig` (signed with the **application secret**).

Returns `auth_token`, `session_key`, `session_secret_key`, `api_server`, `uid`.

- If OK wants a captcha / device confirm, the response carries `verification_url` — open it in a
  browser, complete it, and retry. (`OkVerificationRequired` in the core.)

### By token — `api/auth/loginByToken`

Params: `application_key`, `token` (the earlier `auth_token`), `verification_supported_v=1`,
`format=json`, `sig` (application secret). Returns a fresh session. This is what the app does on
every later start so the password is never sent again.

### Session calls

After login, every call includes `application_key`, `session_key`, `format=json`, its own params,
and `sig` signed with the **session secret**.

---

## 4. Conversations — `api/messagesV2/getList`

Params: `fields=conversation.*`, `count=<n>`.

Returns `{"conversation": [ ... ]}`. Per conversation (base64 id like `PRIVATE:uid` / `CHAT:...`):

| field | meaning |
|---|---|
| `id` | base64 conversation id (the key used everywhere) |
| `type` | `"CHAT"` for a group, else a 1-to-1 |
| `last_msg_text` | preview text |
| `last_msg_time_ms` | last message time (ms since epoch) |
| `last_author_id` | who sent the last message |
| `new_msgs_count` | unread count |
| `participant[]` | `{id, ...}` — for a 1-to-1 the *other* participant is the peer |
| `topic` | group title |

Names/avatars are not here — resolve peer uids with `users.getInfo` (§9).

---

## 5. Message history — `api/messagesV2/getMessages`

**Always called inside `api/batch/execute`** together with `getAttachedResources`, because a
message's attachments come back as bare ids and the media/picture urls must be fetched
separately and joined by id.

### getMessages params

| param | value |
|---|---|
| `cnv_id` | the base64 conversation id |
| `direction` | `Forward` (newer / initial) or `Backward` (older) |
| `anchor` | `U` (unread — the initial "latest" page), `L` (last), `F` (first), or `id:<msgId>` to page around a specific message |
| `frmt` | `PLAIN_EXT_SMILES` |
| `count` | e.g. `30` |
| `mark_as_read` | `true` / `false` |
| `fields` | **required** — see below |

> **The `fields` gotcha.** Without a `fields` list, `getMessages` returns the paging envelope
> (`is_first`, `has_more`, `anchor`) but an **empty `message[]`** — no error. You must request the
> message fields. `OkConstants.MessageFields` sends the exact list the WP client uses:
> `message.attachments,message.ID,message.TYPE,message.AUTHOR_ID,message.TIME,message.TIME_MS,message.TEXT,message.DATE,message.DATE_MS,message.VMAIL_VIDEO,message.VMAIL_THUMBNAIL,message.REPLY_TO_MSG_ID,message.REPLY_TO_ID,...,message.edit_time_ms`.
> Response keys come back **lowercase** (`id`, `text`, `type`, `author_id`, `date_ms`,
> `edit_time_ms`, `reply_to_id`, `attachments`).

### The batch

```json
{"methods":[
  {"method":"messagesV2.getMessages",         "params":{ ...as above..., "fields":"<MessageFields>" }},
  {"method":"messagesV2.getAttachedResources","params":{
      "attach_ids":{"supplier":"messagesV2.getMessages.attachment_ids"},
      "fields":"<AttachmentResourceFields>"}}
]}
```

Result keys: `messagesV2_getMessages_response` (with `message[]`, `anchor`, `has_more`) and
`messagesV2_getAttachedResources_response` (with `attachments[]`). The `attach_ids` **supplier**
feeds the message attachment ids from the first method into the second — this only works inside a
batch.

### A message

`id`, `type` (`USER` / `SYSTEM` / `STICKER` / ...), `author_id`, `text`, `date_ms`,
`edit_time_ms`, `reply_to_id`, and `attachments[]` (each `{type, id, ...}`).

---

## 6. Attachments

On the **message**, an attachment is just `{"type": ..., "id": <attachId>, ...}`. The playable /
viewable urls come from `getAttachedResources`, matched by that `id`.

`OkConstants.AttachmentResourceFields` requests:
`attachment_photo.pic128x128,attachment_photo.pic190x190,attachment_photo.pic640x480,attachment_photo.pic1024max,attachment_movie.title,attachment_movie.thumbnail_url,attachment_movie.id,attachment_movie.url_mobile,attachment_movie.url_low,attachment_movie.url_medium,attachment_movie.url_high,attachment_movie.url_fullhd,attachment_movie.duration,attachment_movie.content_type,attachment_movie.status,attachment_audio_rec.*`

| message type | resource fields → url |
|---|---|
| `PHOTO` | `pic1024max` / `pic640x480` (full), `pic190x190` / `pic128x128` (thumb) |
| `MOVIE` / `VIDEO` | `url_mobile` (fallbacks `url_medium/high/low/fullhd`); thumb `thumbnail_url`; also `duration`, `status` |
| `AUDIO_RECORDING` | `content_locations[]` — pick the entry whose `ct == "audio/mpeg"` → its `url`; also `duration`, `media_id` |

OK's messaging supports **only** PHOTO, MOVIE (video), AUDIO_RECORDING (voice note),
MUSIC (OK-catalog track) and SHARE (link) — there is **no generic file/document** attachment.

> Playback urls are handed out over **`http`** (e.g. `http://ok.ru/dk?st.cmd=moviePlaybackRedirect...`).
> Promote them to `https` (`OkParser` does this) and/or permit cleartext to OK media hosts.

---

## 7. Sending — `api/messagesV2/send`

Params: `cnv_id`, `text`, `uuid` (a fresh GUID, dedup), optional `reply_to_message_id`, and
optional `attachments` (a JSON **string**):

```json
{"attachments":[
  {"type":"UPLOADED_PHOTO","token":"<photo upload token>"},
  {"type":"UPLOADED_MOVIE","movieId":"<video_id>"},
  {"type":"ODKL_PHOTO","id":"<existing OK photo id>"}
]}
```

Returns the new message `id`. Both **video and voice** attach as `UPLOADED_MOVIE`/`movieId`
(they differ only in how they are uploaded, §9).

---

## 8. Uploading a photo

1. `api/photosV2/getUploadUrl` (no album) → `{ upload_url, photo_ids }`.
2. **POST the image as `multipart/form-data`** to `upload_url` (a raw octet body is rejected).
   Reply: `{"photos":{"<photoId>":{"token":"<token>"}}}` — take the `token`.
3. `messagesV2/send` with `{"type":"UPLOADED_PHOTO","token":"<token>"}`.

## 9. Uploading video / voice (media)

1. `api/video/getUploadUrl` with `attachment_type` = `"VIDEO"` (video) or `"AUDIO_RECORDING"`
   (voice), `file_name="File.mp4"`, `file_size="0"` → `{ upload_url, video_id }`.
   (`upload_url` is on `vu.okcdn.ru/upload.do`.)
2. **POST the bytes as `multipart/form-data`** to `upload_url` (raw octet → **HTTP 412
   Precondition Failed**). A 2xx reply is `<retval>1</retval>` (not JSON) — nothing is read from
   it; the `video_id` from step 1 is what matters.
3. **Wait for processing.** Poll `api/video/get` with `vids=<video_id>`, `fields=video.*` until
   `videos[0].status == "OK"` (terminal failures: `ERROR`, `BLOCKED`, `CENSORED`,
   `COPYRIGHTS_RESTRICTED`, `UNAVAILABLE`). Sending before this fails with
   `errors.process.attachment.video.not.processed`. Voice is near-instant; video takes seconds.
4. `messagesV2/send` with `{"type":"UPLOADED_MOVIE","movieId":"<video_id>"}`.

> **Video-send caveat (why SimpleOKM disables it).** Even after `video/get` reports `OK`, a
> freshly-sent **video** makes `messagesV2/getMessages` fail for the **entire conversation** —
> `UNKNOWN: Unexpected error` (or `MessageType is null`) — for as long as OK finishes processing
> the video message (observed **>24 s** for a 9.4 MB clip; up to minutes). It recovers on its own
> (a fully-processed video then loads and plays), but during the window the whole chat is
> unreadable on both sides. Voice does **not** have this problem. SimpleOKM therefore ships voice
> send but not video send.

## 10. Users — `api/users/getInfo`

Params: `uids` (comma-separated), `fields=uid,first_name,last_name,pic128x128,pic190x190,online`,
`emptyPictures=false`. Used to turn conversation peer uids into names and avatars.

## 11. Editing & deleting

| action | method | params |
|---|---|---|
| edit a message | `api/messagesV2/edit` | `cnv_id`, `msg_id`, `text` |
| delete messages | `api/messagesV2/deleteMessages` | `cnv_id`, `msg_ids` (comma-separated) |
| delete a conversation | `api/messagesV2/delete` | `cnv_id` |

---

## 12. Quirks & limitations

- **Chat-with-yourself** (`PRIVATE:<own uid>`) cannot be read via this API — `getMessages` fails
  with `Cannot invoke "...MessageType.ordinal()" because "messageType" is null`. Not a client bug;
  there is nothing the client can do. SimpleOKM shows a note instead of a blank chat.
- **No push / long-poll** on this REST API — clients poll. (The modern OK Android app moved
  messaging to the TamTam/MAX WebSocket at `wss://ws.tamtam.chat/websocket`; that is a different,
  opcode-based protocol and is *not* what this document or `SimpleOKM.Core` uses.)
- **`fields` is mandatory** on `getMessages` (§5) or you get an empty message list with no error.
- **Uploads are multipart**, not raw octet, for both photos and media (§8, §9).
- **Media urls are http** and must be promoted to https / allowed as cleartext (§6).
