# תוכנית עבודה ל-UI — Kung Fu Chess

בנויה באותה שיטה כמו קובץ הדרישות של המרצה ללוגיקה (Iteration Plan): כל שלב קטן, בעל מטרה אחת, נבנה על גבי מה שכבר עבד ונבדק בשלב הקודם. אנחנו לא עוברים לשלב הבא לפני שהקודם רץ ועובד.

**מוסכמות:** כל שלב = "UI-Iteration". כל שלב מכיל: מטרה, מוקד (מה שמור להיות קטן/נקי), התנהגות נדרשת, ובדיקות. בהתאם למסמך המרצה — Renderer לעולם לא מקבל Board/Piece חיים, רק `GameSnapshot`; קלט תמיד עובר דרך `Controller.click`.

---

## מבנה תיקיות מוצע לפרויקט

```
src/
  model/            Board, Piece, Position, PieceState        (יש כבר)
  rules/            RuleEngine, PieceRules, Movement, GameOverRule  (יש כבר)
  realtime/         RealTimeArbiter, Moves.hpp (PieceMove/JumpMove) (יש כבר)
  engine/           GameEngine, GameSnapshot ⚑ (מחלקה חדשה)
  input/            Controller, BoardMapper                    (יש כבר)
  events/           IGameObserver, GameEvent ⚑ (חדש — תיקייה משותפת
                     ונייטרלית, כדי ש-engine ו-view לא יתלו זה בזה ישירות)
  view/
    canvas/         ICanvas, ImgCanvas  ⚑  <- הקובץ היחיד שמכיר Img/OpenCV
    render/         BoardRenderer, PieceAnimator
    hud/             ScoreBoard, MovesLog, PlayerNamesDisplay  (subscribers של IGameObserver)
    assets/         SpriteLoader, AssetCache
  app/              GameLoop/App, main_gui.cpp ⚑ (חדש)
  legacy/           BoardParser, ScriptRunner, main.cpp        (מצב הטקסט הקיים — לא נמחק!
                     הוא בעצם ה-integration test suite שלך)

assets/
  pieces1/ pieces2/ board.png            (מה שקיבלתם)

tests/
  unit/             טסטים ל-BoardMapper, PieceAnimator (מתמטיקת progress),
                     EventBus — בלי Img בכלל
  integration/       ScriptRunner הקיים (text-mode) — נשאר כרשת ביטחון
```

**שתי החלטות שכבר התקבלו וכדאי לזכור:**
1. `events/` היא תיקייה נפרדת, לא בתוך `engine/` ולא בתוך `view/` — כדי שהתלות תישאר חד-כיוונית (`view` תלוי ב-`engine`, לא להפך).
2. `legacy/` (ה-`main.cpp` הטקסטואלי + `ScriptRunner`) נשאר בחיים כ-target CMake נפרד (`chess_console`) לצד ה-UI החדש (`chess_gui`) — לא נמחק ולא נדרס.

---

## טבלת המחלקות של שכבת ה-UI (9 מחלקות)

| מחלקה | אחראית על | לא אחראית על |
|---|---|---|
| `SpriteLoader` | קריאת `config.json` + טעינת פריימים (Img) לכל state של כל כלי, caching | לוגיקת משחק, timing |
| `PieceAnimator` | state נוכחי, אינדקס פריים נוכחי, קידום פריימים לפי fps וזמן שחלף, מעבר ל-`next_state_when_finished` | ציור בפועל, מיקום בלוח |
| `BoardMapper` | pixel↔cell, וגם מיפוי חלון↔תמונה (resize/relative window) | לוגיקה, ציור |
| `BoardRenderer` | ציור הלוח + הכלים לפי הסנאפשוט, הדגשת בחירה | לוגיקה, קלט |
| `ScoreBoard` | חישוב/הצגת "עלות" כלים שנתפסו | קליטת אירועים ישירות מה-Board |
| `MovesLog` | רשימת מהלכים אחרונים | — |
| `EventBus/Observer` | GameEngine מפרסם אירועים (מהלך בוצע, כלי נתפס, המלך נתפס) — subscribers כמו ScoreBoard/MovesLog נרשמים | ציור עצמו |
| `InputAdapter` | mouse events → BoardMapper → Controller.click | לוגיקה |
| `GameLoop/App` | ה-loop שמחבר הכול: קלט → advance time (זמן אמיתי, לא ב-tests!) → snapshot → render | הכול אחר |

בנוסף — `ICanvas`/`ImgCanvas`: ממשק מופשט מעל ספריית `Img` (עוטפת OpenCV), כדי שביום שתרצי להחליף לספרייה גרפית אחרת ישתנה רק adapter אחד. כל שאר המחלקות למעלה מכירות רק את `ICanvas`, אף פעם לא `Img`/`cv::Mat` ישירות.

**עיקרון תזמון חשוב** (למה יש Observer): `Controller.click → GameEngine` הוא נתיב חם וסינכרוני — זול מטבעו, בלי IO/פורמוט טקסט, ולכן מיידי. `GameEngine` מפרסם רק event גולמי (`id`, `from`, `to`, `capturedKind`) — לא קורא ישירות ל-`ScoreBoard`/`MovesLog`. כל העיצוב/הסכימה הכבדים קורים בתוך ה-subscribers, במסלול נפרד, ומותר להם לאחר בכמה פריימים.

---

## UI-Iteration A — שלד ציור ריק (בלי engine בכלל)

**מטרה:** להוכיח שצינור Img/OpenCV עובד, לפני שנוגעים בלוגיקה בכלל.

**מוקד:** `ICanvas`/`ImgCanvas` הן היחידות שמכירות `Img`/OpenCV. שום מחלקה אחרת לא נוצרת עדיין.

**התנהגות נדרשת:**
- חלון נפתח ומציג לוח 8×8 בצבעים מתחלפים (רקע בלבד, בלי כלים).
- החלון נסגר בלחיצת מקש/X, בלי קריסה.
- `ImgCanvas` עוקפת את `Img::show()` (שחוסם עם `waitKey(0)`) ומנהלת בעצמה `cv::imshow` + `cv::waitKey(1)` בלולאה.

**בדיקות:**
- אין unit test אמיתי כאן (זה כל הפוינט — שכבה זו כמעט ולא ניתנת לבדיקה אוטומטית). יש "smoke test" ידני: מריצים, מוודאים שנפתח ונסגר בלי קריסה.
- מה שכן ניתן ל-unit test: פונקציית חישוב גודל הלוח בפיקסלים (rows×cellSize) — פונקציה טהורה, בלי Img.

---

## UI-Iteration B — כלים סטטיים מ-snapshot מזויף

**מטרה:** לצייר כלים על הלוח, בלי engine עדיין — עם `GameSnapshot` מפוברק ידנית בקוד הבדיקה.

**מוקד:** `SpriteLoader` טוען פריים idle יחיד לכל סוג/צבע כלי. `BoardRenderer` מקבל רק רשימת (kind, color, cell) — לא יודע כלום על Board האמיתי.

**התנהגות נדרשת:**
- כל 32 הכלים בעמדת הפתיחה מוצגים במקום הנכון על הלוח.
- שינוי ידני ב-snapshot המפוברק (למשל הזזת מלכה) גורם לציור להתעדכן בהרצה הבאה.

**בדיקות:**
- `SpriteLoader` מחזיר Img לא-ריק לכל תיקיית state קיימת (idle לפחות).
- מיפוי cell→pixel (משתמש ב-`BoardMapper` הקיים) מוחזר נכון לכל 64 התאים — unit test טהור, בלי Img.

---

## UI-Iteration C — קלט: קליק עד קונסול (עדיין בלי engine)

**מטרה:** לוודא שקואורדינטות הקליק נכונות, לפני שמחברים ללוגיקה.

**מוקד:** `InputAdapter` ממיר pixel→cell דרך `BoardMapper::pixelToCell` הקיים שלך, ומדפיס לקונסול בלבד. אסור לו לקרוא ל-`GameEngine`.

**התנהגות נדרשת:**
- קליק בתוך הלוח מדפיס (row, col) נכונים.
- קליק מחוץ ללוח לא גורם לקריסה ולא מדפיס תא.

**בדיקות:**
- כבר מכוסה ע"י unit tests הקיימים של `BoardMapper` (יש לך אותם מהלוגיקה) — כאן רק מוודאים חיווט העכבר בפועל (ידני).

---

## UI-Iteration D — "UI מינימלי משחק" (זהה ל-Iteration 9 של המרצה)

**מטרה:** ה-UI המינימלי המשחק־תקין, בדיוק כפי שהוגדר במסמך הלוגיקה שלך.

**מוקד:**
- מוסיפים ל-`GameEngine` מתודת `snapshot() const` חדשה שמחזירה מבנה קריאה-בלבד (לא `Board&`).
- `Controller.click` הוא נתיב הקלט היחיד — אין נתיב קלט שני בתוך ה-UI.
- אין חוקי משחק חדשים ברנדרר.

**התנהגות נדרשת** (מהמסמך שלך, מילה במילה):
- ציור רשת הלוח.
- ציור כלים מתוך `GameSnapshot`.
- ציור כלים בתנועה בין תאים לפי מיקומי פיקסלים מה-snapshot (עדיין קפיצה ישירה למיקום, לא אנימציה — זה השלב הבא).
- הדגשת כלי נבחר אם ה-snapshot כולל selection.
- הצגת הודעת game-over כש-`game_over` הוא true.

**בדיקות:**
- ה-snapshot מכיל מספיק נתונים לציור בלי לחשוף Board/Piece בני-שינוי.
- smoke test לרנדרר: מצייר לוח פשוט בלי לשנות state.
- קליק ב-UI מנותב ל-`Controller.click`, לא ל-Board או RuleEngine ישירות.

---

## UI-Iteration E — אנימציה אמיתית (glide, לא קפיצה)

**מטרה:** כלי שזז נראה זז, לא קופץ.

**מוקד:** חושפים מ-`RealTimeArbiter` נתוני תנועה פעילה לקריאה-בלבד (מבוסס על `PieceMove::startMs/durationMs` הקיימים אצלך) — בלי לחשוף את ה-`vector` הפנימי עצמו. `PieceAnimator` מחשב `progress = (now-startMs)/durationMs` ופריים נוכחי לפי `frames_per_sec` מ-`config.json`.

**החלטה שצריך לקבל כאן (דנו בזה):** האם `Resting`/`Jumping` נכנסים ל-`PieceState` במודל (כי `restMs` הוא כלל משחק אמיתי) או נשארים סטייט ויזואלי בלבד ב-`PieceAnimator`.

**התנהגות נדרשת:**
- כלי בתנועה מצויר במיקום פיקסלים מדורג בין מקור ליעד, לא קופץ ישר ליעד.
- מעבר בימעבר בין states נקבע ע"י RealTimeArbiter לפי config::statsFor (Engine-authoritative) - לא לפי next_state_when_finished שב-config.json. ה-json נשאר אחראי רק על frames_per_sec/is_loop (קוסמטי בלבד).

**בדיקות:**
- חישוב progress ומיקום פיקסלים ביניים — unit tests טהורים, בלי Img בכלל.
- מיפוי (state, זמן שחלף) → אינדקס פריים — unit test טהור.

---

## UI-Iteration F — HUD דרך Observer: Score / Moves / Names / Game-over

**מטרה:** בדיוק מה שדיברנו עליו — לחיצה נשארת מיידית, ה-HUD מתעדכן בנפרד.

**מוקד:**
- מגדירים `GameEvent` (raw data בלבד: piece id/kind/color, from, to, captured kind אם יש) ו-`IGameObserver` בתיקיית `events/` נייטרלית.
- `GameEngine`/`RealTimeArbiter` מפרסמים event, לא קוראים ל-`ScoreBoard`/`MovesLog` ישירות.
- `ScoreBoard`, `MovesLog`, `PlayerNamesDisplay` הן subscribers — כל העיצוב/הסכימה קורה בתוכן, לא בנתיב הקליק.

**התנהגות נדרשת:**
- ניקוד = סכום "עלות" כלים שנתפסו לכל צד.
- לוג מהלכים אחרונים מוצג בצד הלוח.
- שמות שחקנים מוצגים בקבוע.
- לחיצה על מהלך לא "מרגישה" איטית יותר גם כשה-HUD כבד.

**בדיקות:**
- `EventBus`: publish/subscribe עם subscriber מזויף שסופר קריאות — בלי Img.
- חישוב סכום ניקוד — unit test טהור עם טבלת ערכי כלים.

---

## UI-Iteration G — ליטוש: חלון דינמי, בדיקה עצמית, ניקוי סופי

**מטרה:** לסגור את הפערים מהמצגת (slide "Game Controls") ולנקות מבנה.

**מוקד:** יחס screen pixels↔image pixels כש-resize את החלון (נפרד מ-`BoardMapper` הלוגי שממפה cell↔pixel פנימי-קבוע); שיטת בדיקה עצמית לרנדרר (למשל snapshot מזויף קבוע + השוואת תמונת פלט, או צילומי מסך ייחוס).

**התנהגות נדרשת:**
- שינוי גודל חלון לא שובר את יחסי הלוח/כלים.
- יש דרך מתועדת לוודא חזותית שהרנדרר לא נשבר אחרי שינוי (checklist/smoke script).
- `legacy/` (main.cpp הטקסטואלי הקיים) עדיין רץ כ-target נפרד ב-CMake — לא נמחק.

---

## סדר העבודה

עוברים על השלבים A→G **בסדר הזה בלבד**, כל אחד עד שהוא "ירוק" (רץ + נבדק) לפני המעבר לבא. בכל שיחה הבאה נצלול לשלב אחד ספציפי: מגדירים יחד את המחלקות/הקבצים המדויקים, כותבים יחד את הבדיקות לפני המימוש, ואז המימוש הקטן ביותר שמעביר אותן.
