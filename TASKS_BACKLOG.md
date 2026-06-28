# Fitboddy: список задач (отдельный backlog)

Источник: `BUSINESS_PLAN.md` + `TECH_SPEC.md`  
Модель монетизации: тренер бесплатный, цену подписки задает тренер, платформа берет комиссию с клиента.

---

## Как пользоваться

- Формат: чекбоксы для ручного трекинга прогресса.
- Приоритет: `P0` -> `P1` -> `P2`.
- Рекомендация: сначала закрыть все `P0`, затем переходить к `P1`.

---

## P0 — обязательно до pilot/prod

## 1) Security и корректность текущих сервисов

- [ ] `AUTH-P0-1` Добавить `UNIQUE` constraint на email в `auth-service` и миграцию.
- [ ] `AUTH-P0-2` Закрыть race-condition регистрации (тест на параллельную регистрацию).
- [ ] `TENANT-P0-1` Убрать impersonation через `acting_user_id`, actor брать из валидного токена.
- [ ] `TENANT-P0-2` Добавить server-side ACL на все write-операции связей trainer-client.
- [ ] `TENANT-P0-3` Перевести списки тренеров/клиентов на DB-pagination (без full in-memory scan).
- [ ] `PROFILE-P0-1` Закрыть internal endpoints service-auth механизмом (service token/mTLS).
- [ ] `PROFILE-P0-2` Защитить раздачу медиа (signed URL или контролируемый proxy policy).
- [ ] `PLAN-P0-1` Добавить role-aware доступ к планам/каталогу (только разрешенные участники).
- [ ] `PLAN-P0-2` Ввести инвариант одной активной записи плана на пользователя.

## 2) Billing core (новый обязательный контур)

- [ ] `BILL-P0-1` Добавить сущность оффера тренера: `price`, `currency`, `period`, `is_active`.
- [ ] `BILL-P0-2` Реализовать API установки цены тренером (create/update offer).
- [ ] `BILL-P0-3` Валидация диапазона цен платформы (`min_price`, `max_price`).
- [ ] `BILL-P0-4` Реализовать checkout клиента на выбранного тренера.
- [ ] `BILL-P0-5` Ввести lifecycle подписки: `pending`, `active`, `past_due`, `canceled`, `paused`.
- [ ] `BILL-P0-6` Ограничить доступ клиента к платному контуру только при `active` подписке.
- [ ] `BILL-P0-7` Ввести расчет комиссии платформы (`take_rate`) на успешный платеж.
- [ ] `BILL-P0-8` Реализовать idempotent обработку webhook платежного провайдера.
- [ ] `BILL-P0-9` Ввести финансовый ledger: `gross`, `platform_fee`, `processor_fee`, `trainer_payout`, `net`.
- [ ] `BILL-P0-10` Добавить минимальные отчеты для тренера по списаниям и удержаниям.

## 3) Notification core (обязательно в v1.1)

- [ ] `NOTIFY-P0-1` Добавить event-contracts для уведомлений: `payment_succeeded`, `payment_failed`, `subscription_activated`, `subscription_canceled`, `payout_processed`, `plan_generated`, `workout_ready`, `workout_completed`.
- [ ] `NOTIFY-P0-2` Реализовать хранилище in-app уведомлений (`notifications_in_app`) и API чтения/ack.
- [ ] `NOTIFY-P0-3` Интегрировать email-провайдера для транзакционных уведомлений.
- [ ] `NOTIFY-P0-4` Реализовать шаблоны уведомлений (`notification_templates`) и версионирование шаблонов.
- [ ] `NOTIFY-P0-5` Реализовать idempotent delivery (`event_id + channel + recipient`) и retry policy.
- [ ] `NOTIFY-P0-6` Добавить таблицу доставок (`notification_deliveries`) и обработку failed/DLQ сценариев.
- [ ] `NOTIFY-P0-7` Ввести базовые настройки пользователя (`notification_preferences`) с обязательными транзакционными событиями.
- [ ] `NOTIFY-P0-8` Добавить алерты на падение delivery success rate и рост retry/fail.
- [ ] `NOTIFY-P0-9` Внедрить обязательные user-сценарии:
  - тренеру уведомление о новой подписке клиента (`subscription_activated`);
  - тренеру уведомление о завершении тренировки клиентом (`workout_completed`);
  - клиенту уведомление о готовности тренировки (`workout_ready`).

## 4) Frontend под новую модель

- [ ] `FRONT-P0-1` Экран установки цены подписки тренером.
- [ ] `FRONT-P0-2` Экран оплаты/статуса подписки клиента.
- [ ] `FRONT-P0-3` Гейтинг функционала по статусу подписки клиента.
- [ ] `FRONT-P0-4` Исправить/стабилизировать 401-refresh flow без гонок редиректа.
- [ ] `FRONT-P0-5` Добавить экран детального просмотра плана (не только summary).
- [ ] `FRONT-P0-6` Добавить центр уведомлений (список, unread count, mark as read).
- [ ] `FRONT-P0-7` Добавить экран/блок пользовательских notification preferences.

## 5) Инфраструктура и релизный контур

- [ ] `INFRA-P0-1` Включить миграции в CD перед rollout.
- [ ] `INFRA-P0-2` Добавить post-deploy smoke checks (auth, tenant, profile, plan, billing webhook, notification delivery).
- [ ] `INFRA-P0-3` Убрать продовый default на `latest`, использовать pinned image tags.
- [ ] `INFRA-P0-4` Поднять TLS на внешнем периметре.
- [ ] `INFRA-P0-5` Закрыть внешний доступ к MinIO admin/data портам.
- [ ] `INFRA-P0-6` Настроить backup/restore для Postgres и MinIO.

## 6) Минимальная тестовая защита

- [ ] `TEST-P0-1` E2E smoke: trainer register -> set price -> client pay -> active access -> generate plan.
- [ ] `TEST-P0-2` Контрактные тесты frontend/backend по billing и subscription statuses.
- [ ] `TEST-P0-3` Финансовые тесты idempotency webhook и расчета комиссии.
- [ ] `TEST-P0-4` Нагрузочный тест списков tenant-service после DB-pagination.
- [ ] `TEST-P0-5` Тесты notification routing: правильный канал/шаблон/получатель.
- [ ] `TEST-P0-6` Тесты retry/DLQ и идемпотентной доставки уведомлений.
- [ ] `TEST-P0-7` E2E-проверка обязательных уведомлений:
  - тренер получает "клиент подписался";
  - тренер получает "клиент потренировался";
  - клиент получает "тренировка готова".

---

## P1 — 1-2 релиза после pilot

## 1) Выплаты и сверка

- [ ] `BILL-P1-1` Реализовать payout batches тренерам (ежедневно/еженедельно).
- [ ] `BILL-P1-2` Статусы выплат и retry-механизм.
- [ ] `BILL-P1-3` Reconciliation с провайдером, алерты на расхождения.
- [ ] `BILL-P1-4` Отчеты для тренера: выплаты, удержания, net доход.

## 2) Product improvements

- [ ] `PROD-P1-1` Улучшить onboarding клиента до оплаты (conversion-first flow).
- [ ] `PROD-P1-2` Улучшить onboarding тренера по установке цены и упаковке оффера.
- [ ] `PROD-P1-3` Сценарии pausing/canceling subscription в UI/API.
- [ ] `PROD-P1-4` Автоматические nudges для роста completion и retention.

## 3) Notifications и retention automation

- [ ] `NOTIFY-P1-1` Добавить продуктовые сценарии уведомлений: `workout_due`, `workout_missed`, `invite_accepted`.
- [ ] `NOTIFY-P1-2` Настроить расписания reminder/nudge и quiet hours.
- [ ] `NOTIFY-P1-3` Добавить аналитику эффективности уведомлений (open/click/conversion в оплату/тренировку).
- [ ] `NOTIFY-P1-4` A/B тесты шаблонов и времени отправки.
- [ ] `NOTIFY-P1-5` Добавить тренеру weekly summary по клиентской активности (тренировки завершены/пропущены).

## 4) Messaging (чат trainer-client)

- [ ] `MESS-P1-1` Спроектировать и внедрить сущности чата: `chat_conversations`, `chat_messages`, `chat_message_reads`.
- [ ] `MESS-P1-2` Реализовать API: получить диалог, список сообщений (cursor), отправка, mark-as-read.
- [ ] `MESS-P1-3` Реализовать ACL чата: доступ только при active relation + active subscription.
- [ ] `MESS-P1-4` Реализовать realtime доставку сообщений (WebSocket) + fallback polling.
- [ ] `MESS-P1-5` Добавить UI чата для тренера и клиента (список диалогов, окно переписки, unread).
- [ ] `MESS-P1-6` Генерировать `message_sent` событие и нотификацию получателю.
- [ ] `MESS-P1-7` Добавить anti-spam/rate limit и базовую модерацию.
- [ ] `MESS-P1-8` Добавить метрики чата (delivery latency, unread backlog, first response time).

## 5) Platform/Architecture

- [ ] `ARCH-P1-1` Ввести event contracts для `subscription_activated`, `payment_succeeded`, `payment_failed`, `message_sent`.
- [ ] `ARCH-P1-2` Минимальная event-шина для analytics/notifications/messaging.
- [ ] `ARCH-P1-3` Dashboards и alerting по payment success rate, churn, GMV, messaging SLA.
- [ ] `ARCH-P1-4` E2E regression suite в CI (включая чат и notification flows).

---

## P2 — масштабирование

- [ ] `BIZ-P2-1` Tiered take-rate (дифференцированная комиссия по обороту).
- [ ] `BIZ-P2-2` Promo/referral механики для тренеров и клиентов.
- [ ] `BIZ-P2-3` Add-ons для тренеров (расширенная аналитика/автоматизации).
- [ ] `ARCH-P2-1` Выделение `Analytics` и `Notification` в отдельные сервисы.
- [ ] `ARCH-P2-2` Мультиканальные уведомления (`telegram`, `push`) и сценарные цепочки.
- [ ] `ARCH-P2-3` Выделение `Messaging` в отдельный сервис и горизонтальное масштабирование realtime контура.
- [ ] `ARCH-P2-4` Расширение multi-tenant модели и enterprise features.
- [ ] `PROD-P2-1` Advanced chat features: вложения, поиск по диалогу, pinned messages.
- [ ] `OPS-P2-1` Расширенный compliance контур для международных платежей.

---

## Контрольные KPI (чтобы считать задачи закрытыми по результату)

- [ ] `KPI-1` Успешная активация тренера: регистрация -> установка цены -> первый платный клиент.
- [ ] `KPI-2` Успешная активация клиента: инвайт -> оплата -> первая завершенная тренировка.
- [ ] `KPI-3` Payment success rate >= 95% на pilot.
- [ ] `KPI-4` API 5xx <= 1% на критичных endpoint-ах.
- [ ] `KPI-5` Финансовое расхождение ledger vs provider <= 0.1%.
- [ ] `KPI-6` Delivery success rate критичных уведомлений >= 99%.
- [ ] `KPI-7` P95 доставки транзакционных уведомлений <= 60 секунд.
- [ ] `KPI-8` P95 доставки чат-сообщений <= 1 сек.
- [ ] `KPI-9` Доля активных trainer-client пар, использующих чат еженедельно.

