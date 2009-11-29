#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QMap>
#include <QHash>
#include <QObject>
#include <QVariant>
#include "protocol_item_ids.h"
#include "protocol_datastructures.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QXmlStreamAttributes;

class ProtocolResponse;
class DeckList;

enum ItemId {
	ItemId_Command_DeckUpload = ItemId_Other + 100,
	ItemId_Command_DeckSelect = ItemId_Other + 101,
	ItemId_Event_ListChatChannels = ItemId_Other + 200,
	ItemId_Event_ChatListPlayers = ItemId_Other + 201,
	ItemId_Event_ListGames = ItemId_Other + 202,
	ItemId_Event_GameStateChanged = ItemId_Other + 203,
	ItemId_Event_CreateArrows = ItemId_Other + 204,
	ItemId_Event_CreateCounters = ItemId_Other + 205,
	ItemId_Event_DrawCards = ItemId_Other + 206,
	ItemId_Response_DeckList = ItemId_Other + 300,
	ItemId_Response_DeckDownload = ItemId_Other + 301,
	ItemId_Response_DeckUpload = ItemId_Other + 302,
	ItemId_Invalid = ItemId_Other + 1000
};

class ProtocolItem : public SerializableItem_Map {
	Q_OBJECT
private:
	static void initializeHashAuto();
public:
	static const int protocolVersion = 5;
	static void initializeHash();
	virtual int getItemId() const = 0;
	ProtocolItem(const QString &_itemType, const QString &_itemSubType);
};

class ProtocolItem_Invalid : public ProtocolItem {
public:
	ProtocolItem_Invalid() : ProtocolItem(QString(), QString()) { }
	int getItemId() const { return ItemId_Invalid; }
};

class TopLevelProtocolItem : public SerializableItem {
	Q_OBJECT
signals:
	void protocolItemReceived(ProtocolItem *item);
private:
	ProtocolItem *currentItem;
	bool readCurrentItem(QXmlStreamReader *xml);
protected:
	void readElement(QXmlStreamReader *xml);
	void writeElement(QXmlStreamWriter *xml);
public:
	TopLevelProtocolItem();
};

// ----------------
// --- COMMANDS ---
// ----------------

class Command : public ProtocolItem {
	Q_OBJECT
signals:
	void finished(ProtocolResponse *response);
	void finished(ResponseCode response);
private:
	int ticks;
	static int lastCmdId;
	QVariant extraData;
public:
	Command(const QString &_itemName = QString(), int _cmdId = -1);
	int getCmdId() const { return static_cast<SerializableItem_Int *>(itemMap.value("cmd_id"))->getData(); }
	int tick() { return ++ticks; }
	void processResponse(ProtocolResponse *response);
	void setExtraData(const QVariant &_extraData) { extraData = _extraData; }
	QVariant getExtraData() const { return extraData; }
};

class ChatCommand : public Command {
	Q_OBJECT
public:
	ChatCommand(const QString &_cmdName, const QString &_channel)
		: Command(_cmdName)
	{
		insertItem(new SerializableItem_String("channel", _channel));
	}
	QString getChannel() const { return static_cast<SerializableItem_String *>(itemMap.value("channel"))->getData(); }
};

class GameCommand : public Command {
	Q_OBJECT
public:
	GameCommand(const QString &_cmdName, int _gameId)
		: Command(_cmdName)
	{
		insertItem(new SerializableItem_Int("game_id", _gameId));
	}
	int getGameId() const { return static_cast<SerializableItem_Int *>(itemMap.value("game_id"))->getData(); }
	void setGameId(int _gameId) { static_cast<SerializableItem_Int *>(itemMap.value("game_id"))->setData(_gameId); }
};

class Command_DeckUpload : public Command {
	Q_OBJECT
public:
	Command_DeckUpload(DeckList *_deck = 0, const QString &_path = QString());
	static SerializableItem *newItem() { return new Command_DeckUpload; }
	int getItemId() const { return ItemId_Command_DeckUpload; }
	DeckList *getDeck() const;
	QString getPath() const { return static_cast<SerializableItem_String *>(itemMap.value("path"))->getData(); }
};

class Command_DeckSelect : public GameCommand {
	Q_OBJECT
public:
	Command_DeckSelect(int _gameId = -1, DeckList *_deck = 0, int _deckId = -1);
	static SerializableItem *newItem() { return new Command_DeckSelect; }
	int getItemId() const { return ItemId_Command_DeckSelect; }
	DeckList *getDeck() const;
	int getDeckId() const { return static_cast<SerializableItem_Int *>(itemMap.value("deck_id"))->getData(); }
};

// -----------------
// --- RESPONSES ---
// -----------------

class ProtocolResponse : public ProtocolItem {
	Q_OBJECT
private:
	static QHash<QString, ResponseCode> responseHash;
public:
	ProtocolResponse(int _cmdId = -1, ResponseCode _responseCode = RespOk, const QString &_itemName = QString());
	int getItemId() const { return ItemId_Other; }
	static void initializeHash();
	static SerializableItem *newItem() { return new ProtocolResponse; }
	int getCmdId() const { return static_cast<SerializableItem_Int *>(itemMap.value("cmd_id"))->getData(); }
	ResponseCode getResponseCode() const { return responseHash.value(static_cast<SerializableItem_String *>(itemMap.value("response_code"))->getData(), RespOk); }
};

class Response_DeckList : public ProtocolResponse {
	Q_OBJECT
public:
	Response_DeckList(int _cmdId = -1, ResponseCode _responseCode = RespOk, DeckList_Directory *_root = 0);
	int getItemId() const { return ItemId_Response_DeckList; }
	static SerializableItem *newItem() { return new Response_DeckList; }
	DeckList_Directory *getRoot() const { return static_cast<DeckList_Directory *>(itemMap.value("directory")); }
};

class Response_DeckDownload : public ProtocolResponse {
	Q_OBJECT
public:
	Response_DeckDownload(int _cmdId = -1, ResponseCode _responseCode = RespOk, DeckList *_deck = 0);
	int getItemId() const { return ItemId_Response_DeckDownload; }
	static SerializableItem *newItem() { return new Response_DeckDownload; }
	DeckList *getDeck() const;
};

class Response_DeckUpload : public ProtocolResponse {
	Q_OBJECT
public:
	Response_DeckUpload(int _cmdId = -1, ResponseCode _responseCode = RespOk, DeckList_File *_file = 0);
	int getItemId() const { return ItemId_Response_DeckUpload; }
	static SerializableItem *newItem() { return new Response_DeckUpload; }
	DeckList_File *getFile() const { return static_cast<DeckList_File *>(itemMap.value("file")); }
};

// --------------
// --- EVENTS ---
// --------------

class GenericEvent : public ProtocolItem {
	Q_OBJECT
public:
	GenericEvent(const QString &_eventName)
		: ProtocolItem("generic_event", _eventName) { }
};

class GameEvent : public ProtocolItem {
	Q_OBJECT
public:
	GameEvent(const QString &_eventName, int _gameId, int _playerId);
	int getGameId() const { return static_cast<SerializableItem_Int *>(itemMap.value("game_id"))->getData(); }
	int getPlayerId() const { return static_cast<SerializableItem_Int *>(itemMap.value("player_id"))->getData(); }
	void setGameId(int _gameId) { static_cast<SerializableItem_Int *>(itemMap.value("game_id"))->setData(_gameId); }
};

class ChatEvent : public ProtocolItem {
	Q_OBJECT
public:
	ChatEvent(const QString &_eventName, const QString &_channel);
	QString getChannel() const { return static_cast<SerializableItem_String *>(itemMap.value("channel"))->getData(); }
};

class Event_ListChatChannels : public GenericEvent {
	Q_OBJECT
public:
	Event_ListChatChannels(const QList<ServerInfo_ChatChannel *> &_channelList = QList<ServerInfo_ChatChannel *>());
	int getItemId() const { return ItemId_Event_ListChatChannels; }
	static SerializableItem *newItem() { return new Event_ListChatChannels; }
	QList<ServerInfo_ChatChannel *> getChannelList() const { return typecastItemList<ServerInfo_ChatChannel *>(); }
};

class Event_ChatListPlayers : public ChatEvent {
	Q_OBJECT
public:
	Event_ChatListPlayers(const QString &_channel = QString(), const QList<ServerInfo_ChatUser *> &_playerList = QList<ServerInfo_ChatUser *>());
	int getItemId() const { return ItemId_Event_ChatListPlayers; }
	static SerializableItem *newItem() { return new Event_ChatListPlayers; }
	QList<ServerInfo_ChatUser *> getPlayerList() const { return typecastItemList<ServerInfo_ChatUser *>(); }
};

class Event_ListGames : public GenericEvent {
	Q_OBJECT
public:
	Event_ListGames(const QList<ServerInfo_Game *> &_gameList = QList<ServerInfo_Game *>());
	int getItemId() const { return ItemId_Event_ListGames; }
	static SerializableItem *newItem() { return new Event_ListGames; }
	QList<ServerInfo_Game *> getGameList() const { return typecastItemList<ServerInfo_Game *>(); }
};

class Event_GameStateChanged : public GameEvent {
	Q_OBJECT
public:
	Event_GameStateChanged(int _gameId = -1, const QList<ServerInfo_Player *> &_playerList = QList<ServerInfo_Player *>());
	static SerializableItem *newItem() { return new Event_GameStateChanged; }
	int getItemId() const { return ItemId_Event_GameStateChanged; }
	QList<ServerInfo_Player *> getPlayerList() const { return typecastItemList<ServerInfo_Player *>(); }
};

class Event_CreateArrows : public GameEvent {
	Q_OBJECT
public:
	Event_CreateArrows(int _gameId = -1, int _playerId = -1, const QList<ServerInfo_Arrow *> &_arrowList = QList<ServerInfo_Arrow *>());
	int getItemId() const { return ItemId_Event_CreateArrows; }
	static SerializableItem *newItem() { return new Event_CreateArrows; }
	QList<ServerInfo_Arrow *> getArrowList() const { return typecastItemList<ServerInfo_Arrow *>(); }
};

class Event_CreateCounters : public GameEvent {
	Q_OBJECT
public:
	Event_CreateCounters(int _gameId = -1, int _playerId = -1, const QList<ServerInfo_Counter *> &_counterList = QList<ServerInfo_Counter *>());
	int getItemId() const { return ItemId_Event_CreateCounters; }
	static SerializableItem *newItem() { return new Event_CreateCounters; }
	QList<ServerInfo_Counter *> getCounterList() const { return typecastItemList<ServerInfo_Counter *>(); }
};

class Event_DrawCards : public GameEvent {
	Q_OBJECT
public:
	Event_DrawCards(int _gameId = -1, int _playerId = -1, int numberCards = -1, const QList<ServerInfo_Card *> &_cardList = QList<ServerInfo_Card *>());
	int getItemId() const { return ItemId_Event_DrawCards; }
	static SerializableItem *newItem() { return new Event_DrawCards; }
	int getNumberCards() const { return static_cast<SerializableItem_Int *>(itemMap.value("number_cards"))->getData(); }
	QList<ServerInfo_Card *> getCardList() const { return typecastItemList<ServerInfo_Card *>(); }
};

#endif