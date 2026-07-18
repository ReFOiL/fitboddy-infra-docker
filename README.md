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
