# Telegram Setup via API (Bot Token & Chat ID)

This firmware sends alerts through the Telegram Bot API. You need:
- **`TG_BOT_TOKEN`** — the bot’s API token from **@BotFather**
- **`TG_CHAT_ID`** — the destination (your private chat ID or a group chat ID)



## 1) Create a bot (get the token)

1. In Telegram, open **@BotFather**.
2. Send: `/newbot`
3. Follow prompts (choose a name + a unique username ending with `bot`).
4. Copy the token BotFather returns (looks like `123456789:AA...`).

> Keep this token secret. If exposed, regenerate in @BotFather with `/token`.



## 2) Get your private (DM) chat_id via API

1. Open a chat with your bot and send `/start`.
2. In a browser, open (replace `BOT_TOKEN` with your token):
https://api.telegram.org/botBOT_TOKEN/getUpdates
3. Find a block like:
```json
{
  "message": {
    "chat": { "id": 1090747743, "type": "private", ... },
    ...
  }
}
```
4. The numeric id (e.g., 1090747743) is your `TG_CHAT_ID` for direct messages.

>If result is empty (`{"ok":true,"result":[]}`), send a new message to the bot and refresh the URL.

## 3) Get a group chat_id via API

1. Create or open a Telegram group.

2. Add your bot to the group.

3. Send a message in the group (e.g., hello).

4. Open (replace BOT_TOKEN):

https://api.telegram.org/botBOT_TOKEN/getUpdates


5. Find the update for that group message:
```
{
  "message": {
    "chat": { "id": -123456789, "type": "group", ... },
    ...
  }
}
````

6. Copy that negative id (e.g., -123456789) — that is the group TG_CHAT_ID.

## 4) Quick API test

Use a browser (replace placeholders):

[https://api.telegram.org/botBOT_TOKEN/sendMessage?chat_id=CHAT_ID&text=Hello%20from%20ESP32](https://api.telegram.org/botBOT_TOKEN/sendMessage?chat_id=CHAT_ID&text=Hello%20from%20ESP32)

You should immediately receive the message in Telegram.

## 5) Add to `include/credentials.h`
```
#define TG_BOT_TOKEN  "123456789:AA...your-long-secret-token..."
#define TG_CHAT_ID    "1090747743"       
```

`include/credentials.h` is gitignored. Never commit real tokens.