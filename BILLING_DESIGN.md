# Fitboddy: дизайн биллинга

Версия: 1.3  
Дата: 2026-08-15  
Источники: `BUSINESS_PLAN.md`, `TECH_SPEC.md`, `TASKS_BACKLOG.md`, `NOTIFICATIONS_DESIGN.md`, `PAYMENTS_LEGAL.md`.  
Контекст: модель `trainer-set pricing + client subscription + platform commission`.

Документ фиксирует целевую архитектуру и контракт P0. Открытые продуктовые развилки вынесены в конец — их нужно подтвердить до реализации.

---

## 1. Цели

- Тренер бесплатно входит на платформу и сам задает цену подписки.
- Платформа сама продаёт свои тренировки (system-plan), ориентир `500 ₽`/мес.
- Клиент платит либо за тариф платформы, либо за доступ к конкретному тренеру (можно оба, разными платежами).
- С тарифа тренера платформа берёт комиссию (`take_rate`). Свой тариф — целиком наша выручка.
- Деньги и статусы сходятся: webhook идемпотентен, ledger сходится с провайдером, доступ включается только при `active`.

Не цели P0:

- автоматические выплаты тренерам (это P1);
- tiered take-rate, промо, add-ons;
- multi-provider orchestration;
- сложные налоги / международный VAT.

---

## 2. Модель монетизации

Два `seller_type`. В одном платеже не смешивать.

Тариф тренера (`seller_type = trainer`):

```
клиент платит price_minor
        │
        ▼
   gross_minor (100%)
        │
        ├── platform_fee_minor     = round(gross_minor * take_rate)
        ├── processor_fee_minor    = комиссия эквайера (на тренере)
        └── trainer_payout_minor   = gross_minor - platform_fee_minor - processor_fee_minor
```

Свой тариф (`seller_type = platform`), ориентир `50000` копеек:

```
клиент платит price_minor
        │
        ▼
   gross_minor (100%)
        │
        ├── processor_fee_minor    = комиссия эквайера (на платформе)
        ├── platform_fee_minor     = gross_minor - processor_fee_minor
        └── trainer_payout_minor   = 0
```

Инвариант каждой ledger-записи:

```
gross_minor = platform_fee_minor + processor_fee_minor + trainer_payout_minor
```

Это совпадает с формулой бизнес-плана:

```
Payout_trainer = GMV * (1 - TakeRate) - processing_fees
Revenue_platform = GMV * TakeRate
```

То есть комиссию эквайера в P0 несет тренер, не платформа.

Пилотные цифры (конфиг, не хардкод):

| Параметр | Пилот | Комментарий |
|---|---|---|
| `currency` | `RUB` | единственная валюта P0 |
| `period` | `month` | единственный период P0 |
| `take_rate` | `0.15` | вилка гипотез 10–25% |
| `platform_price_minor` | `50000` (`500 ₽`) | свой тариф Fitboddy |
| `min_price_minor` | `50000` (`500 ₽`) | нижняя граница оффера тренера |
| `max_price_minor` | `5000000` (`50 000 ₽`) | верхняя граница оффера тренера |
| средний чек тренера (гипотеза) | `2500 ₽` | только для KPI, не код |

Деньги везде одно представление: **целые копейки (`bigint` / `int`)**. БД, API, ledger, вызов провайдера, тесты — один тип. Никаких `float`, `Decimal` и рублей в JSON. Рубли только в UI: человек вводит/видит `2500`, фронт шлёт `250000`. Округление комиссии — `ROUND_HALF_UP` до целой копейки.

---

## 3. Границы релиза

### P0 — обязательно до pilot

- Оффер тренера: create/update, одна активная цена.
- Checkout клиента на выбранного тренера.
- Lifecycle подписки: `pending`, `active`, `past_due`, `canceled`, `paused`.
- Гейтинг платного контура по `active`.
- Расчет комиссии и ledger на успешный платеж.
- Идемпотентный webhook провайдера.
- Минимальный отчет тренера: списания и удержания (начисления, не выплаты).
- События для нотификаций: `payment_succeeded`, `payment_failed`, `subscription_activated`, `subscription_canceled`.

### P1 — после pilot

- Payout batches (ежедневно/еженедельно), статусы, retry.
- Reconciliation с провайдером и алерты на расхождения.
- Отчет по фактическим выплатам.
- Pause/cancel в UI (статусы в API уже есть).
- Смена цены для уже активных подписчиков.

### Вне v1.1

- Несколько провайдеров одновременно.
- Trial / промокоды / referral.
- Несколько одновременных офферов у тренера (годовой тариф и т.д.).
- Возвраты и dispute-автоматизация сверх ручного admin-flow.

---

## 4. Архитектура

Новый сервис `fitboddy-billing-service` — по тому же шаблону, что auth/tenant/profile/plan.

Почему не модуль внутри tenant:

- отдельный денежный контур (ledger, webhook, идемпотентность);
- другой SLA и другой секретный периметр;
- tenant остаётся про связи, billing — про деньги и доступ по подписке.

```
admin-frontend
      │  JWT
      ▼
   gateway :8080
      │
      ├── /api/v1/billing/*          → billing-service
      └── /api/v1/billing/webhooks/* → billing-service  (без JWT, подпись провайдера)
                                      ▲
plan-service  ── X-Service-Token ─────┤  GET subscription access
tenant-service ─ X-Service-Token ─────┘
                                      │
                                      ▼
                              payment provider
```

Владение:

| Сервис | Что знает |
|---|---|
| `tenant-service` | связь trainer-client (`invited/active/...`) |
| `billing-service` | оффер, подписка, платеж, ledger, доступ по деньгам |
| `plan-service` | спрашивает billing: можно ли генерировать/открывать план |
| `auth-service` | кто пользователь |
| `admin-frontend` | экраны цены, checkout, статус, отчет тренера |

Правило: **связь ≠ подписка**. Можно быть в `active` relation без оплаты (инвайт принят, но checkout не пройден). Платный контур открывается только при `subscription.status = active`.

Рекомендация по checkout: клиент может оплатить только тренера, с которым уже есть `active` relation. Иначе биллинг начинает создавать связи — это смешение контуров. Онбординг: invite/direct → accept → цена → оплата → план.

---

## 5. Сущности

Деньги везде `bigint` копейки + `currency char(3)`. Поля денежных сумм — суффикс `_minor`.  
У платежа и подписки есть `seller_type`: `platform` | `trainer`.

### 5.0 `platform_subscription_offer`

Один активный оффер платформы (system-plan). Цена из конфига `PLATFORM_PRICE_MINOR`, в пилоте `50000`. Меняет platform_admin, не тренер.

### 5.1 `trainer_subscription_offers`

Публичное предложение тренера. В P0 — не более одного `is_active = true` на тренера.

| Поле | Тип | Смысл |
|---|---|---|
| `offer_id` | uuid | PK |
| `trainer_user_id` | str | владелец |
| `price_minor` | bigint | цена за период, копейки |
| `currency` | `RUB` | |
| `period` | `month` | |
| `is_active` | bool | виден клиентам и доступен для checkout |
| `created_at`, `updated_at` | ts | |

Инварианты:

- `min_price_minor <= price_minor <= max_price_minor`;
- unique partial index: один активный оффер на `trainer_user_id`;
- смена цены создает новую версию или обновляет текущий оффер. Для P0 достаточно update того же `offer_id`. Уже активные подписки **не пересчитываются** до P1 (клиент сидит на `price_minor`, зафиксированном в подписке).

### 5.2 `trainer_client_subscriptions`

| Поле | Тип | Смысл |
|---|---|---|
| `subscription_id` | uuid | PK |
| `seller_type` | `platform` / `trainer` | чей тариф |
| `trainer_user_id` | str? | пусто, если `platform` |
| `client_user_id` | str | |
| `offer_id` | uuid | оффер на момент оформления |
| `price_minor` | bigint | зафиксированная цена, копейки |
| `currency` | `RUB` | |
| `period` | `month` | |
| `status` | enum | см. §6 |
| `provider` | str | `yookassa` / `stripe` / `sandbox` |
| `provider_subscription_id` | str? | внешний id, если провайдер ведет рекуррент |
| `current_period_start` | ts? | |
| `current_period_end` | ts? | |
| `canceled_at` | ts? | |
| `created_at`, `updated_at` | ts | |

Инварианты:

- не более одной `active` подписки `seller_type = platform` на клиента;
- не более одной `active` подписки на пару `(client_user_id, trainer_user_id)`;
- свой тариф и тариф тренера могут быть `active` одновременно — это разные продукты;
- в P0 клиент не может иметь две `active` подписки на разных тренеров (как и две active relation). Checkout второй пары — `409`.

### 5.3 `payments`

| Поле | Тип | Смысл |
|---|---|---|
| `payment_id` | uuid | PK |
| `subscription_id` | uuid | |
| `idempotency_key` | str | ключ checkout-запроса |
| `provider` | str | |
| `provider_payment_id` | str? | внешний id |
| `status` | `created` / `pending` / `succeeded` / `failed` / `canceled` / `refunded` | |
| `gross_minor` | bigint | сумма платежа, копейки |
| `currency` | `RUB` | |
| `failure_code` | str? | |
| `created_at`, `updated_at` | ts | |

Unique: `provider + provider_payment_id` (когда id уже известен).  
Unique: `idempotency_key`.

### 5.4 `billing_ledger_entries`

Пишется **только** на `payment.status → succeeded`, один раз.

| Поле | Тип | Смысл |
|---|---|---|
| `entry_id` | uuid | PK |
| `payment_id` | uuid | unique |
| `subscription_id` | uuid | |
| `trainer_user_id` | str | |
| `client_user_id` | str | |
| `gross_minor` | bigint | |
| `platform_fee_minor` | bigint | |
| `processor_fee_minor` | bigint | |
| `trainer_payout_minor` | bigint | |
| `net_minor` | bigint | = `platform_fee_minor` |
| `take_rate` | numeric | ставка на момент платежа |
| `currency` | `RUB` | |
| `created_at` | ts | |

`net_minor` — доход платформы с этого платежа, не «чистыми тренеру».

### 5.5 `payouts` (таблица с P0, процесс с P1)

Строка-начисление может жить без выплаты. В P0 таблица либо пустая, либо `status = accrued`. Батчи, ретраи и сверка — P1.

### 5.6 `provider_webhook_events`

| Поле | Тип | Смысл |
|---|---|---|
| `event_id` | uuid | внутренний PK |
| `provider` | str | |
| `provider_event_id` | str | unique вместе с provider |
| `event_type` | str | |
| `payload_hash` | str | |
| `processed_at` | ts? | |
| `created_at` | ts | |

Повтор webhook с тем же `provider_event_id` → `200` без повторного ledger/активации.

---

## 6. Lifecycle подписки

```
                  checkout
                     │
                     ▼
                 pending ────── payment_failed ──► остается pending
                     │                              (повторный checkout)
              payment_succeeded
                     │
                     ▼
                  active ◄── renewal succeeded
                     │
                     ├── renewal failed ──► past_due ── renewal ok ──► active
                     │                         │
                     │                         └── timeout / cancel ──► canceled
                     ├── trainer/client cancel ──► canceled
                     └── (P1) pause ──► paused ── resume ──► active
```

Правила P0:

- `pending` — деньги не прошли, доступа нет. Можно создать новый checkout, если нет другого `pending`/`active` на эту пару.
- `active` — доступ к платному контуру открыт до `current_period_end`.
- `past_due` — доступ закрыт. Провайдер ретраит списание; успех возвращает `active`.
- `canceled` — доступа нет, повторный checkout создает новую подписку (или реактивирует, если пара уникальна — на реализации: новая строка проще).
- `paused` — в enum с P0, переходы из UI в P1. Webhook провайдера может выставить статус, если провайдер так умеет.

Продление: если провайдер ведет рекуррент сам — мы только применяем webhook. Если нет — billing-service по `current_period_end` создает новый `payment` (воркер, P0 минимум: полагаться на рекуррент провайдера).

---

## 7. Пользовательские сценарии

### 7.1 Тренер публикует цену

1. Тренер открывает экран цены (`FRONT-P0-1`).
2. `PUT /api/v1/billing/trainers/me/offer` с `price_minor` (копейки), например `250000`.
3. Сервис валидирует диапазон и роль `trainer`.
4. Оффер становится `is_active = true`.
5. Без активного оффера checkout клиента невозможен.

### 7.2 Клиент оплачивает

1. Есть `active` relation с тренером.
2. Клиент видит цену оффера и статус своей подписки (`FRONT-P0-2`).
3. `POST /api/v1/billing/checkout` + `Idempotency-Key`.
4. Billing проверяет: JWT client, active relation (через tenant), активный оффер, нет чужой/своей конфликтующей active-подписки.
5. Создаются `subscription=pending` и `payment=created`.
6. Провайдер возвращает `confirmation_url`.
7. Клиент оплачивает. Webhook:
   - `succeeded` → payment succeeded, ledger, subscription active, событие `subscription_activated` + `payment_succeeded`;
   - `failed` → payment failed, подписка остается pending, событие `payment_failed`.

### 7.3 Доступ к плану

`plan-service` перед generate / active / today / complete:

```
GET /api/v1/billing/internal/access?client_user_id=&trainer_user_id=
X-Service-Token: ...
→ { "allowed": true }  только если subscription.status == active
```

Гейтинг P0:

- system-plan (без тренера) — только при `active` подписке `seller_type = platform`;
- план тренера — только при `active` подписке на этого тренера;
- свой тариф **не** открывает контур тренера; подписка на тренера **не** открывает system-plan. Это два продукта.

### 7.4 Отчет тренера (P0)

`GET /api/v1/billing/trainers/me/earnings?from=&to=`

Агрегат по ledger:

- число успешных платежей;
- `gross_minor`, `platform_fee_minor`, `processor_fee_minor`, `trainer_payout_minor`;
- список платежей (дата, клиент, суммы в копейках).

Это начисления, не «уже выплачено». Выплаты — P1.

---

## 8. Провайдер платежей

Адаптер, не конкретный SDK в use-case:

```
PaymentProvider
  create_payment(amount_minor, metadata, return_url, idempotency_key) -> { provider_payment_id, confirmation_url }
  parse_webhook(headers, raw_body) -> { provider_event_id, type, payment_id, status, processor_fee_minor? }
  verify_signature(...)
```

Реализации:

| Режим | Когда |
|---|---|
| `sandbox` | local/CI, сразу `succeeded` по тестовому webhook или auto-confirm |
| `yookassa` | пилот в РФ (гипотеза `₽`) |
| `stripe` | запасной контур, если пилот не РФ |

Выбор провайдера — открытая развилка (§15). Код не должен знать провайдера вне adapter + env `BILLING_PROVIDER`. В адаптер уходит уже `amount_minor`; отдельной конвертации рублей нет.

Webhook:

- отдельный location на gateway, без JWT;
- проверка подписи;
- идемпотентность по `provider + provider_event_id`;
- быстрый `200` после записи события; тяжёлую работу можно в той же транзакции P0 (объёмы пилота маленькие);
- NFR: success обработки >= 99.9%.

---

## 9. API (черновик контракта)

Префикс: `/api/v1/billing`. Все write — JWT + `Idempotency-Key` на checkout.

### Тренер

- `GET /trainers/me/offer` — текущий оффер или 404.
- `PUT /trainers/me/offer` — `{ price_minor, currency?, period? }` → оффер. `price_minor` — int, копейки, например `250000`.
- `GET /trainers/me/earnings` — отчет P0.
- `GET /trainers/{trainer_user_id}/offer` — публичная цена для клиента (JWT любого авторизованного).

### Клиент

- `POST /checkout` — `{ seller_type, trainer_user_id? }` → `{ subscription_id, payment_id, confirmation_url, status }`. Для `platform` поле тренера не нужно и игнорируется.
- `GET /subscriptions/me` — подписки клиента (в P0 обычно 0..2: своя и/или тренер).
- `GET /subscriptions/me/current` — своя + на активного тренера + статусы доступа.

### Internal (service token, не наружу)

- `GET /internal/access?client_user_id=&trainer_user_id=` — план тренера
- `GET /internal/access?client_user_id=&seller_type=platform` — system-plan
- `GET /internal/subscriptions/{subscription_id}` — для admin/debug.

### Webhook

- `POST /webhooks/{provider}` — сырое тело провайдера.

### Admin (platform_admin)

- `GET /admin/payments`, `GET /admin/subscriptions` — пагинация, фильтр по статусу.
- `POST /admin/subscriptions/{id}/force-cancel` — ручной инцидент.

Ошибки: как в остальных сервисах, плюс стабильные коды:

- `OFFER_NOT_FOUND`
- `PRICE_OUT_OF_RANGE`
- `RELATION_NOT_ACTIVE`
- `SUBSCRIPTION_CONFLICT`
- `CHECKOUT_IN_PROGRESS`
- `ACCESS_DENIED`

---

## 10. События наружу

Billing публикует (пока достаточно outbox-таблицы + поллинг, шина — P1):

| Событие | Когда | Кому в notify |
|---|---|---|
| `subscription_created` | checkout создал pending | внутреннее |
| `subscription_activated` | первый succeeded | тренер + клиент |
| `payment_succeeded` | каждый успешный платеж | клиент, опционально тренер |
| `payment_failed` | failed webhook | клиент |
| `subscription_canceled` | cancel / terminal past_due | тренер + клиент |
| `payout_processed` | P1 | тренер |

Контракт события — как в `NOTIFICATIONS_DESIGN.md`: `event_id`, `event_type`, `occurred_at`, `actor_user_id`, `recipient_user_id`, `context`, `dedupe_key`.

`dedupe_key` для платежа: `payment_succeeded:{payment_id}`.

---

## 11. Frontend

- `FRONT-P0-1`: форма цены в рублях, в API уходит `price_minor` (×100). Текущий оффер, ошибка диапазона. Это единственное место конвертации.
- `FRONT-P0-2`: карточка тренера → цена → кнопка оплаты → редирект провайдера → экран статуса (`pending/active/past_due/failed`).
- `FRONT-P0-3`: system-plan без `active` своей подписки не показать; план тренера — без `active` на этого тренера (UI-гейт + серверный гейт). Отдельная кнопка оплаты своего тарифа (~500 ₽).
- Отчет тренера: простая таблица начислений (можно на том же экране цены).

Return URL после оплаты: `/billing/return?subscription_id=...`. Страница сама перечитывает статус, не верит query `success=1`.

---

## 12. Инфраструктура и безопасность

- Отдельная БД `fitboddy_billing` + Alembic.
- Env: `BILLING_PROVIDER`, `TAKE_RATE`, `PLATFORM_PRICE_MINOR`, `MIN_PRICE_MINOR`, `MAX_PRICE_MINOR`, `YOOKASSA_*` / `STRIPE_*`, `INTERNAL_SERVICE_TOKEN`. Свой тариф — `PLATFORM_PRICE_MINOR=50000`. Вилка тренера — `50000`…`5000000`.
- Gateway: `/api/v1/billing` с JWT; `/api/v1/billing/webhooks` без JWT, с увеличенным timeout.
- Rate limit на `/checkout` (SEC-6).
- Аудит: смена оффера, checkout, каждый webhook, force-cancel.
- Секреты провайдера не в git.
- Smoke: create offer → sandbox checkout → webhook → access=true.

---

## 13. Тесты и KPI закрытия

Обязательные тесты (`TEST-P0-1..3`):

- идемпотентный checkout (один `Idempotency-Key` → один payment);
- повтор webhook → один ledger entry;
- `price_minor` вне диапазона → 422;
- checkout без relation → 409/403;
- комиссия: `100000` копеек, `take_rate=0.15`, `processor_fee_minor=0` → fee `15000`, payout `85000`;
- generate плана без active subscription → 403.

KPI:

- Payment success rate >= 95% на pilot.
- Ledger vs provider расхождение <= 0.1%.
- Webhook processing success >= 99.9%.
- Trainer activation: регистрация → цена → первый платный клиент.
- Client activation: инвайт → оплата → первая тренировка.

---

## 14. Порядок реализации

1. Сервис-скелет + миграции сущностей.
2. Offer API + валидация диапазона (`BILL-P0-1..3`).
3. Checkout + sandbox provider (`BILL-P0-4`).
4. Webhook + статусы подписки (`BILL-P0-5`, `BILL-P0-8`).
5. Ledger + take_rate (`BILL-P0-7`, `BILL-P0-9`).
6. Internal access + гейт в plan-service (`BILL-P0-6`).
7. Earnings API (`BILL-P0-10`).
8. Frontend экраны.
9. События для notify (контракт, даже если notify ещё заглушка).

---

## 15. Открытые решения (нужно подтвердить)

1. **Провайдер пилота**: YooKassa vs Stripe vs только sandbox до появления юрлица. Рекомендация контура (агент + сплит + самозанятый) — в `PAYMENTS_LEGAL.md`.
2. **Take rate и вилка цен**: 15% / 500–50 000 ₽ — ок как старт или другие цифры?
3. **Кто платит эквайринг**: в этом доке — тренер (как в бизнес-плане). Можно переложить на платформу — тогда формула ledger меняется.
4. **Checkout без relation**: запрещен (рекомендация) или checkout сам создает `direct` связь?
5. **Одна active-подписка на клиента глобально** (как одна active relation) или можно платить нескольким тренерам?
6. **Смена цены**: текущие подписчики остаются на старой цене до конца периода / до отмены — ок?
7. **System-plan**: свой платный тариф платформы, ориентир 500 ₽/мес. Закрыто: продаём, `seller_type = platform`.

Пока 1–3 и 5 блокируют точные цифры и уникальные индексы. Остальное можно начать с рекомендаций этого документа.
