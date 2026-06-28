# Fitboddy: дизайн системы уведомлений

Версия: 1.0  
Дата: 2026-06-28  
Контекст: модель `trainer-set pricing + client subscription + platform commission`.

---

## 1. Цели уведомлений

- Повышать конверсию в оплату и продление подписки.
- Снижать churn клиентов и тренеров.
- Давать прозрачность по транзакциям (платежи, списания, выплаты).
- Поддерживать продуктовый цикл активности (инвайт -> план -> тренировки).

---

## 2. Каналы v1.1

Обязательные:
- `in_app` (веб-центр уведомлений в `admin-frontend`).
- `email` (транзакционные и сервисные уведомления).

Опционально (после pilot):
- `telegram` (если пользователь привязал аккаунт).
- `push` (после появления мобильного приложения/PWA push).

---

## 3. Роли и получатели

- `trainer`:
  - события по подпискам клиентов (включая "клиент подписался");
  - платежи и выплаты;
  - события активности клиентов (включая "клиент завершил тренировку", "клиент пропустил тренировку").
  - новые сообщения от клиента в чате.
- `client`:
  - успешная/неуспешная оплата;
  - статус подписки;
  - готовность тренировки (`workout_ready`) и напоминания о тренировке.
  - новые сообщения от тренера в чате.
- `platform_admin` (ограниченно):
  - системные алерты по delivery/failure rate (не пользовательские нотификации).

---

## 4. Типы уведомлений и приоритеты

Критичные транзакционные (обязательно доставить):
- `subscription_activated`
- `payment_succeeded`
- `payment_failed`
- `subscription_canceled`
- `payout_processed` (для тренера)

Операционные продуктовые:
- `invite_received`
- `invite_accepted`
- `plan_generated`
- `workout_ready` (клиенту)
- `workout_completed` (тренеру о клиенте)
- `new_message_received` (обоим участникам чата)
- `workout_today_reminder`
- `workout_missed_nudge`

Маркетинговые (позже):
- кампании, советы, upsell.

Приоритеты:
- `P1_CRITICAL`: транзакционные.
- `P2_HIGH`: важные продуктовые.
- `P3_NORMAL`: engagement/маркетинг.

---

## 5. Event-модель (что триггерит уведомления)

События-источники:
- из billing:
  - `subscription_created`
  - `subscription_activated`
  - `payment_succeeded`
  - `payment_failed`
  - `subscription_renewal_due`
  - `subscription_canceled`
  - `payout_processed`
- из tenant/profile/plan:
  - `trainer_invite_sent`
  - `trainer_invite_accepted`
  - `plan_generated`
  - `workout_ready`
  - `workout_completed`
  - `workout_due`
  - `workout_missed`
- из messaging:
  - `message_sent`
  - `message_read`

Для каждого события задается:
- `event_id` (uuid)
- `event_type`
- `occurred_at`
- `actor_user_id`
- `recipient_user_id`
- `context` (json: plan_id, payment_id, amount, trainer_name и т.д.)
- `dedupe_key` (для идемпотентной доставки)

---

## 6. Архитектура уведомлений (целевая)

## 6.1 Компоненты

- `Notification API`:
  - создание in-app уведомлений;
  - чтение/ack/unread count;
  - управление preference.
- `Notification Worker`:
  - обработка event queue;
  - выбор шаблона;
  - отправка в email/telegram adapters;
  - retry/dead-letter.
- `Template Manager`:
  - шаблоны по `event_type + channel + locale`.
- `Provider adapters`:
  - email provider;
  - telegram adapter (опционально).

## 6.2 Поток доставки

1. Domain service публикует событие (или пишет в outbox).
2. Worker читает событие, резолвит получателя и правила.
3. Проверяет preferences и mandatory-правила.
4. Формирует сообщение по шаблону.
5. Отправляет по каналам.
6. Сохраняет delivery status.
7. При ошибке делает retry с backoff.

---

## 7. Политика preferences

Неотключаемые:
- платежные и юридически значимые уведомления.

Отключаемые:
- напоминания и engagement-уведомления.

Уровни настроек:
- глобально по каналу (`email`, `in_app`, `telegram`);
- точечно по типу события (`workout_reminders`, `plan_updates`, `chat_messages`, `marketing`).

---

## 8. SLA и надежность

- Транзакционные события: доставка <= 1 минута (P95).
- Product reminders: доставка <= 5 минут (P95).
- Retry policy: 1m, 5m, 15m, 1h (max 6 попыток).
- Dead-letter queue/table для ручного разбора.
- Идемпотентность: уникальный ключ `event_id + channel + recipient`.

---

## 9. Данные (минимальная схема)

Минимальные таблицы:
- `notification_events`:
  - event_id, event_type, payload_json, occurred_at, dedupe_key, source_service
- `notification_templates`:
  - template_id, event_type, channel, locale, title_tpl, body_tpl, is_active, version
- `notification_preferences`:
  - user_id, channel, event_group, is_enabled, updated_at
- `notifications_in_app`:
  - notification_id, user_id, title, body, metadata_json, is_read, created_at, read_at
- `notification_deliveries`:
  - delivery_id, event_id, user_id, channel, status, attempt, provider_message_id, error_code, sent_at

---

## 10. Безопасность и compliance

- Подписанные webhooks провайдеров.
- PII в payload минимизируется, чувствительные поля маскируются в логах.
- Audit trail по изменениям шаблонов и preference.
- Rate limit на endpoint ручной отправки.

---

## 11. Наблюдаемость

Базовые метрики:
- `notifications_sent_total{channel,event_type,status}`
- `notification_delivery_latency_ms`
- `notification_retry_total`
- `notification_dlq_total`
- `unread_in_app_count` (агрегат по пользователям)

Алерты:
- spike по `delivery_failed`;
- падение success-rate ниже порога;
- рост DLQ.

---

## 12. Пошаговый rollout

### Phase P0 (до pilot)
- in-app + email для транзакционных billing events.
- обязательные пользовательские уведомления:
  - тренеру: `subscription_activated`, `workout_completed`;
  - клиенту: `workout_ready`.
- базовые reminders: `plan_generated`, `payment_failed`.
- минимальные preferences.

### Phase P1
- расширение событий lifecycle клиента.
- запуск notification-сценариев для чата (`new_message_received`) с приоритетом in-app и optional email fallback.
- telegram канал (опционально).
- шаблоны с A/B заголовками.

### Phase P2
- сегментация и персонализация кампаний.
- orchestration сценариев retention (sequence-based notifications).

---

## 13. Критерии готовности

- Все критичные транзакционные события доставляются в SLA.
- У пользователя доступен центр уведомлений в интерфейсе.
- Есть контроль retry/DLQ и технические алерты.
- Есть тесты идемпотентности и корректной маршрутизации каналов.

