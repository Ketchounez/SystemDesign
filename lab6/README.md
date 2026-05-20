# Домашнее задание №6

**Тема:** Проектирование Event-Driven архитектуры, RabbitMQ, CQRS  
**Вариант 13:** Система бронирования отелей  

## Реализовано

1. **Анализ событий и команд** — см. `event_driven_design.md`
2. **Event-Driven архитектура** — producers/consumers, поток событий, payload
3. **RabbitMQ** — topic exchange `hotel.booking.events`, routing keys по типу события
4. **CQRS** — write-model (PostgreSQL + MongoDB `bookings`), read-model (`booking_read_model`, `hotel_catalog_view`, `users_read_model`)
5. **Рабочая реализация** — `events/producer.py`, `events/consumer.py`, Docker-сервисы
6. **Каталог событий** — `event_catalog.md`

## Файлы для проверки

| Файл | Назначение |
|---|---|
| `event_driven_design.md` | Архитектура, команды/события, RabbitMQ, CQRS |
| `event_catalog.md` | Каталог событий с payload и подписчиками |
| `events/producer.py` | Producer событий |
| `events/consumer.py` | Consumer и проекция read-model |
| `docker-compose.yml` | API + PostgreSQL + MongoDB + Redis + RabbitMQ + consumer |
| `README.md` | Инструкции запуска |

## Запуск

Из каталога `lab6`:

```bash
docker-compose -f docker-compose.yml up --build -d
```

Проверка контейнеров:

```bash
docker-compose -f docker-compose.yml ps
```

Проверка Event-Driven потока:

```bash
chmod +x events/test_events.sh
./events/test_events.sh
```

Остановка:

```bash
docker-compose -f docker-compose.yml down -v
```

## RabbitMQ

- AMQP: `localhost:5672`
- Management UI: http://localhost:15672 (`hotel` / `hotel`)
- Exchange: `hotel.booking.events` (type: `topic`)
- Queue read-model: `hotel.booking.read_model.worker`

## API (из lab5)

Публичный вход через gateway: `http://localhost:8080`

- `POST /api/v1/auth/register`
- `POST /api/v1/auth/login`
- `GET /api/v1/users/by-login/{login}`
- `GET /api/v1/users/search?name=&surname=`
- `GET|POST /api/v1/hotels`
- `POST /api/v1/bookings`
- `GET /api/v1/users/{user_id}/bookings`
- `DELETE /api/v1/bookings/{booking_id}`

Дополнительно сохранены оптимизации lab5: cache-aside и rate limiting.

## Ручная публикация события

```bash
docker-compose -f docker-compose.yml --profile tools run --rm event-producer \
  --event-type booking.created \
  --payload-json '{"booking_id":42,"user_id":1,"hotel_id":2,"check_in":"2026-07-01","check_out":"2026-07-03","status":"active"}'
```

## Проверка read-model в MongoDB

```bash
docker-compose -f docker-compose.yml exec mongo mongosh hotel_booking_lab6 --eval 'db.booking_read_model.find().pretty()'
```
