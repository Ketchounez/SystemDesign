#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if docker compose version >/dev/null 2>&1; then
  COMPOSE="docker compose"
else
  COMPOSE="docker-compose"
fi

echo "==> Waiting for RabbitMQ..."
for _ in $(seq 1 40); do
  if $COMPOSE -f docker-compose.yml exec -T rabbitmq rabbitmq-diagnostics -q ping >/dev/null 2>&1; then
    break
  fi
  sleep 2
done

echo "==> Publishing demo events"
$COMPOSE -f docker-compose.yml --profile tools run --rm event-producer --demo

echo "==> Waiting for read-model projection"
sleep 3

echo "==> Verifying booking_read_model in MongoDB"
$COMPOSE -f docker-compose.yml exec -T mongo mongosh --quiet hotel_booking_lab6 --eval '
const doc = db.booking_read_model.findOne({ booking_id: 9001 });
if (!doc) {
  print("ERROR: booking_read_model document not found");
  quit(1);
}
if (doc.status !== "cancelled") {
  print("ERROR: expected status=cancelled, got " + doc.status);
  quit(1);
}
print("OK: booking_read_model contains booking_id=9001 status=cancelled");
'

echo "==> Event flow test passed"
