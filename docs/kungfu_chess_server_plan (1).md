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

**את כל הקבועים יש לשים בקובץ של קובעים ולא להשאיר מספרים בקוד
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
      IIdentityStore.hpp            # בפועל: לא ISessionStore - GameSession כבר תופס את "session"
      IUserRepository.hpp
      IMatchmakingQueue.hpp
      IRoomStore.hpp
      IClock.hpp
      ILogger.hpp
    application/                  # use-cases - אפס תלות ב-WS/SQLite/JSON, רק ב-core/ + ports
      GameSession.cpp/.hpp         # עוטפת GameEngine אחד לכל משחק
      ConnectionManager.cpp/.hpp
      LoginUseCase.cpp/.hpp
      AuthGuard.cpp/.hpp           # decorator סביב IEventBus - אוכף LOGIN-לפני-הכל, לא ב-MessageRouter
      MakeMoveUseCase.cpp/.hpp
      DisconnectUseCase.cpp/.hpp
      ReconnectUseCase.cpp/.hpp
      PlayRequestUseCase.cpp/.hpp
      RoomUseCase.cpp/.hpp
      EloService.cpp/.hpp          # פונקציה טהורה, ניתנת ל-unit test בלי שום port
    infrastructure/
      transport/     WebSocketTransport.cpp/.hpp
      persistence/    SqliteUserRepository.cpp/.hpp, InMemoryIdentityStore.cpp/.hpp
      matchmaking/    InMemoryMatchmakingQueue.cpp/.hpp
      rooms/          InMemoryRoomStore.cpp/.hpp
      bus/            InProcessEventBus.cpp/.hpp
      logging/        FileLogger.cpp/.hpp
      time/           SystemClock.cpp/.hpp
    protocol/
      dto/            MessageEnvelope.hpp, LoginDto.hpp, MoveDto.hpp, StateUpdateDto.hpp,
                       PlayDto.hpp, RoomDto.hpp, ErrorDto.hpp, DisconnectDto.hpp
      mappers/        GameSnapshotMapper.cpp/.hpp, MoveRequestMapper.cpp/.hpp
      MessageRouter.cpp/.hpp       # מפרסר JSON גולמי ומפרסם BusEvent - לא מחליט dispatch לפי type בעצמו; ה-dispatch קורה דרך bus.subscribe(...) ב-main_server.cpp (composition root)
    events/
      GameEvents.hpp                # MoveApplied, GameOver, PlayerDisconnected, PlayerReconnected...
    main_server.cpp                 # Composition Root - כל ה-DI הידני, אין container

tests/
  unit/                          <- קיים (Board/Movement/GameEngine/... - ללא שינוי)
  unit/server/                   [חדש] טסטים לשכבת השרת בלבד
    EloService.tests.cpp          # טהור, בלי DB/רשת
    MakeMoveUseCase.tests.cpp     # core אמיתי + IEventBus מזויף
    LoginUseCase.tests.cpp        # IIdentityStore מזויף
    AuthGuard.tests.cpp           # IEventBus עטוף מזויף + IIdentityStore מזויף + ITransport מזויף
    DisconnectReconnect.tests.cpp # IClock מזויף - מתקדמים 19s/20s, בלי sleep אמיתי!
    PlayRequestUseCase.tests.cpp  # IMatchmakingQueue + IClock מזויפים
    RoomUseCase.tests.cpp         # IRoomStore מזויף
    MessageRouter.tests.cpp       # ITransport מזויף
    Mappers.tests.cpp             # round-trip: Domain -> DTO -> JSON -> DTO -> Domain
  integration/
    server_smoke/                 # שרת אמיתי + קליינט-בדיקה אמיתי על WS מקומי, תרחיש קצר מקצה-לקצה
```

**עיקרון בדיקות (כמו אצל המרצה):** כל use-case נבדק עם `core/` **האמיתי** (הוא כבר מכוסה
ובדוק) ועם **fakes** לכל ה-ports (Bus/Clock/IdentityStore/Queue/RoomStore) - לעולם לא עם
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
{ "type": "MOVE", "requestId": "uuid", "payload": { "fromRow": 1, "fromCol": 4, "toRow": 3, "toCol": 4 } }
```
**תוקן בפועל באיטרציה 2 (לא היה ברור עדיין כשהטבלה הזאת נכתבה לראשונה):** אין notation אלגברי
("e2"/"e4") בשום מקום ב-`core/`/`view/` - `BoardMapper::pixelToCell` כבר ממיר פיקסלים ישר ל-`Position{row,col}`
מספרי, בלי שלב אותיות/files. לכן ה-DTO-ים נושאים קואורדינטות מספריות גולמיות, לא מחרוזת algebraic.
`JumpDto{row,col}` הוא **DTO נפרד** מ-`MoveDto{fromRow,fromCol,toRow,toCol}`, לא אותה צורה -
`GameEngine::requestJump(row,col)` מקבל מיקום בודד, לא זוג {from,to}.

| type | כיוון | payload |
|---|---|---|
| `LOGIN` | client->server | `{ username }` -> `{ username, password }` (איטרציה 6) |
| `LOGIN_OK` | server->client | `{ sessionToken }` (איטרציה 5 - נדרש ל-`RECONNECT` מאוחר יותר) |
| `MOVE` | client->server | `{ fromRow, fromCol, toRow, toCol }` |
| `JUMP` | client->server | `{ row, col }` - **לא** `{from,to}`, ראו הערה למעלה |
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

**נוספו בפועל באיטרציה 2 (לא היו ברשימה המקורית - מתועד כאן בדיעבד כדי שהמסמך ישקף את המציאות):**
- `TABLE_FULL` — חיבור שלישי, בזמן שאין עדיין Play/Room (איטרציות 7-8) שיכולים לקבל אותו לתפקיד צופה.
- `MALFORMED_PAYLOAD` — payload של MOVE/JUMP חסר שדות/מהצורה הלא נכונה (קלט רע **מהצד השני**). נבדל בכוונה מ-`INTERNAL_ERROR`, ששמור למצב שלא-אמור-לקרות **בצד שלנו** (למשל connectionId לא רשום לשום session).

**‏`SESSION_EXPIRED`‏ (איטרציה 5) - שתי משמעויות זהות ללקוח, לא נבדלות בכוונה:** מוחזר גם עבור `sessionToken`‏ שלא קיים בכלל, וגם עבור טוקן שכבר לא נמצא במצב "מנותק וממתין" (חלון ה-20 שניות כבר חלף וה-resign כבר בוצע, **או** ניסיון `RECONNECT`‏ כפול על אותו טוקן שכבר הצליח פעם אחת - `PlayerSessionRegistry::reconnect`‏ מסרב לשני המקרים באותו אופן). מבחינת הלקוח שתי הסיבות אומרות אותו דבר בדיוק - "אין משחק לחזור אליו" - הבחנה ביניהן הייתה דורשת לשמור רשומות "מצבה" (tombstone) לצמיתות עבור הבדל שאף פעם לא נראה למשתמש.

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

**מוקד:** `LOGIN` חייב לקרות לפני כל `type` אחר. **תוקן בפועל בעת המימוש (לא כפי שנוסח כאן
במקור):** ה-guard **אינו** בתוך `MessageRouter` - `MessageRouter` נשאר "טיפש" לגמרי (מפרסר+מפרסם
בלבד, בלי לדעת שאימות בכלל קיים), בדיוק לפי טבלת הבעלות-שכבות למעלה. האכיפה חיה במחלקה נפרדת,
`AuthGuard`, שמממשת בעצמה `IEventBus` ועוטפת (decorator) את ה-bus האמיתי - מחוברת בין
`InProcessEventBus` ל-`MessageRouter` ב-`main_server.cpp` (composition root). קלט הטקסט בקליינט
הוא **shell/console בלבד** (`AllocConsole` + `std::cin`) - לא GUI, בדיוק כפי שהוגדר בשקף.

**מחלקות/קבצים:** `LoginUseCase`, `AuthGuard`, `IIdentityStore` + `InMemoryIdentityStore`, `LoginDto`.
(שם ה-port הוא `IIdentityStore`, לא `ISessionStore` כפי שנוסח כאן במקור - `GameSession` כבר
תופס את המשמעות "session" בקוד הקיים, לא כדאי לשם השני להתנגש בו.)

**התנהגות נדרשת:** קליינט פותח קונסולה, מבקש username, שולח `LOGIN`; השרת שומר session;
ה-HUD (הרנדרר הקיים) מציג את שני השמות מתוך `STATE_UPDATE` מורחב.

**נמצא בביקורת לאחר המימוש (לא תוקן באיטרציה זו, ראו Iteration 3.5):** השורה למעלה לא ניתנת
למימוש בפועל - אין עדיין שום `client_net/ServerConnection`, ו-`main_gui.cpp` אף פעם לא מתוזמן
לשינוי באף איטרציה. מה שנבנה ונבדק ב-Iteration 3 הוא **צד השרת בלבד** - `LOGIN`/`AuthGuard`/
`players[]` - מאומת דרך JSON גולמי על WebSocket (ראו הבדיקה הידנית ב-`CLAUDE.md`), לא דרך ה-HUD
האמיתי. סגירת הפער הזה היא בדיוק מה ש-Iteration 3.5 (למטה) קיימת בשבילו.

**בדיקות:** `LoginUseCase` - username ריק (או חסר) נדחה עם `MALFORMED_PAYLOAD`; login חוזר על
אותו connectionId מחליף את השם הקודם. `AuthGuard` - `LOGIN`/`PING` עוברים גם בלי login; `type`
אחר בלי login מחזיר `AUTH_REQUIRED` ישירות דרך `ITransport` ולא מגיע ל-bus האמיתי; `type` אחר
עם login מועבר הלאה; `subscribe()` מועבר לבוס העטוף ללא שינוי.

---

## Server-Iteration 3.5 — לקוח מחובר לרשת (Networked GUI Client)

**נוספה בביקורת תכנון מלאה שנעשתה אחרי Iteration 3** (לא הייתה בתוכנית המקורית - נוספה בדיעבד
כדי לשקף פער אמיתי: אף איטרציה מ-1 עד 9 לא הייתה מתזמנת אי-פעם את בניית `ServerConnection` או
שינוי `main_gui.cpp`, למרות ש-Iteration 3 (ותכן גם 4/5/8) מניחות בשקט שהם כבר קיימים).

**מטרה:** `chess_gui` (ה-GUI האמיתי, לא קליינט-בדיקה של JSON גולמי) משחק בפועל מול `chess_server`
על הרשת, במקום מול `GameEngine` לוקאלי.

**מוקד/היקף גס (התכנון המפורט נדחה בכוונה לשיחת תכנון ייעודית, באותו אופן שבו Iteration 3
תוכננה):** `client_net/ServerConnection` - מממשת את מה ש-`Controller` הקיים כבר מצפה לו
(`requestMove`/`requestJump`), אבל שולחת JSON על WebSocket במקום לקרוא ל-`GameEngine` ישירות.
שינוי ב-`main_gui.cpp` כדי להשתמש ב-`ServerConnection` (השורה בפריסת התיקיות שאומרת "ישונה
בהמשך" - זה ה"בהמשך"). קונסולה (`AllocConsole`+`std::cin`) לבקשת username ושליחת `LOGIN` - בדיוק
כפי שכבר מתואר (אך לא מומש) ב-Iteration 3. הרחבת ה-HUD/renderer לצייר את `players[]` (השרת כבר
שולח את זה, מאומת ב-Iteration 3 - זה רק צד הציור).

**חייב לקרות לפני Iteration 4:** Iteration 4's `ClientLogger` עוטפת `ServerConnection` - בלי
Iteration 3.5, Iteration 4 לא ניתנת למימוש כפי שהיא כתובה.

**מספור:** נשארת "3.5", לא renumber לכל האיטרציות 4-9 - שינוי מספור היה נוגע בכל הפניה צולבת
בקובץ (טבלת הפרוטוקול, יומן ה-CR ב-`CLAUDE.md` וכו') בלי תועלת פונקציונלית.

---

## Server-Iteration 4 — לוגים דו-צדדיים דרך ה-Bus

**מטרה:** כל הודעה שנשלחת/מתקבלת נרשמת לקובץ, גם בשרת גם בקליינט, באותו פורמט.

**מוקד:** `Logger` הוא **subscriber** על ה-`IEventBus` בשרת (לא קריאה ישירה מכל use-case).
בקליינט - `ClientLogger` עוטף את `ServerConnection` (התלות הזאת מסופקת ע"י Iteration 3.5, שקודמת
לאיטרציה זו בדיוק בשביל זה).

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

**נמצא בביקורת:** "ספירה לאחור **על המסך**" ברישא למעלה היא רינדור client-side - מוצג דרך הרחבת
ה-HUD שנבנתה ב-Iteration 3.5, לא מחלקה חדשה. התכנון המדויק (איפה בדיוק ב-HUD, איך מתעדכן כל
שנייה) נדחה לשיחת התכנון של איטרציה זו עצמה, לא מוכרע כאן.

**נסגר בפועל (client half A, שיחת תכנון נפרדת אחרי שהשרת כבר היה ירוק):** נבנה `Hud::‏
drawDisconnectCountdown`‏ (‏`src/view/hud/DisconnectCountdownHud.hpp`‏/‏`.cpp`‏, אותו pattern כמו
`PlayerNamesHud`‏), ו-`ServerConnection`‏ מטפל עכשיו ב-`DISCONNECT_COUNTDOWN`‏ בפועל (במקום
להתעלם ממנו בשקט) ושומר את ה-`sessionToken`‏ מ-`LOGIN_OK`‏. **פער שנמצא ותוקן באותה שיחה:**
`ReconnectUseCase`‏ שולחת ליריב הודעת `DISCONNECT_COUNTDOWN`‏ נוספת עם `secondsLeft:0`‏ כש-
reconnect מצליח - בלעדיה, ה-HUD אצל היריב היה נשאר תקוע על הערך האחרון לנצח (‏`STATE_UPDATE`‏
הרגיל לא נושא סימון "הבעיה נפתרה"). בצד הלקוח, ה-countdown גם מתנקה אוטומטית בכל `STATE_UPDATE`‏
עם `gameOver:true`‏ - זה מכסה גם את נתיב ה-timeout (resign אוטומטי), בלי לגעת ב-`DisconnectUseCase`‏
בכלל.

**נסגר בפועל (client half B, שיחת תכנון + מספר סבבי CR נפרדים):** הצד של **השחקן שהתנתק עצמו**
נבנה. `IServerLink`‏ קיבל `setOnClose`‏; `WebSocketClientLink`‏ עובד ב-perpetual mode (מאומת
בניסוי מבודד לפני המימוש) ומאפשר reconnect על אותו אובייקט (‏`ioThread_`‏ מוקם פעם אחת ב-
constructor, לא ב-`connect()`‏, אחרת קריסה מובטחת). `ServerConnection`‏ קיבלה מתודה סימטרית
ל-`login()`‏ - `reconnect()‏`‏ (ניסיון בודד, מתואם לפי `requestId`‏, מחזירה `Success`‏/`Retry`‏
בלבד - **בלי** `GiveUp`‏ מיידי על `SESSION_EXPIRED`‏, כדי לא ליפול קורבן לרייס מול זיהוי הניתוק
בצד השרת). מחלקה חדשה, `AutoReconnector`‏, מחזיקה את מדיניות ה-retry (thread רקע קבוע אחד,
לא אחד לכל ניתוק) - בדיוק אותה חלוקה כמו `DisconnectUseCase`‏/`ReconnectUseCase`‏ בצד השרת.
`kReconnectWindowMs`‏ עבר ל-`shared/protocol/config.hpp`‏ - מקור אמת אחד לשני הצדדים.

**שרשרת תיקוני אינטגרציה שנמצאו בסבבי CR נפרדים, כולם נסגרו לפני המימוש:** הוספת timeout
ל-`connect()`‏ (למניעת תקיעה נצחית) חשפה use-after-free (משתני סנכרון מקומיים שנתפסו
ב-reference, בעוד ה-handler עדיין רשום אחרי חזרה מוקדמת) - תוקן עם `shared_ptr`‏ הנתפס
by value; זה בתורו חשף דליפת "חיבור זומבי" בזמן timeout - תוקן עם `con->terminate(ec)`‏.
גם `ClientLogger`‏ (ה-`IServerLink`‏ שבאמת בשימוש ב-production) ו-`requestIdCounter_`‏
(‏`std::atomic`‏ עכשיו, נקרא משני threads) נמצאו כפערים שהתוכנית המקורית פספסה.

**בדיקות:** עם fake clock - מתקדמים 19s + `RECONNECT` => מתבטל; מתקדמים 20s בלי `RECONNECT` =>
resign.

**הוחלט בתכנון (לא נעשה שינוי בקוד עבור זה):** `FileLogger`‏'s `TimestampProvider`‏ (איטרציה 4)
**לא** מוחלף ב-`IClock`‏ - שתי שאלות שונות לגמרי (`IClock`‏ עונה "כמה זמן חלף", מונוטוני,
ניתן-לקידום בבדיקות; `TimestampProvider`‏ עונה "מה השעה/תאריך האמיתיים עכשיו" לצורך שורת לוג
קריאה, לעולם לא מזויף בבדיקות). סוגר את השאלה הפתוחה שהושארה באיטרציה 4.

**נמצא בביקורת (תוקן לפני המימוש):** `DisconnectUseCase::tick`‏ קורא ל-`GameEngine::resign`‏
**דרך `GameSession::resign`‏**, לא ישירות - `GameSession`‏ קיימת רק בשביל ה-`mutex`‏ שלה, שמגן על
כל גישה ל-`GameEngine`‏ מפני מרוץ בין ה-tick thread לבין `MakeMoveUseCase`‏ (הרצים על threads
שונים); קריאה ישירה ל-`GameEngine::resign`‏ הייתה עוקפת את ההגנה הזו בדיוק.

**הערת פרוטוקול (לא באג):** לקוח ששולח `RECONNECT`‏ על socket חדש עשוי לקבל `TABLE_FULL`‏ על אותו
socket (מ-`onConnected`‏, שרץ אוטומטית בפתיחת socket, לפני שההודעה הראשונה בכלל נשלחת) רגע לפני
שה-`RECONNECT`‏ עצמו מצליח בפועל (עוקף את `onConnected`‏/`TABLE_FULL`‏ לגמרי דרך
`ConnectionManager::bindKnown`‏). אין `close()`‏ בענף הדחייה אז זה לא שובר כלום - אבל לקוח ששולח
`RECONNECT`‏ על socket טרי **חייב להתעלם** מ-`TABLE_FULL`‏ שמגיע על אותו socket ולחכות לתשובת
ה-`RECONNECT`‏ בפועל.

---

## Server-Iteration 6 — SQLite: יוזר+סיסמה+Elo

**מטרה:** התחברות אמיתית מול DB; דירוג מתחיל מ-**1200** ונע לפי Elo סטנדרטי.

**מוקד:** `IUserRepository` כ-port; `EloService` היא **פונקציה טהורה** ללא תלות ב-DB כלל.

**ספריית הצפנה כבר נבחרה ונכנסה לפרויקט באיטרציה 5:** `libsodium`‏ - הובאה שם עבור
`ITokenGenerator`‏/`SodiumTokenGenerator`‏ (‏`randombytes_buf`‏, ל-`sessionToken`‏), בכוונה כדי
שאיטרציה זו תשתמש **באותה** ספרייה לגיבוב סיסמאות (‏`crypto_pwhash_str`‏/‏`crypto_pwhash_str_verify`‏
- Argon2id) במקום לבחור ספרייה שנייה נפרדת. נבחרה חוצת-פלטפורמות בכוונה (על פני Windows CNG) בגלל
יעד הפריסה העתידי ב-Docker (כנראה Linux).

**מחלקות/קבצים:** `IUserRepository`, `SqliteUserRepository`, `EloService`. סכמת DB:
`users(id, username UNIQUE, password_hash, elo DEFAULT 1200, created_at)`.

**התנהגות נדרשת:** הרשמה אם המשתמש לא קיים, אימות סיסמה אם קיים; דירוג מתעדכן **רק** בסיום
משחק שמקורו ב-Play (Room הוחלט כ-unranked - ראו איטרציה 8).

**שאלות פתוחות שנמצאו בביקורת (להכריע כשמתחילים לתכנן איטרציה זו, לא עכשיו):**
- **`IIdentityStore` מול `IUserRepository`:** Iteration 3 בנתה `IIdentityStore`/`InMemoryIdentityStore`
  (connectionId->username בזיכרון, בלי סיסמה). איטרציה זו מוסיפה `IUserRepository`/DB אמיתי ומשנה
  את payload ה-`LOGIN` (מוסיפה `password`). לא הוכרע: `IIdentityStore` מוחלף, נשאר כ-cache
  ברמת-חיבור מעל `IUserRepository`, או שניהם ממשיכים לחיות זה לצד זה? כך או כך `LoginUseCase`
  (מ-Iteration 3) יזדקק לעריכה - זה לא רשום כקובץ שאיטרציה זו נוגעת בו, וצריך להיות.
- **מי קורא ל-`EloService::apply(...)` ומתי:** לא מצוין באף איטרציה (6, 7, או 9) איזה use-case
  בדיוק מזהה "סיום משחק שמקורו ב-Play" ומפעיל את חישוב ה-Elo, ואיך `GameSession` בכלל "יודע"
  שהוא Play-origin ולא Room-origin (`GameSession` כפי שנבנה ב-Iteration 2 אין לו שדה כזה).

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

**שאלות פתוחות שנמצאו בביקורת (להכריע כשמתחילים לתכנן איטרציה זו, לא עכשיו):**
- **`TABLE_FULL` מול matchmaking:** Iteration 2 דוחה חיבור שלישי עם `TABLE_FULL` במפורש "כי אין
  עדיין Play/Room" - אבל עכשיו שיש, לא מצוין מה קורה לחיבור חדש: הוא נכנס לתור matchmaking
  במקום להידחות? צריך להחליט ולתעד את השינוי בהתנהגות הקבלה של `ConnectionManager`.
- **תור ה-tick היחיד:** ה-thread ב-`main_server.cpp` שקורא `session.wait(deltaMs)` היום מכיר
  `GameSession` **אחד קשיח** (משתנה `main()`-local). ברגע ש-`MATCH_FOUND` יוצר `GameSession`ים
  נוספים בו-זמנית, הלולאה הזאת חייבת להכליל את כולם (ראו את הסיכון המתועד כבר ב-`CLAUDE.md`,
  "Deferred, deliberately" ב-Iteration 2) - צריך תכנון מפורש כאן, לא נשאר סתום.

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

**נמצא בביקורת:** "מוצג בראש המסך" הוא רינדור client-side, דרך הרחבת ה-HUD מ-Iteration 3.5
(אותה תשתית ציור, לא מחלקה חדשה) - `WindowsInputDialog` למעלה הוא ל**קלט** (Create/Join), לא
לפלט הזה.

**בדיקות:** `RoomUseCase` - הצטרפות ראשונה/שנייה/שלישית => תפקידים נכונים; `JOIN` ל-id לא קיים
=> `ROOM_NOT_FOUND`. `WindowsInputDialog` - לא ניתן ל-unit test אמיתי (כמו UI-Iteration A שלך);
smoke test ידני בלבד.

---

## Server-Iteration 9 — סאונד + אנימציות start/end דרך ה-Bus

**מטרה:** להשלים את שקף ה-BUS - עוד subscribers חדשים, בלי לגעת בלוגיקת ה-subscriber-ים הקיימים.

**מוקד:** `SoundSubscriber`, `GameLifecycleAnimationSubscriber` - שניהם subscribers חדשים על
ה-`IEventBus` הקיים מאיטרציה 1. השרת רק משדר את האירוע (`GameStarted`/`GameOver`) כ-DTO;
ניגון הסאונד/האנימציה עצמם הם עניין של הקליינט.

**תוקן בביקורת:** "בלי לגעת בשום use-case קיים" (כפי שהיה כתוב במקור) לא מדויק - `GameStarted`/
`GameOver` לא מתפרסמים היום ע"י שום use-case קיים (`MakeMoveUseCase` מפרסם רק `MoveApplied`), אז
מישהו חייב לקבל `bus_.publish(...)` חדש כדי שהאירועים האלה יתחילו להתקיים בכלל - כנראה `GameSession`
(סיום) ו/או המקום שיוצר `GameSession` חדש (התחלה, Iteration 2 או 7). מה שבאמת חדש ב-Iteration 9
הוא ה**subscriber** בלבד - צד ה-publish דורש עריכה קטנה במקום קיים, לא "בלי לגעת" לגמרי.

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
