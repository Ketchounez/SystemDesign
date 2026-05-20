# Лабораторная работа №6: Event-Driven архитектура

**Вариант 13:** Система бронирования отелей 
**Брокер сообщений:** RabbitMQ (topic exchange)  
**Паттерн:** CQRS + Event-Driven

## 1. Анализ событий и команд

### Доменные сущности

- `User` — пользователь системы
- `Hotel` — отель
- `Booking` — бронирование номера

### Команды (инициируют изменения write-модели)

| Команда | HTTP endpoint | Описание |
|---|---|---|
| `RegisterUser` | `POST /api/v1/auth/register` | Создание пользователя |
| `Login` | `POST /api/v1/auth/login` | Аутентификация (без доменного события) |
| `CreateHotel` | `POST /api/v1/hotels` | Создание отеля |
| `CreateBooking` | `POST /api/v1/bookings` | Создание бронирования |
| `CancelBooking` | `DELETE /api/v1/bookings/{booking_id}` | Отмена бронирования |

### События (публикуются после успешной команды)

| Событие | routing key | Команда-источник |
|---|---|---|
| `user.registered` | `user.registered` | `RegisterUser` |
| `hotel.created` | `hotel.created` | `CreateHotel` |
| `booking.created` | `booking.created` | `CreateBooking` |
| `booking.cancelled` | `booking.cancelled` | `CancelBooking` |

### Подписчики на события

| Событие | Потребители |
|---|---|
| `user.registered` | Read-model projector (`event-consumer`) |
| `hotel.created` | Read-model projector, cache invalidation worker (концептуально) |
| `booking.created` | Read-model projector, notification/analytics worker (концептуально) |
| `booking.cancelled` | Read-model projector, notification worker (концептуально) |

В реализации лабораторной включен рабочий consumer `event-consumer`, который обновляет CQRS read-model в MongoDB.

## 2. Проектирование Event-Driven архитектуры

### Event producers

- `hotel-booking-api` (C++ userver) — command-side сервис, выполняет команды и в production публикует события после commit в БД.
- `events/producer.py` — демонстрационный producer для проверки интеграции с RabbitMQ.

### Event consumers

- `events/consumer.py` (`event-consumer` в Docker) — обновляет read-model:
  - `users_read_model`
  - `hotel_catalog_view`
  - `booking_read_model`

### Поток событий

Типичный сценарий создания бронирования:

1. Клиент отправляет команду `POST /api/v1/bookings` в сервис `hotel-booking-api`.
2. API проверяет пользователя в PostgreSQL (write-model пользователей и отелей).
3. API сохраняет бронирование в MongoDB в коллекцию `bookings` (write-model бронирований).
4. После успешной записи API публикует событие `booking.created` в RabbitMQ (exchange `hotel.booking.events`, routing key `booking.created`).
5. RabbitMQ доставляет сообщение consumer-сервису `event-consumer` с гарантией at-least-once.
6. Consumer выполняет идемпотентный upsert документа в коллекцию `booking_read_model` (read-model).

Чтение бронирований:

1. Клиент вызывает `GET /api/v1/users/{id}/bookings`.
2. В текущей реализации API читает write-model (`bookings`) с cache-aside из lab5.
3. В production-сценарии тот же запрос может обслуживаться из `booking_read_model`.

## 3. Брокер сообщений RabbitMQ

### Выбор RabbitMQ

Для домена бронирования важны:

- маршрутизация по типу события;
- независимые consumer-группы;
- подтверждение доставки и повторная обработка.
### Exchange и routing

- **Exchange:** `hotel.booking.events`
- **Тип:** `topic`
- **Очередь read-model:** `hotel.booking.read_model.worker`
- **Bindings:**
  - `user.registered`
  - `hotel.created`
  - `booking.created`
  - `booking.cancelled`

### Формат сообщения

JSON envelope:

```json
{
  "event_id": "uuid",
  "event_type": "booking.created",
  "occurred_at": "2026-05-20T12:00:00+00:00",
  "producer": "hotel-booking-api",
  "payload": {
    "booking_id": 9001,
    "user_id": 101,
    "hotel_id": 501,
    "check_in": "2026-06-01",
    "check_out": "2026-06-05",
    "status": "active"
  }
}
```

Свойства AMQP:

- `content_type=application/json`
- `delivery_mode=2` (persistent)
- `message_id=<event_id>`
- `type=<event_type>`

### Гарантии доставки

| Участник | Гарантия | Механизм |
|---|---|---|
| Producer → RabbitMQ | **at-least-once** | `publisher confirms` + persistent messages |
| RabbitMQ → Consumer | **at-least-once** | durable queue + manual `ack` |
| End-to-end exactly-once | **не гарантируется** | возможны дубликаты при retry |

Идемпотентность на consumer:

- upsert в MongoDB по ключу `(user_id, booking_id)` или `hotel_id`;
- повторная доставка не ломает состояние read-model.

## 4. CQRS

CQRS в системе **применим**.

### Write model (команды)

- PostgreSQL: `users`, `hotels`
- MongoDB: `bookings` (операционная write-модель бронирований)

### Read model (запросы)

- MongoDB:
  - `booking_read_model` — проекция бронирований пользователя
  - `hotel_catalog_view` — проекция каталога отелей
  - `users_read_model` — проекция пользователей

### Разделение операций

**Команды (write, изменяют состояние и порождают события):**

| Команда | HTTP endpoint |
|---|---|
| `RegisterUser` | `POST /api/v1/auth/register` |
| `CreateHotel` | `POST /api/v1/hotels` |
| `CreateBooking` | `POST /api/v1/bookings` |
| `CancelBooking` | `DELETE /api/v1/bookings/{booking_id}` |

**Запросы (read, события не публикуют):**

| Запрос | HTTP endpoint | Источник данных |
|---|---|---|
| `FindUserByLogin` | `GET /api/v1/users/by-login/{login}` | PostgreSQL (write-model) |
| `SearchUsers` | `GET /api/v1/users/search` | PostgreSQL (write-model) |
| `ListHotels` | `GET /api/v1/hotels` | PostgreSQL + cache (write-model) |
| `ListHotelsByCity` | `GET /api/v1/hotels?city=` | PostgreSQL + cache (write-model) |
| `ListUserBookings` | `GET /api/v1/users/{user_id}/bookings` | MongoDB `bookings` + cache (write-model) |
| `Login` | `POST /api/v1/auth/login` | PostgreSQL (без доменного события) |

**Целевые read-запросы через CQRS-проекции (обновляются consumer):**

| Запрос | Коллекция read-model |
|---|---|
| Каталог отелей | `hotel_catalog_view` |
| Бронирования пользователя | `booking_read_model` |
| Справочник пользователей | `users_read_model` |

### Синхронизация read/write через события

1. Команда успешно изменяет write-model.
2. API публикует доменное событие в RabbitMQ.
3. `event-consumer` получает событие и обновляет read-model (eventual consistency).
4. Query-сервис (или отдельный read API) читает `*_read_model` коллекции.

## 5. Реализация

| Компонент | Файл / сервис |
|---|---|
| RabbitMQ в Docker | `docker-compose.yml` → `rabbitmq` |
| Producer | `events/producer.py`, сервис `event-producer` |
| Consumer | `events/consumer.py`, сервис `event-consumer` |
| Интеграционный тест | `events/test_events.sh` |
| Command API (вариант 13) | `hotel-booking-api` |

Публикация событий выполняется отдельным producer-сервисом (`events/producer.py`).  
C++ API выполняет команды и пишет в write-model; в production-сценарии тот же сервис после успешного commit вызывал бы публикацию в RabbitMQ (как описано в разделе «Поток событий»).

Исходный код:

- `events/producer.py` — публикация событий
- `events/consumer.py` — обработка событий и обновление read-model
- `events/common.py` — общий контракт событий
- `docker-compose.yml` — RabbitMQ + consumer + API

Проверка:

```bash
docker-compose -f docker-compose.yml up --build -d
chmod +x events/test_events.sh
./events/test_events.sh
```