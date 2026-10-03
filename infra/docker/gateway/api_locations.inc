location = /health {
    access_log off;
    default_type text/plain;
    return 200 "ok";
}

location ^~ /api/v1/auth {
    proxy_pass http://auth-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/users {
    proxy_pass http://auth-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/marketplace {
    proxy_pass http://tenant-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/stats {
    proxy_pass http://tenant-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/profiles {
    proxy_pass http://profile-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/exercises {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

# Фото платформенных упражнений: лимит приложения 15 МБ, небольшой запас на multipart.
location ~ ^/api/v1/admin/platform-exercises/[^/]+/photos(/|$) {
    client_max_body_size 16m;
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

# Видео платформенных упражнений: лимит приложения 200 МБ плюс запас на multipart.
location ~ ^/api/v1/admin/platform-exercises/[^/]+/video(/|$) {
    client_max_body_size 210m;
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
    proxy_request_buffering off;
    proxy_read_timeout 300s;
    proxy_send_timeout 300s;
}

location /api/v1/admin/platform-exercises {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/plans {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/admin/generation-policy {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/profiles/internal {
    default_type application/json;
    return 403 '{"detail":"internal endpoint is not exposed"}';
}

# Аватар: лимит приложения 5 МБ, запас на multipart.
location ~ ^/api/v1/profiles/[^/]+/avatar/?$ {
    client_max_body_size 8m;
    proxy_pass http://profile-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

# Файл аватара: не даём браузеру подменять Content-Type.
location ^~ /api/v1/profiles/media/ {
    proxy_pass http://profile-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
    add_header X-Content-Type-Options nosniff always;
}

location /api/v1/profiles {
    proxy_pass http://profile-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/plans {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/muscles {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/platform-exercises {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

# Фото упражнений тренера: лимит приложения 15 МБ, небольшой запас на multipart.
location ~ ^/api/v1/trainers/[^/]+/exercises/[^/]+/photos(/|$) {
    client_max_body_size 16m;
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

# Видео упражнений тренера: лимит приложения 200 МБ плюс запас на multipart.
location ~ ^/api/v1/trainers/[^/]+/exercises/[^/]+/video(/|$) {
    client_max_body_size 210m;
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
    proxy_request_buffering off;
    proxy_read_timeout 300s;
    proxy_send_timeout 300s;
}

# Байты фото и видео упражнений.
location ^~ /api/v1/trainers/media/ {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
    add_header X-Content-Type-Options nosniff always;
}

location /api/v1/trainers {
    proxy_pass http://plan-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/marketplace/internal {
    default_type application/json;
    return 403 '{"detail":"internal endpoint is not exposed"}';
}

location ^~ /api/v1/marketplace {
    proxy_pass http://tenant-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}

location ^~ /api/v1/messages {
    proxy_pass http://messaging-service:8000;
    proxy_http_version 1.1;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_read_timeout 3600s;
    proxy_send_timeout 3600s;
}

location ^~ /api/v1/tenants {
    proxy_pass http://tenant-service:8000;
    proxy_set_header Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}
