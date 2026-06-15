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

Тогда `docker-compose.yml` соберет сервисы из соседних директорий.
