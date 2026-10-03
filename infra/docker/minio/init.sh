#!/bin/sh
# Инициализация приватного бакета и отдельных ключей сервисов.
# Root-учётка используется только здесь. Бакет принудительно private:
# аватары читает profile-service через gateway, фото и видео — plan-service.
set -eu

MINIO_ENDPOINT="${MINIO_ENDPOINT:-http://minio:9000}"
MINIO_ROOT_USER="${MINIO_ROOT_USER:?MINIO_ROOT_USER is required}"
MINIO_ROOT_PASSWORD="${MINIO_ROOT_PASSWORD:?MINIO_ROOT_PASSWORD is required}"
S3_BUCKET="${S3_BUCKET:?S3_BUCKET is required}"
S3_PROFILE_ACCESS_KEY="${S3_PROFILE_ACCESS_KEY:?S3_PROFILE_ACCESS_KEY is required}"
S3_PROFILE_SECRET_KEY="${S3_PROFILE_SECRET_KEY:?S3_PROFILE_SECRET_KEY is required}"
S3_PLAN_ACCESS_KEY="${S3_PLAN_ACCESS_KEY:?S3_PLAN_ACCESS_KEY is required}"
S3_PLAN_SECRET_KEY="${S3_PLAN_SECRET_KEY:?S3_PLAN_SECRET_KEY is required}"
POLICY_DIR="${POLICY_DIR:-/policies}"

# mc поставляется в образе minio/minio как /usr/bin/mc.
export PATH="/usr/bin:${PATH:-/bin}"
export MC_CONFIG_DIR="${MC_CONFIG_DIR:-/tmp/mc}"
export MC_DISABLE_PAGER=1
mkdir -p "$MC_CONFIG_DIR"

require_access_key() {
  name="$1"
  value="$2"
  min_len="$3"
  if [ "${#value}" -lt "$min_len" ]; then
    echo "minio-init: ${name} must be at least ${min_len} characters" >&2
    exit 1
  fi
  case "$value" in
    *[!A-Za-z0-9._-]*)
      echo "minio-init: ${name} may contain only letters, digits, '.', '_' and '-'" >&2
      exit 1
      ;;
  esac
}

require_secret() {
  name="$1"
  value="$2"
  nl='
'
  cr=$(printf '\r')
  if [ "${#value}" -lt 8 ]; then
    echo "minio-init: ${name} must be at least 8 characters" >&2
    exit 1
  fi
  case "$value" in
    *"$nl"*|*"$cr"*)
      echo "minio-init: ${name} must not contain line breaks" >&2
      exit 1
      ;;
  esac
}

require_access_key MINIO_ROOT_USER "$MINIO_ROOT_USER" 3
require_secret MINIO_ROOT_PASSWORD "$MINIO_ROOT_PASSWORD"
require_access_key S3_BUCKET "$S3_BUCKET" 3
require_access_key S3_PROFILE_ACCESS_KEY "$S3_PROFILE_ACCESS_KEY" 3
require_secret S3_PROFILE_SECRET_KEY "$S3_PROFILE_SECRET_KEY"
require_access_key S3_PLAN_ACCESS_KEY "$S3_PLAN_ACCESS_KEY" 3
require_secret S3_PLAN_SECRET_KEY "$S3_PLAN_SECRET_KEY"

if [ "$S3_PROFILE_ACCESS_KEY" = "$MINIO_ROOT_USER" ] \
  || [ "$S3_PLAN_ACCESS_KEY" = "$MINIO_ROOT_USER" ] \
  || [ "$S3_PROFILE_ACCESS_KEY" = "$S3_PLAN_ACCESS_KEY" ]; then
  echo "minio-init: root, profile and plan access keys must be different" >&2
  exit 1
fi

render_policy() {
  src="$1"
  dst="$2"
  tmp="${dst}.partial"
  : > "$tmp"
  while IFS= read -r line || [ -n "$line" ]; do
    rendered=""
    rest="$line"
    while :; do
      case "$rest" in
        *__BUCKET__*)
          rendered="${rendered}${rest%%__BUCKET__*}${S3_BUCKET}"
          rest="${rest#*__BUCKET__}"
          ;;
        *)
          rendered="${rendered}${rest}"
          break
          ;;
      esac
    done
    printf '%s\n' "$rendered" >> "$tmp"
  done < "$src"
  mv "$tmp" "$dst"
}

remove_user_if_present() {
  if mc admin user info local "$1" >/dev/null 2>&1; then
    mc admin user remove local "$1" >/dev/null
  fi
}

remove_policy_if_present() {
  if mc admin policy info local "$1" >/dev/null 2>&1; then
    mc admin policy remove local "$1" >/dev/null
  fi
}

ensure_user() {
  access_key="$1"
  secret_key="$2"
  policy_name="$3"
  # Ключ передаём через stdin, чтобы секрет с ведущим '-' не стал флагом.
  printf '%s\n%s\n' "$access_key" "$secret_key" | mc admin user add local >/dev/null
  mc admin policy attach local "$policy_name" --user "$access_key" >/dev/null
}

echo "minio-init: waiting for MinIO at ${MINIO_ENDPOINT}"
until mc alias set local "$MINIO_ENDPOINT" "$MINIO_ROOT_USER" "$MINIO_ROOT_PASSWORD" >/dev/null 2>&1; do
  sleep 2
done

mc mb --ignore-existing "local/${S3_BUCKET}" >/dev/null
# Бакет остаётся приватным даже если раньше на нём включали anonymous download.
mc anonymous set private "local/${S3_BUCKET}" >/dev/null

# Null s3:prefix в политике разрешает HeadBucket (bucket_exists в minio-py)
# и не открывает листинг всего бакета. Чтение объектов остаётся на префиксе сервиса.
render_policy "${POLICY_DIR}/profile-media.json" /tmp/profile-media.json
render_policy "${POLICY_DIR}/plan-media.json" /tmp/plan-media.json

remove_user_if_present "$S3_PROFILE_ACCESS_KEY"
remove_user_if_present "$S3_PLAN_ACCESS_KEY"
remove_policy_if_present profile-media
remove_policy_if_present plan-media

mc admin policy create local profile-media /tmp/profile-media.json >/dev/null
mc admin policy create local plan-media /tmp/plan-media.json >/dev/null
ensure_user "$S3_PROFILE_ACCESS_KEY" "$S3_PROFILE_SECRET_KEY" profile-media
ensure_user "$S3_PLAN_ACCESS_KEY" "$S3_PLAN_SECRET_KEY" plan-media

rm -f /tmp/profile-media.json /tmp/plan-media.json /tmp/profile-media.json.partial /tmp/plan-media.json.partial
echo "minio-init: private bucket ${S3_BUCKET}; users profile-media and plan-media are ready"
