# fitboddy-infra-docker

Docker Compose инфраструктура Fitboddy (Postgres/Redis/MinIO, gateway, сервисы) + GHCR deploy.

## Quick start

```bash
cp .env.docker.example .env.docker
docker compose --env-file .env.docker up -d
```
