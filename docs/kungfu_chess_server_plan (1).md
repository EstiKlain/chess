# תוכנית עבודה — שכבת השרת (Networking / Multiplayer) — Kung Fu Chess

בנויה באותה שיטה כמו מסמך הדרישות של המרצה ל-CORE, ובאותו סגנון כמו `kungfu_chess_ui_plan.md`
שכתבת ל-UI: כל שלב הוא **Server-Iteration** קטן, בעל מטרה אחת, נבנה על גבי מה שכבר עבד ונבדק
בשלב הקודם. לא עוברים לשלב הבא לפני שהקודם "ירוק" (רץ + נבדק).

**חשוב להבהיר לפני הכול (זה עלה בדיון ולכן מתועד):** מסמך המרצה על ה-CORE (Model / Rules /
RuleEngine / GameEngine / RealTimeArbiter / Controller / Renderer / Text I/O) הוא **לא** הגדרת
היקף לתוכנית הזו — הוא רק **דוגמת סגנון**: פורמט Iteration, טבלת "owns / must not own", משמעת
בדיקות (unit לפני implementation, fake clock במקום sleep אמיתי), ומינוח דפוסים (Application
Service, Strategy, DTO...). התוכנית שלפניך מאמצת בדיוק את אותה משמעת עבור שכבת השרת, שהיא
תוספת חדשה **מעל** ה-CORE הקיים — לא תחליף לו.

**מיקום ה-CORE — ריצה מול תיקיות (שתי שאלות שונות, לא לבלבל):**
- **ריצה (runtime):** ה-CORE (`src/core/`) מתקמפל **בתוך** תהליך `chess_server` ורץ שם בלבד.
  הוא לא שירות נפרד ולא רץ במקום אחר — הוא ה"מוח" שיושב בזיכרון של תהליך השרת ומכריע כל מהלך.
  הקליינט לא מכיל אותו כלל; הוא רק מצייר את מה שהשרת משדר (`STATE_UPDATE`).
- **תיקיות (source layout):** `src/core/` נשאר תיקייה **אחות** ל-`src/server/`, לא תת-תיקייה
  בתוכה — כדי לשקף נכון ש-`core/` הוא Domain משותף (היום: `chess_gui`; מהיום: גם
  `chess_server`), ולאכוף חזותית שכיוון התלות תמיד `server → core`, לעולם לא הפוך.

**החלטה נוספת שהתקבלה בדיון (משנה את מבנה הבדיקות מול מה שהוצע קודם):** `chess_game`
(מצב הטקסט המקורי) שימש רק לתחילת הפיתוח וכיום לא משמעותי — לא נשקיע בו יותר. **הטסטים
של שכבת השרת רצים מתוך `chess_server`/target ייעודי לה**, לא דרך `chess_game`.

---

## מבנה תיקיות מלא (מעודכן לשכבת השרת)

```
src/
  core/                          <- קיים, ללא שינוי (Domain: Board, Rules, GameEngine...)
                                    צרכן יחיד כעת בפועל: chess_gui (מקומי) + chess_server (חדש).
                                    chess_game (טקסט) עדיין מתקמפל אך אינו יעד פיתוח פעיל.
  view/                          <- קיים, ללא שינוי (רנדור צד-קליינט)
  app/
    main_console.cpp             <- קיים (chess_game, לא בפיתוח פעיל)
    main_gui.cpp                 <- קיים, ישונה בהמשך: Controller ידבר עם ServerConnection
                                    במקום עם GameEngine מקומי (התבנית כבר תומכת בזה היום)

  client_net/                    [חדש] צד קליינט, מקביל לשרת
    ServerConnection.cpp/.hpp     # מממש את אותה חתימה ש-Controller כבר מצפה לה
                                   # (requestMove/requestJump), רק ששולחת JSON ולא קוראת ל-Engine
    ClientLogger.cpp/.hpp         # אותו envelope כמו השרת, direction SENT/RECEIVED
    WindowsInputDialog.cpp/.hpp   # Win32 בלבד (CreateWindowEx/AllocConsole) - לא ספריית עיצוב

  server/                        [חדש] כל השרת
    domain_ports/                 # ports: Application מגדיר, Infrastructure מממש
      IEventBus.hpp
      ITransport.hpp
      ISessionStore.hpp
      IUserRepository.hpp
      IMatchmakingQueue.hpp
      IRoomStore.hpp
      IClock.hpp
      ILogger.hpp
    application/                  # use-cases - אפס תלות ב-WS/SQLite/JSON, רק ב-core/ + ports
      GameSession.cpp/.hpp         # עוטפת GameEngine אחד לכל משחק
      ConnectionManager.cpp/.hpp
      LoginUseCase.cpp/.hpp
      MakeMoveUseCase.cpp/.hpp
      DisconnectUseCase.cpp/.hpp
      ReconnectUseCase.cpp/.hpp
      PlayRequestUseCase.cpp/.hpp
      RoomUseCase.cpp/.hpp
      EloService.cpp/.hpp          # פונקציה טהורה, ניתנת ל-unit test בלי שום port
    infrastructure/
      transport/     WebSocketTransport.cpp/.hpp
      persistence/    SqliteUserRepository.cpp/.hpp, InMemorySessionStore.cpp/.hpp
      matchmaking/    InMemoryMatchmakingQueue.cpp/.hpp
      rooms/          InMemoryRoomStore.cpp/.hpp
      bus/            InProcessEventBus.cpp/.hpp
      logging/        FileLogger.cpp/.hpp
      time/           SystemClock.cpp/.hpp
    protocol/
      dto/            MessageEnvelope.hpp, LoginDto.hpp, MoveDto.hpp, StateUpdateDto.hpp,
                       PlayDto.hpp, RoomDto.hpp, ErrorDto.hpp, DisconnectDto.hpp
      mappers/        GameSnapshotMapper.cpp/.hpp, MoveRequestMapper.cpp/.hpp
      MessageRouter.cpp/.hpp       # type -> use-case dispatch
    events/
      GameEvents.hpp                # MoveApplied, GameOver, PlayerDisconnected, PlayerReconnected...
    main_server.cpp                 # Composition Root - כל ה-DI הידני, אין container

tests/
  unit/                          <- קיים (Board/Movement/GameEngine/... - ללא שינוי)
  unit/server/                   [חדש] טסטים לשכבת השרת בלבד
    EloService.tests.cpp          # טהור, בלי DB/רשת
    MakeMoveUseCase.tests.cpp     # core אמיתי + IEventBus מזויף
    LoginUseCase.tests.cpp        # ISessionStore מזויף
    DisconnectReconnect.tests.cpp # IClock מזויף - מתקדמים 19s/20s, בלי sleep אמיתי!
    PlayRequestUseCase.tests.cpp  # IMatchmakingQueue + IClock מזויפים
    RoomUseCase.tests.cpp         # IRoomStore מזויף
    MessageRouter.tests.cpp       # ITransport מזויף
    Mappers.tests.cpp             # round-trip: Domain -> DTO -> JSON -> DTO -> Domain
  integration/
    server_smoke/                 # שרת אמיתי + קליינט-בדיקה אמיתי על WS מקומי, תרחיש קצר מקצה-לקצה
```

**עיקרון בדיקות (כמו אצל המרצה):** כל use-case נבדק עם `core/` **האמיתי** (הוא כבר מכוסה
ובדוק) ועם **fakes** לכל ה-ports (Bus/Clock/SessionStore/Queue/RoomStore) - לעולם לא עם
WebSocket/SQLite אמיתיים ב-unit test. IO אמיתי (Sqlite, WS) נבדק רק ב-`integration/`, מעטים
ומוגדרים בבירור.

---

## טבלת בעלות-שכבות (בהשראת טבלת "owns / must not own" של המרצה)

| שכבה | מכילה | לא מכילה |
|---|---|---|
| `core/` (Domain) | חוקי משחק, מצב לוח, זמן משחק (`wait`) | JSON, WebSocket, SQLite, matchmaking, session |
| `application/` (use-cases) | תיאום בין בקשה נכנסת ל-`core/` דרך `ports` בלבד | מימוש WS/SQLite/JSON, כל include של ספריית צד-שלישי |
| `domain_ports/` | חוזים מופשטים (interfaces) בלבד | מימוש כלשהו |
| `infrastructure/` | מימוש קונקרטי של port בודד | לוגיקת משחק, כללי Elo/matchmaking |
| `protocol/` | JSON envelope + מיפוי Domain<->DTO | לוגיקה עסקית, גישה ל-ports |
| `main_server.cpp` | הרכבה (DI ידני) בלבד | כל לוגיקה שאינה "מי מקבל למי" |

הכלל המרכזי: **חץ התלות תמיד פנימה** - `infrastructure/protocol -> application -> domain_ports -> core`,
לעולם לא הפוך. `application/` אף פעם לא `#include` קובץ מ-`infrastructure/`.

---

## פרוטוקול ההודעות (הוסכם)

מעטפת אחידה לכל הודעה, בשני הכיוונים:
```json
{ "type": "MOVE", "requestId": "uuid", "payload": { "from": "e2", "to": "e5" } }
```

| type | כיוון | payload |
|---|---|---|
| `LOGIN` | client->server | `{ username }` -> `{ username, password }` (איטרציה 6) |
| `MOVE` / `JUMP` | client->server | `{ from, to }` |
| `STATE_UPDATE` | server->client | `GameSnapshot` ממופה ל-JSON, + `role` (player/viewer) |
| `PLAY` | client->server | `{}` |
| `MATCH_FOUND` / `NO_MATCH_FOUND` | server->client | `{ opponent, color }` / `{}` |
| `ROOM_CREATE` / `ROOM_JOIN` | client->server | `{ roomId? }` |
| `DISCONNECT_COUNTDOWN` | server->client | `{ secondsLeft }` |
| `RECONNECT` | client->server | `{ sessionToken }` |
| `ERROR` | server->client | `{ code, message }` |

**קודי שגיאה (מחרוזתיים, לא HTTP status):**
`AUTH_REQUIRED`, `INVALID_CREDENTIALS`, `ILLEGAL_MOVE`, `NO_MATCH_FOUND`, `ROOM_NOT_FOUND`,
`SESSION_EXPIRED`, `INTERNAL_ERROR`.

---

## Server-Iteration 1 — תשתית: Bus, Transport, Envelope, Composition Root

**מטרה:** להוכיח שהודעה עוברת קליינט->שרת->קליינט, לפני שקיימת לוגיקת משחק ברשת בכלל.

**מוקד:** רק `WebSocketTransport` ו-`MessageEnvelope` מכירים JSON גולמי. `IEventBus` מוטמע
מהיום הראשון (הוחלט: לא נדחה לשלב מאוחר).

**מחלקות/קבצים:** `IEventBus`, `InProcessEventBus`, `ITransport`, `WebSocketTransport`,
`MessageEnvelope.hpp`, `MessageRouter`, `ConnectionManager` (registry בלבד, ללא צבעים/משחק
עדיין), `main_server.cpp`.

**התנהגות נדרשת:** השרת עולה ומקבל חיבור WS; קליינט שולח `type: "PING"`; השרת מגיב
`type: "PONG"` דרך `MessageRouter` ו-`IEventBus` (לא קריאה ישירה).

**בדיקות:** `MessageRouter` עם `ITransport` מזויף. `InProcessEventBus` - publish/subscribe עם
subscriber מזויף שסופר קריאות, בלי רשת בכלל. Smoke test ידני: הרצת שרת + קליינט מקומי אחד.

---

## Server-Iteration 2 — משחק דו-שחקני על הרשת

**מטרה:** `MOVE`/`JUMP` אמיתיים מניעים `GameEngine` אחד בצד שרת; שני קליינטים משחקים.

**מוקד:** `GameSession` עוטפת `GameEngine` קיים (בלי לגעת בו). `ConnectionManager` ממפה שני
החיבורים הראשונים לצבעים. ה-Mapper-ים ממירים `GameSnapshot`/`MoveRequest` הדומייניים ל-DTO -
זו הנקודה **היחידה** במערכת שמכירה גם את `core/` וגם את `protocol/`.

**מחלקות/קבצים:** `GameSession`, `MakeMoveUseCase`, `MoveRequestMapper`, `GameSnapshotMapper`,
`MoveDto`, `StateUpdateDto`.

**התנהגות נדרשת:** ראשון שמתחבר=לבן, שני=שחור (תואם לשקף "Support only 2 players"); שלישי
נדחה עדיין (אין Play/Room). לאחר `MOVE` מתקבל - `STATE_UPDATE` משודר **לשני** הקליינטים, לא
רק ליוזם.

**בדיקות:** `MakeMoveUseCase` עם `core/` אמיתי + `IEventBus` מזויף - מהלך חוקי מפרסם
`MoveApplied`, לא חוקי מחזיר `ILLEGAL_MOVE` (ה-`reason` הקיים ב-`MoveResult` הופך ל-`message`).
Round-trip ל-Mappers: Domain->DTO->JSON->DTO->Domain שווה למקור. ידני: שני חלונות מזיזים כלים
שונים.

---

## Server-Iteration 3 — לוגין שם-משתמש (shell) + הצגה ב-HUD

**מטרה:** קליינט מזדהה בשם; השרת משייך שם לחיבור; שני השמות מוצגים על המסך.

**מוקד:** `LOGIN` חייב לקרות לפני כל `type` אחר (`AUTH_REQUIRED` guard ב-`MessageRouter`).
קלט הטקסט בקליינט הוא **shell/console בלבד** (`AllocConsole` + `std::cin`) - לא GUI, בדיוק
כפי שהוגדר בשקף.

**מחלקות/קבצים:** `LoginUseCase`, `ISessionStore` + `InMemorySessionStore`, `LoginDto`.

**התנהגות נדרשת:** קליינט פותח קונסולה, מבקש username, שולח `LOGIN`; השרת שומר session;
ה-HUD (הרנדרר הקיים) מציג את שני השמות מתוך `STATE_UPDATE` מורחב.

**בדיקות:** `LoginUseCase` - username ריק נדחה; פקודה שנשלחת לפני `LOGIN` מחזירה
`AUTH_REQUIRED`.

---

## Server-Iteration 4 — לוגים דו-צדדיים דרך ה-Bus

**מטרה:** כל הודעה שנשלחת/מתקבלת נרשמת לקובץ, גם בשרת גם בקליינט, באותו פורמט.

**מוקד:** `Logger` הוא **subscriber** על ה-`IEventBus` בשרת (לא קריאה ישירה מכל use-case).
בקליינט - `ClientLogger` עוטף את `ServerConnection`.

**מחלקות/קבצים:** `ILogger` + `FileLogger`, `ClientLogger` (ב-`client_net/`).

**התנהגות נדרשת:** `server.log`/`client.log`: שורה לכל הודעה - timestamp, כיוון
(`SENT`/`RECEIVED`), ה-envelope המלא.

**בדיקות:** `FileLogger` עם fake clock + fake writer (ports, בלי IO אמיתי ב-unit test). ידני:
השוואת שני הקבצים אחרי משחק קצר - צריכים להיות סימטריים.

---

## Server-Iteration 5 — Reconnect + ניתוק

**מטרה:** ניתוק -> ספירה לאחור **20 שניות** על המסך -> הפסד אוטומטי; reconnect לפני 0 מבטל.

**מוקד:** `IClock` כ-port (בדיוק כמו `engine.wait(ms)` הקיים ב-core - אין sleep אמיתי בבדיקות,
תואם למתודולוגיית המרצה). `sessionToken` מוחזר ב-`LOGIN` ומשמש לזיהוי בחיבור מחדש.
**הוחלט:** מהלכים שכבר אושרו (`requestMove` הצליח) ממשיכים לרוץ בשרת בזמן הניתוק (דרך
`GameEngine::wait`, שלא תלוי בחיבור). מהלכים שלא נשלחו **לא נאגרים** בצד קליינט (Buffering
נדחה במפורש - ניתן להוסיף בעתיד כרכיב נפרד בצד קליינט בלבד, בלי לגעת בשרת).

**מחלקות/קבצים:** `DisconnectUseCase`, `ReconnectUseCase`, `IClock` + `SystemClock`,
`DisconnectDto`, קוד שגיאה `SESSION_EXPIRED`.

**התנהגות נדרשת:** נפילת WS מזוהה <- `ConnectionManager` מסמן session כ"disconnected" <- טיימר
20s מתחיל <- `DISCONNECT_COUNTDOWN` משודר ליריב <- reconnect לפני 0 מבטל ומחזיר `STATE_UPDATE`
עדכני; אחרת - resign אוטומטי דרך `GameEngine`.

**בדיקות:** עם fake clock - מתקדמים 19s + `RECONNECT` => מתבטל; מתקדמים 20s בלי `RECONNECT` =>
resign.

---

## Server-Iteration 6 — SQLite: יוזר+סיסמה+Elo

**מטרה:** התחברות אמיתית מול DB; דירוג מתחיל מ-**1200** ונע לפי Elo סטנדרטי.

**מוקד:** `IUserRepository` כ-port; `EloService` היא **פונקציה טהורה** ללא תלות ב-DB כלל.

**מחלקות/קבצים:** `IUserRepository`, `SqliteUserRepository`, `EloService`. סכמת DB:
`users(id, username UNIQUE, password_hash, elo DEFAULT 1200, created_at)`.

**התנהגות נדרשת:** הרשמה אם המשתמש לא קיים, אימות סיסמה אם קיים; דירוג מתעדכן **רק** בסיום
משחק שמקורו ב-Play (Room הוחלט כ-unranked - ראו איטרציה 8).

**בדיקות:** `EloService` - unit tests טהורים עם וקטור מקרים ידועים (1200 מול 1200; 1200
מנצח 1600 וכו'), בלי DB בכלל. `SqliteUserRepository` - integration test על DB זמני.

---

## Server-Iteration 7 — Play / Matchmaking

**מטרה:** כפתור Play; חיפוש יריב בטווח **±100** Elo; המתנה עד דקה; `NO_MATCH_FOUND`.

**מוקד:** `IMatchmakingQueue` כ-port; טיימר שוב דרך `IClock` (ניתן לבדיקה בלי sleep אמיתי).

**מחלקות/קבצים:** `IMatchmakingQueue`, `InMemoryMatchmakingQueue`, `PlayRequestUseCase`.

**התנהגות נדרשת:** `PLAY` נכנס לתור; match בטווח ±100 (מיידי או תוך דקה) יוצר `GameSession`
חדש ושולח `MATCH_FOUND` לשניהם עם צבע - מי שהמתין יותר זמן מקבל **לבן** (הוסכם). אין match
תוך דקה => `NO_MATCH_FOUND`.

**בדיקות:** fake clock מתקדם ל-59s/61s, עם "שחקנים ממתינים" מזויפים בטווחי Elo שונים.

---

## Server-Iteration 8 — Room

**מטרה:** דיאלוג Win32 native, Create/Join/Cancel; יריב + צופים.

**מוקד:** Room מנותק לגמרי מ-Play - `RoomUseCase` נפרד לחלוטין מ-`PlayRequestUseCase`
(שתי מערכות שונות, כפי שהודגש בתמלול). **unranked**: אין קריאה ל-`EloService` ממשחקי Room.

**מחלקות/קבצים:** `IRoomStore`, `InMemoryRoomStore`, `RoomUseCase`, `RoomDto`, קוד שגיאה
`ROOM_NOT_FOUND`, `WindowsInputDialog` (Win32 API בלבד - `CreateWindowEx`, לא ספריית עיצוב).

**התנהגות נדרשת:** Create מייצר room-id, מוצג בראש המסך (רנדור בצד קליינט); Join לפי id
שהוקלד. שני הראשונים = לבן/שחור, כל הבאים = `viewer` (תפקיד מועבר ב-`STATE_UPDATE`).

**בדיקות:** `RoomUseCase` - הצטרפות ראשונה/שנייה/שלישית => תפקידים נכונים; `JOIN` ל-id לא קיים
=> `ROOM_NOT_FOUND`. `WindowsInputDialog` - לא ניתן ל-unit test אמיתי (כמו UI-Iteration A שלך);
smoke test ידני בלבד.

---

## Server-Iteration 9 — סאונד + אנימציות start/end דרך ה-Bus

**מטרה:** להשלים את שקף ה-BUS - עוד subscribers, בלי לגעת בשום use-case קיים.

**מוקד:** `SoundSubscriber`, `GameLifecycleAnimationSubscriber` - שניהם subscribers חדשים על
ה-`IEventBus` הקיים מאיטרציה 1. השרת רק משדר את האירוע (`GameStarted`/`GameOver`) כ-DTO;
ניגון הסאונד/האנימציה עצמם הם עניין של הקליינט.

**בדיקות:** subscribers עם bus מזויף שסופר קריאות (כמו UI-Iteration F שלך).

---

## סדר העבודה

עוברים על 1->9 **בסדר הזה בלבד**, כל אחד עד ש"ירוק" (רץ + נבדק) לפני המעבר לבא. בכל שיחת
עבודה הבאה - צוללים לאיטרציה אחת ספציפית: מגדירים יחד את המחלקות/קבצים המדויקים, כותבים
יחד את הבדיקות **לפני** המימוש, ואז המימוש הקטן ביותר שמעביר אותן.

## יומן החלטות (לתיעוד — עלו בדיון והשפיעו על התוכנית)

- `core/` נשאר תיקייה נפרדת מ-`server/`; רץ אך ורק בתוך תהליך השרת.
- Bus (`IEventBus`) מוטמע מאיטרציה 1, לא נדחה.
- פרוטוקול JSON עם `type`/`payload`/`requestId`, לא מחרוזות גולמיות.
- קודי שגיאה מחרוזתיים, לא HTTP status.
- אין buffering של קליקים בזמן ניתוק; מהלכים מאושרים ממשיכים לרוץ לפי זמן שרת.
- טיימר reconnect אחיד: 20 שניות.
- Elo מתעדכן רק ממשחקי Play; משחקי Room הם unranked.
- מי שהמתין יותר זמן ב-matchmaking מקבל לבן.
- `chess_game` (טקסט) אינו יעד פיתוח פעיל; טסטים חדשים רצים דרך `chess_server`/target ייעודי.
- **Concurrency ברמת הרשת (לא לבלבל עם בו-זמניות המשחק עצמו):** בו-זמניות של תנועת כלים
  (cooldown פר-כלי) היא כבר לוגיקת `core/` הקיימת ולא משתנה. אבל ברמת השרת - כשכמה חיבורי-רשת
  שולחים `MOVE`/`JUMP` ממש באותו רגע, הם עלולים לרוץ על **threads שונים** ולנסות לגעת באותו
  `GameEngine` בו-זמנית (race condition על הזיכרון, לא על חוקי המשחק). **הוחלט:** כל
  `GameSession` מבטיחה שקריאות ל-`GameEngine` שלה תמיד **מסודרות ברמת הזיכרון** (לדוגמה: `std::
  mutex` פר-session סביב הקריאה ב-`MakeMoveUseCase`) - זה לא מאט ולא משנה את חוקי הבו-זמניות
  של המשחק, רק מונע כתיבה מקבילה לאותו object. יש לממש זאת כבר באיטרציה 2, לפני שיש הרגל של
  קריאה ישירה בלי נעילה.
- **ריבוי משחקים (multi-session) מאיטרציה 2, גם אם בפועל יש session אחת עד איטרציה 7:**
  `ConnectionManager` מנוהל כ-map מ-connectionId ל-(GameSession, role) כבר מאיטרציה 2 (ולא
  כשני משתנים בודדים "white connection" / "black connection"), כך שהמעבר לכמה `GameSession`
  מקביליות באיטרציה 7 (Play) ו-8 (Room) הוא תוספת ערכים ל-map, לא שינוי בממשק/ברפקטור מבני.
