# Fitboddy Platform: техническое задание (ТЗ)

Версия: 1.1  
Дата: 2026-06-28  
Подготовлено на основе анализа текущего кода и документации:
- `fitboddy-auth-service`
- `fitboddy-tenant-service`
- `fitboddy-profile-service`
- `fitboddy-plan-service`
- `fitboddy-admin-frontend`
- `fitboddy-infra-docker`
- `tg_bot` (legacy-контур)
- `docs/mvp/*`, `docs/platform/*`

---

## 1. Цель ТЗ

Зафиксировать требования к платформе Fitboddy v1.1, где:
- тренер регистрируется бесплатно;
- тренер самостоятельно задает цену подписки;
- клиент подписывается на тренера;
- платформа удерживает комиссию с каждого платежа клиента.

## 2. Границы релиза v1.1

Входит:
- auth/tenant/profile/plan services + admin frontend + infra-docker;
- trainer-client marketplace flow;
- pricing + subscription + commission billing core;
- notification core (in-app + email для транзакционных и продуктовых событий);
- messaging core (чат trainer-client) как обязательный контур этапа P1;
- генерация и чтение активного плана;
- каталог упражнений тренера;
- минимальная продуктовая и финансовая аналитика.

Не входит в v1.1 (roadmap):
- multi-provider billing orchestration;
- сложные международные налоговые сценарии;
- полноценные native mobile-приложения.

## 3. Функциональные требования (FR)

### FR-AUTH
- FR-AUTH-1: Регистрация и логин по email/password, роль `trainer|client`.
- FR-AUTH-2: Выдача access/refresh JWT, ротация refresh, logout.
- FR-AUTH-3: `/me` и `/check` для валидации токенов.
- FR-AUTH-4: Гарантировать уникальность email на уровне БД (unique constraint).
- FR-AUTH-5: Поддерживать tenant-context в токене и downstream проверках.

### FR-TENANT
- FR-TENANT-1: Upsert discovery профилей тренеров/клиентов.
- FR-TENANT-2: Управление связями trainer-client (`invite/direct/accept/leave`).
- FR-TENANT-3: Выдача списков trainer/client + пагинация + поиск.
- FR-TENANT-4: Воронка тренера (`invites`, `active_clients`, conversion).
- FR-TENANT-5: Все операции изменения связей должны быть защищены аутентификацией и проверкой actor identity.

### FR-PROFILE
- FR-PROFILE-1: GET/PUT профиля пользователя с ACL по роли и связи.
- FR-PROFILE-2: Управление анкетой и ее валидностью (goal/level/location/equipment/limitations).
- FR-PROFILE-3: Загрузка аватара в S3/MinIO.
- FR-PROFILE-4: Внутренние endpoints должны быть защищены service-auth механизмом.
- FR-PROFILE-5: Медиа-доступ должен поддерживать безопасную схему (signed URLs/ACL policy).

### FR-PLAN
- FR-PLAN-1: Генерация 4-недельного плана для пользователя с привязкой к `trainer_user_id`.
- FR-PLAN-2: Хранение активного плана и архива предыдущих циклов.
- FR-PLAN-3: Получение активного плана и дня плана.
- FR-PLAN-4: CRUD каталога упражнений тренера + archive flow.
- FR-PLAN-5: Доступ к планам/каталогу должен быть role-aware и tenant-aware.
- FR-PLAN-6: Учитывать историю выполнения (новизна, прогрессия) в генерации следующего цикла.

### FR-BILLING (новый обязательный контур)
- FR-BILLING-1: Тренер может создать и обновлять публичное предложение подписки (`price`, `currency`, `period`, `is_active`).
- FR-BILLING-2: Цена задается тренером в допустимом диапазоне платформы (`min_price <= price <= max_price`).
- FR-BILLING-3: Клиент может оформить подписку на тренера и пройти checkout.
- FR-BILLING-4: Жизненный цикл подписки: `pending`, `active`, `past_due`, `canceled`, `paused`.
- FR-BILLING-5: Доступ клиента к платному функционалу тренера зависит от статуса подписки (`active`).
- FR-BILLING-6: Платформа рассчитывает комиссию (take rate) для каждого успешного платежа.
- FR-BILLING-7: Должен вестись финансовый ledger: `gross`, `platform_fee`, `processor_fee`, `trainer_payout`, `net`.
- FR-BILLING-8: Webhook платежного провайдера обрабатывается идемпотентно и обновляет подписку/платеж.
- FR-BILLING-9: Тренер получает отчет по доходам, комиссиям и выплатам.

### FR-NOTIFY (новый обязательный контур)
- FR-NOTIFY-1: Поддержка каналов `in_app` и `email` в v1.1.
- FR-NOTIFY-2: Транзакционные события billing (`payment_succeeded`, `payment_failed`, `subscription_activated`, `subscription_canceled`, `payout_processed`) должны генерировать уведомления автоматически.
- FR-NOTIFY-3: Продуктовые события (`invite_accepted`, `plan_generated`, `workout_ready`, `workout_due`, `workout_completed`) должны поддерживать уведомления по правилам.
- FR-NOTIFY-4: Пользователь должен иметь центр уведомлений с read/unread статусами.
- FR-NOTIFY-5: Предпочтения пользователя: отключаемые engagement-уведомления и обязательные транзакционные.
- FR-NOTIFY-6: Доставка уведомлений должна быть идемпотентной по `event_id + channel + recipient`.
- FR-NOTIFY-7: Ошибки доставки должны обрабатываться retry-policy с DLQ/таблицей неотправленных сообщений.
- FR-NOTIFY-8: Обязательные пользовательские уведомления v1.1:
  - тренеру: `subscription_activated` (клиент подписался на тренера);
  - тренеру: `workout_completed` (клиент завершил тренировку);
  - клиенту: `workout_ready` (тренировка готова).

### FR-MESSAGING (обязательный контур этапа P1)
- FR-MSG-1: Реализовать 1:1 чат `trainer <-> client` в рамках существующей связки.
- FR-MSG-2: Доступ к чату разрешён только при активной связи trainer-client и активной подписке клиента.
- FR-MSG-3: Поддержать отправку текстовых сообщений, историю сообщений и постраничную загрузку.
- FR-MSG-4: Поддержать статусы `sent`, `delivered`, `read` и счётчик непрочитанных.
- FR-MSG-5: Поддержать near-realtime доставку через WebSocket (fallback: short polling).
- FR-MSG-6: Каждое новое сообщение генерирует доменное событие `message_sent` для контура уведомлений.
- FR-MSG-7: Вести audit trail сообщений и действий модерации/блокировки.

### FR-FRONT
- FR-FRONT-1: Поддержка ролей trainer/client и защищенных роутов.
- FR-FRONT-2: Полноценный UX для критического мобильного цикла (login -> profile -> relation -> subscription -> plan -> completion).
- FR-FRONT-3: Экран установки цены подписки тренером.
- FR-FRONT-4: Экран оплаты/управления подпиской клиентом.
- FR-FRONT-5: Отдельный экран/модуль детального просмотра плана (не только summary).
- FR-FRONT-6: Корректная обработка 401/refresh без гонок редиректа.
- FR-FRONT-7: Единый API-layer с типобезопасными DTO и централизованными query keys.

### FR-INFRA
- FR-INFRA-1: Единая точка входа через gateway.
- FR-INFRA-2: Детерминированный deployment по pinned image tag (не `latest` по умолчанию).
- FR-INFRA-3: Автоматизированный шаг миграций в release pipeline.
- FR-INFRA-4: Post-deploy smoke checks (health/readiness ключевых сервисов).
- FR-INFRA-5: Backup/restore регламент для Postgres и MinIO.
- FR-INFRA-6: Отдельный защищенный endpoint/маршрут для приема billing webhooks.

## 4. Нефункциональные требования (NFR)

Надежность:
- NFR-1: API availability >= 99.9% для ключевого контура.
- NFR-2: План генерации P95 <= 1.5s (на baseline инфраструктуре).
- NFR-3: Ошибки 5xx <= 1% на критичных endpoints.
- NFR-4: Payment webhook processing success >= 99.9%.

Безопасность:
- NFR-5: TLS на внешнем периметре.
- NFR-6: Секреты вне git, ротация по регламенту.
- NFR-7: Аудит критичных действий (auth, relation changes, pricing changes, payment events).

Производительность:
- NFR-8: Списочные endpoints tenant-service должны работать с DB-pagination, без in-memory full scans.
- NFR-9: Frontend Lighthouse mobile >= 85 на ключевых экранах.
- NFR-10: Messaging delivery latency (server receive -> recipient event) P95 <= 1 секунда.

Эксплуатация:
- NFR-11: Structured logs (JSON) с `request_id`, `user_id`, `tenant_id` (где применимо).
- NFR-12: Метрики Prometheus + dashboards + alerting.
- NFR-13: Финансовая консистентность: расхождение ledger и данных провайдера <= 0.1%.
- NFR-14: Delivery success rate для критичных уведомлений >= 99%.
- NFR-15: Delivery latency P95 для транзакционных уведомлений <= 60 секунд.

## 5. Требования к безопасности

- SEC-1: Во всех write endpoints требуется валидный access token.
- SEC-2: Запрет impersonation через поля вроде `acting_user_id` без server-side верификации.
- SEC-3: Internal endpoints защищены service token/mTLS/private network ACL.
- SEC-4: Жесткий CORS policy для frontend origin.
- SEC-5: JWT secret и DB credentials только через защищенные секреты окружения.
- SEC-6: Ограничение rate limit для auth и checkout endpoints.
- SEC-7: Валидация подписи webhook платежного провайдера.
- SEC-8: Идемпотентность всех финансовых операций через `Idempotency-Key`.
- SEC-9: Rate limit и anti-spam правила для отправки сообщений в чате.
- SEC-10: Контроль доступа к сообщениям только для участников конкретной пары trainer-client.

## 6. Требования к данным и миграциям

- DATA-1: Все schema changes через Alembic migration.
- DATA-2: Миграции выполняются до запуска нового release.
- DATA-3: Ограничения целостности:
  - unique email в auth;
  - unique active relation invariant в tenant (через бизнес-логику + db safeguard);
  - unique active plan invariant в plan-service;
  - не более одной `active` подписки клиента на одного тренера;
  - `trainer_price` хранится как денежный тип с фиксированной точностью.
- DATA-4: Политика архивации старых планов и cleanup stale refresh tokens.
- DATA-5: Обязательные сущности биллинга:
  - `trainer_subscription_offers`;
  - `trainer_client_subscriptions`;
  - `payments`;
  - `payouts`;
  - `billing_ledger_entries`.
- DATA-6: Обязательные сущности уведомлений:
  - `notification_events`;
  - `notification_templates`;
  - `notification_preferences`;
  - `notifications_in_app`;
  - `notification_deliveries`.
- DATA-7: Обязательные сущности messaging:
  - `chat_conversations`;
  - `chat_conversation_participants`;
  - `chat_messages`;
  - `chat_message_reads`.

## 7. Требования к API-контрактам

- API-1: Единый префикс `/api/v1`.
- API-2: Стандартизованный формат ошибок (`code/message/details/request_id`).
- API-3: Backward-compatible расширения в рамках v1.
- API-4: Версионирование через `/api/v2` для breaking changes.
- API-5: Описание контрактов в OpenAPI и синхронизация с frontend типами.
- API-6: Все финансовые write endpoints поддерживают `Idempotency-Key`.
- API-7: Статусы подписки и платежей возвращаются как enum из фиксированного контракта.
- API-8: Messaging API должен включать:
  - создание/получение диалога trainer-client;
  - список сообщений диалога (cursor pagination);
  - отправку сообщения;
  - mark-as-read.

## 8. CI/CD и DevOps требования

- DEVOPS-1: CI для каждого сервиса: lint + test + build image.
- DEVOPS-2: CD pipeline: pull images -> migrate -> deploy -> smoke checks -> notify.
- DEVOPS-3: Rollback сценарий на предыдущий стабильный image tag.
- DEVOPS-4: Отказ от default `latest` как продового тега.
- DEVOPS-5: Контроль уязвимостей образов (SCA/Container scan).
- DEVOPS-6: Отдельные smoke checks для billing webhooks и подписочного lifecycle.
- DEVOPS-7: Отдельные smoke checks для messaging realtime и unread counters.

## 9. Тестовая стратегия

- TEST-1: backend unit + API integration tests на критичные use-cases.
- TEST-2: contract tests между frontend и backend.
- TEST-3: e2e smoke:
  - trainer registration/login;
  - trainer sets price;
  - client subscribes and pays;
  - access becomes active;
  - plan generation and retrieval.
- TEST-4: Нагрузочные тесты для tenant lists и plan generation.
- TEST-5: Финансовые тесты:
  - идемпотентная обработка webhook;
  - правильный расчет комиссии;
  - корректный payout и reconciliation.
- TEST-6: Messaging tests:
  - ACL и доступ только при active relation + active subscription;
  - корректная доставка/чтение сообщений;
  - генерация события `message_sent` и связанной нотификации.

## 10. Критерии приемки релиза v1.1

- AC-1: Полный сценарий trainer->price->client subscription->payment->plan проходит без ручных обходов.
- AC-2: Все критичные endpoints защищены auth и ACL.
- AC-3: Деплой воспроизводим и проходит smoke checks автоматически.
- AC-4: Наблюдаемость покрывает latency/error/availability по сервисам и платежам.
- AC-5: Нет критичных регрессий по mobile-first UX.
- AC-6: Финансовый ledger совпадает с провайдером в пределах допустимого SLA.
- AC-7: Ключевые уведомления доставляются корректным ролям:
  - тренеру о новой подписке клиента;
  - тренеру о завершении тренировки клиентом;
  - клиенту о готовности тренировки.
- AC-8: Messaging-сценарий работает end-to-end:
  - клиент и тренер могут обмениваться сообщениями;
  - unread/read статусы корректны;
  - событие `message_sent` отражается в центре уведомлений.

---

## 11. Приоритизированный roadmap работ (implementation backlog)

## 11.1. P0 (обязательно до production pilot)

- P0-1: Безопасность tenant/profile/plan endpoints (auth + actor verification).
- P0-2: Уникальность email в auth DB + защита от race conditions.
- P0-3: DB-пагинация в tenant-service (убрать full in-memory list flows).
- P0-4: Базовый billing-контур: trainer pricing, subscription, payment webhook, commission.
- P0-5: TLS + секреты + закрытие внешнего доступа к MinIO admin/data портам.
- P0-6: Frontend: экран установки цены, checkout, статус подписки, стабилизация auth-refresh.
- P0-7: Notification core: in-app + email для критичных billing и ключевых продуктовых событий (`subscription_activated`, `workout_completed`, `workout_ready`).
- P0-8: Автоматизация миграций и post-deploy smoke checks.

## 11.2. P1 (в течение 1-2 релизов после pilot)

- P1-1: Выплаты тренерам (payout batches, статусы, отчеты).
- P1-2: Event contracts и минимальная event-шина для analytics/notifications.
- P1-3: Расширение notification событий для lifecycle/retention сценариев.
- P1-4: Messaging core: 1:1 чат trainer-client, realtime delivery, read/unread статусы.
- P1-5: Алертинг и dashboards (SLO-driven + payment + notification + messaging metrics).
- P1-6: E2E regression suite в CI.

## 11.3. P2 (масштабирование)

- P2-1: Тонкая тарифизация комиссии (tiered take rate).
- P2-2: Расширенная сегментация клиентов и coaching automations.
- P2-3: Выделенные `Analytics`/`Notification` сервисы.
- P2-4: Мультиканальные уведомления (telegram/push) и orchestration-цепочки.
- P2-5: Messaging advanced features (вложения, поиск, модерирование, smart prompts).
- P2-6: Расширение multi-tenant модели и enterprise features.

---

## 12. Ответственность по сервисам (ownership)

- `auth-service`: identity, credentials, sessions, JWT policy.
- `tenant-service`: marketplace relations, доступы trainer-client.
- `profile-service`: анкета/профиль/медиа.
- `plan-service`: генерация и хранение планов, каталог упражнений тренера.
- `billing` (новый модуль/сервис): цены, подписки, платежи, комиссия, ledger, payouts.
- `messaging` (новый модуль/сервис): диалоги trainer-client, realtime доставка, read/unread, chat events.
- `admin-frontend`: UX, мобильный сценарий, pricing/subscription flows.
- `infra-docker`: окружение, deploy process, безопасность периметра.
- `tg_bot`: legacy и source-reference алгоритмов, не целевой production-контур v1.1.

---

## 13. Итог

Критический фокус v1.1:
1. **Billing-first** (цена тренера -> подписка клиента -> комиссия платформы).  
2. **Security & correctness** (auth/ACL/data invariants).  
3. **Notification reliability** (транзакционные и продуктовые уведомления в SLA).  
4. **Production operations** (CI/CD migrations, TLS, observability, reconciliation).

После закрытия P0-P1 платформа готова масштабировать GMV и комиссионную выручку без смены базовой модели монетизации.

