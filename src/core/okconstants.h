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
// The fixed values the protocol needs - the same set as SimpleOKM.Core's OkConstants, every
// one read out of the official Odnoklassniki Windows Phone client (v4.9, 2018). The
// application key and secret come from credentials.cfg at qmake time (OK_APP_KEY /
// OK_APP_SECRET macros, see SimpleOKM.pro), so the secret never lives in a committed file.
#ifndef OKCONSTANTS_H
#define OKCONSTANTS_H

#define OK_STRINGIFY_(x) #x
#define OK_STRINGIFY(x) OK_STRINGIFY_(x)

#ifndef OK_APP_KEY
#error "OK_APP_KEY is not defined - copy credentials.cfg.template to credentials.cfg and re-run qmake"
#endif
#ifndef OK_APP_SECRET
#error "OK_APP_SECRET is not defined - copy credentials.cfg.template to credentials.cfg and re-run qmake"
#endif

namespace OkConstants
{
    /// AppConfigurationProduction.AppPublicKey. Sent as application_key on every call.
    static const char *const ApplicationKey = OK_STRINGIFY(OK_APP_KEY);

    /// AppConfigurationProduction.AppPrivateKey. Signs login (no session secret yet). It
    /// never leaves the client.
    static const char *const ApplicationSecret = OK_STRINGIFY(OK_APP_SECRET);

    /// AppConfigurationProduction.ApiBaseUri, and the api_server login falls back to.
    static const char *const ApiBase = "https://api.ok.ru/";

    /// What the app calls itself. The application key is registered to the Windows Phone
    /// client, so the User-Agent is kept consistent with it (the same UA the Android and WP
    /// ports send) - a key from one client under a different agent is what anti-fraud
    /// notices.
    static const char *const UserAgent = "OK/4.9 (Windows Phone 8.1; ru)";

    // The API is path-based: the method name IS the path, appended to the server base.
    static const char *const LoginByPassword = "api/auth/login";
    static const char *const LoginByToken = "api/auth/loginByToken";
    static const char *const FriendsGet = "api/friends/get";
    static const char *const UsersGetInfo = "api/users/getInfo";
    static const char *const MessagesGetList = "api/messagesV2/getList";
    static const char *const MessagesGetMessages = "api/messagesV2/getMessages";
    static const char *const MessagesSend = "api/messagesV2/send";
    static const char *const MessagesEdit = "api/messagesV2/edit";
    static const char *const MessagesDelete = "api/messagesV2/deleteMessages";
    static const char *const ConversationDelete = "api/messagesV2/delete";
    static const char *const BatchExecute = "api/batch/execute";
    static const char *const PhotosGetUploadUrl = "api/photosV2/getUploadUrl";
    static const char *const MediaGetUploadUrl = "api/video/getUploadUrl";
    static const char *const VideoGet = "api/video/get";

    /// The message fields getMessages must request. Without them OK returns the paging
    /// envelope with an EMPTY "message" array and no error (see OK-Messenger-API.md, section 5).
    static const char *const MessageFields =
        "message.attachments,message.ID,message.TYPE,message.AUTHOR_ID,message.TIME,"
        "message.TIME_MS,message.TEXT,message.DATE,message.DATE_MS,message.VMAIL_VIDEO,"
        "message.VMAIL_THUMBNAIL,message.REPLY_TO_MSG_ID,message.REPLY_TO_ID,"
        "message.LIKE_COUNT,message.LIKED_IT,message.LAST_LIKE_DATE,message.LAST_LIKE_DATE_MS,"
        "message.DELETE_ALLOWED,message.LIKE_ALLOWED,message.MARK_AS_SPAM_ALLOWED,"
        "message.AUTHOR_BLOCK_ALLOWED,message.LIKES_UNREAD,message.REPLY_UNREAD,message.edit_time_ms";

    /// The fields getAttachedResources must request so a received photo comes back with its
    /// picture urls, keyed by attachment id.
    static const char *const AttachmentResourceFields =
        "attachment_photo.pic128x128,attachment_photo.pic190x190,attachment_photo.pic640x480,"
        "attachment_photo.pic1024max,attachment_movie.title,attachment_movie.thumbnail_url,"
        "attachment_movie.id,attachment_movie.url_mobile,attachment_movie.url_low,"
        "attachment_movie.url_medium,attachment_movie.url_high,attachment_movie.url_fullhd,"
        "attachment_movie.duration,attachment_movie.content_type,attachment_movie.status,"
        "attachment_audio_rec.*";

    static const char *const UserFields = "uid,first_name,last_name,pic128x128,pic190x190,online";

    // Parameter names, exactly as the client spells them.
    static const char *const ParamApplicationKey = "application_key";
    static const char *const ParamSessionKey = "session_key";
    static const char *const ParamSignature = "sig";
    static const char *const ParamUserName = "user_name";
    static const char *const ParamPassword = "password";
    static const char *const ParamGenToken = "gen_token";
    static const char *const ParamToken = "token";
    static const char *const ParamVerificationSupported = "verification_supported_v";
    static const char *const ParamVerificationToken = "verification_token";
    static const char *const ParamFormat = "format";

    /// Only this key is excluded from the signature (QueryBuilder.CalculateSignature).
    static const char *const ParamUrl = "url";
}

#endif // OKCONSTANTS_H
