# Каталог событий (Event Catalog)

**Домен:** Бронирование отелей  
**Exchange:** `hotel.booking.events` (`topic`)  
**Формат:** JSON envelope

---

## 1. `user.registered`

**Описание:** Пользователь успешно зарегистрирован в системе.

**Producer:** `hotel-booking-api` (команда `RegisterUser`)

**Consumers:**

- `event-consumer` → коллекция `users_read_model`

**Гарантия доставки:** at-least-once

### Payload

| Поле | Тип | Обязательное | Описание |
|---|---|---|---|
| `user_id` | int64 | да | Идентификатор пользователя |
| `login` | string | да | Логин |
| `name` | string | да | Имя |
| `surname` | string | да | Фамилия |
| `email` | string | да | Email |

### Пример

```json
{
  "event_id": "f6f2d2f7-2f0a-4f0a-9f0a-111111111111",
  "event_type": "user.registered",
  "occurred_at": "2026-05-20T10:00:00+00:00",
  "producer": "hotel-booking-api",
  "payload": {
    "user_id": 101,
    "login": "demo_user",
    "name": "Ivan",
    "surname": "Petrov",
    "email": "ivan@example.com"
  }
}
```

---

## 2. `hotel.created`

**Описание:** В каталог добавлен новый отель.

**Producer:** `hotel-booking-api` (команда `CreateHotel`)

**Consumers:**

- `event-consumer` → `hotel_catalog_view`
- (опционально) cache-invalidation worker

**Гарантия доставки:** at-least-once

### Payload

| Поле | Тип | Обязательное | Описание |
|---|---|---|---|
| `hotel_id` | int64 | да | Идентификатор отеля |
| `name` | string | да | Название |
| `city` | string | да | Город |
| `address` | string | да | Адрес |
| `created_by_user_id` | int64 | да | Автор создания |

### Пример

```json
{
  "event_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "event_type": "hotel.created",
  "occurred_at": "2026-05-20T10:05:00+00:00",
  "producer": "hotel-booking-api",
  "payload": {
    "hotel_id": 501,
    "name": "Grand Plaza",
    "city": "Moscow",
    "address": "Tverskaya 1",
    "created_by_user_id": 101
  }
}
```

---

## 3. `booking.created`

**Описание:** Создано новое активное бронирование.

**Producer:** `hotel-booking-api` (команда `CreateBooking`)

**Consumers:**

- `event-consumer` → `booking_read_model`
- (опционально) notification-service, analytics-service

**Гарантия доставки:** at-least-once

### Payload

| Поле | Тип | Обязательное | Описание |
|---|---|---|---|
| `booking_id` | int64 | да | Идентификатор бронирования |
| `user_id` | int64 | да | Пользователь |
| `hotel_id` | int64 | да | Отель |
| `check_in` | string (YYYY-MM-DD) | да | Дата заезда |
| `check_out` | string (YYYY-MM-DD) | да | Дата выезда |
| `status` | string | да | `active` |

### Пример

```json
{
  "event_id": "22222222-3333-4444-5555-666666666666",
  "event_type": "booking.created",
  "occurred_at": "2026-05-20T10:10:00+00:00",
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

---

## 4. `booking.cancelled`

**Описание:** Бронирование отменено пользователем.

**Producer:** `hotel-booking-api` (команда `CancelBooking`)

**Consumers:**

- `event-consumer` → `booking_read_model`
- (опционально) notification-service

**Гарантия доставки:** at-least-once

### Payload

| Поле | Тип | Обязательное | Описание |
|---|---|---|---|
| `booking_id` | int64 | да | Идентификатор бронирования |
| `user_id` | int64 | да | Владелец бронирования |
| `hotel_id` | int64 | да | Отель |
| `status` | string | да | `cancelled` |

### Пример

```json
{
  "event_id": "77777777-8888-9999-aaaa-bbbbbbbbbbbb",
  "event_type": "booking.cancelled",
  "occurred_at": "2026-05-20T10:15:00+00:00",
  "producer": "hotel-booking-api",
  "payload": {
    "booking_id": 9001,
    "user_id": 101,
    "hotel_id": 501,
    "status": "cancelled"
  }
}
```

---

## Матрица маршрутизации

| event_type | exchange | routing_key | queue | binding |
|---|---|---|---|---|
| `user.registered` | `hotel.booking.events` | `user.registered` | `hotel.booking.read_model.worker` | exact |
| `hotel.created` | `hotel.booking.events` | `hotel.created` | `hotel.booking.read_model.worker` | exact |
| `booking.created` | `hotel.booking.events` | `booking.created` | `hotel.booking.read_model.worker` | exact |
| `booking.cancelled` | `hotel.booking.events` | `booking.cancelled` | `hotel.booking.read_model.worker` | exact |

## Версионирование

- Поле `event_type` используется как контракт.
- Для обратной совместимости новые поля добавляются только как optional в `payload`.
- Breaking changes требуют нового `event_type` (например, `booking.created.v2`).
