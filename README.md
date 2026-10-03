# fitboddy-infra-docker

Docker Compose инфраструктура Fitboddy (Postgres/Redis/MinIO, gateway, сервисы) + GHCR deploy.

## Quick start

```bash
cp .env.docker.example .env.docker
docker compose --env-file .env.docker up -d
```

## Локальный запуск после split

Репозиторий `fitboddy-infra-docker` должен лежать рядом с:

- `fitboddy-auth-service`
- `fitboddy-tenant-service`
- `fitboddy-profile-service`
- `fitboddy-plan-service`
- `fitboddy-messaging-service`
- `fitboddy-admin-frontend`
- `fitboddy-support-frontend`

Тогда `docker-compose.yml` соберет сервисы из соседних директорий.

После запуска основного стека:

- продукт: `http://localhost:8080`
- support console: `http://support.localhost:8080` (добавь `127.0.0.1 support.localhost` в hosts)
- health: `http://localhost:8080/health`

Локально `docker-compose.override.yml` поднимает `fitboddy-admin` и `fitboddy-support` на Vite с hot-reload (исходники монтируются с хоста).

Bootstrap первого `platform_admin` задаётся в `.env.docker`:

```env
PLATFORM_ADMIN_LOGIN=platform_admin
PLATFORM_ADMIN_PASSWORD=change_me_platform_admin
PLATFORM_ADMIN_EMAIL=admin@fitboddy.dev
```

## MinIO и медиа

MinIO слушает API `9000` и консоль `9001` только внутри docker-сети. `docker-compose.yml` эти порты наружу не публикует, прод (`docker-compose.yml` + `docker-compose.ghcr.yml`) тоже. На сервере не подключай `docker-compose.minio-dev.yml`.

Локальная консоль — отдельный opt-in, только loopback:

```bash
docker compose --env-file .env.docker \
  -f docker-compose.yml \
  -f docker-compose.override.yml \
  -f docker-compose.minio-dev.yml \
  up -d
```

После этого консоль доступна на `http://127.0.0.1:9001`. Логин — root-пара `S3_ACCESS_KEY` / `S3_SECRET_KEY`.

Бакет `S3_BUCKET` (по умолчанию `fitboddy-media`) приватный. `minio-init` создаёт его и явно ставит `anonymous private`. Публичной раздачи объектов нет.

Ключи:

| Переменные | Кто использует | Права |
| --- | --- | --- |
| `S3_ACCESS_KEY`, `S3_SECRET_KEY` | только контейнеры `minio` и `minio-init` | root |
| `S3_PROFILE_ACCESS_KEY`, `S3_PROFILE_SECRET_KEY` | `profile-service` | `HeadBucket`, список и `GetObject`/`PutObject` только на `avatars/*` |
| `S3_PLAN_ACCESS_KEY`, `S3_PLAN_SECRET_KEY` | `plan-service` | `HeadBucket`, список и `GetObject`/`PutObject`/`DeleteObject` только на `photos/*` и `videos/*` |

Три access key должны отличаться друг от друга. Префиксы в политиках зафиксированы: `avatars/`, `photos/`, `videos/`. Если меняешь префикс в compose, обнови JSON в `infra/docker/minio/policies/`.

Аватары не ходят в MinIO из браузера. `profile-service` сохраняет объект под `avatars/` и отдаёт относительный URL `/api/v1/profiles/media/...`. Gateway проксирует его в profile-service, сервис читает объект своим ключом. `S3_PUBLIC_BASE_URL` не задаётся: в текущем profile-service поле не используется, а прямой URL MinIO открыл бы бакет наружу.

Фото упражнений plan-service отдаёт короткоживущими подписанными URL. Для этого обязательны:

- `MEDIA_URL_SIGNING_SECRET` — секрет подписи приложения, не S3-ключ. В локальном примере стоит placeholder; в любом общем или продовом `.env.docker` замени его (`openssl rand -hex 32`). Пустое значение не допускается.
- `MEDIA_URL_TTL_SECONDS` — срок жизни URL в секундах (в примере `300`).

Смена ключей: поправь `.env.docker` и подними стек заново (`docker compose up -d`). `minio-init` пересоздаёт сервисных пользователей и политики. Root-секрет при этом не ротируется автоматически: его меняют отдельно, вместе с пересозданием volume или через консоль.

Gateway режет размер тела запроса:

- обычные API — `1m`;
- `POST /api/v1/profiles/{user}/avatar` — `8m` (приложение принимает до 5 МБ);
- `POST .../exercises/{row}/photos/...` и `POST /api/v1/admin/platform-exercises/{row}/photos/...` — `16m` (приложение принимает до 15 МБ);
- `POST .../exercises/{row}/video` и `POST /api/v1/admin/platform-exercises/{row}/video` — `210m` (приложение принимает до 200 МБ, запас на multipart).

Ответы `/api/v1/profiles/media/` и `/api/v1/trainers/media/` получают `X-Content-Type-Options: nosniff`.

## Отдельный контур: Progress Tracker

Для полностью изолированного сервиса личных отметок (не влияет на основной продукт) используй:

```bash
cp .env.checkins.example .env.checkins
docker compose --env-file .env.checkins -f docker-compose.checkins.yml up -d --build
```

После запуска:

- сайт: `http://localhost:8090`
- health: `http://localhost:8090/health`

Остановить только этот контур:

```bash
docker compose --env-file .env.checkins -f docker-compose.checkins.yml down
```
