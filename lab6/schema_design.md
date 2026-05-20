# Проектирование гибридной модели данных (Вариант 13)

Система: бронирование отелей (`Пользователь`, `Отель`, `Бронирование`).

## Разделение между PostgreSQL и MongoDB

- PostgreSQL: `users`, `hotels`, `sessions`
- MongoDB: `bookings`

Такое разделение выбрано, потому что:
- `users` и `hotels` хорошо ложатся в реляционную модель (строгие связи, уникальные поля, нормализованные данные).
- `bookings` удобно хранить документами с embedded snapshot полями для истории и быстрых выборок.

## Структуры документов

### bookings

```javascript
{
  _id: ObjectId,
  id: NumberLong,
  user_id: NumberLong,    // reference -> PostgreSQL users.id
  hotel_id: NumberLong,   // reference -> PostgreSQL hotels.id
  check_in: String,       // YYYY-MM-DD
  check_out: String,      // YYYY-MM-DD
  guests_count: Number,
  status: String,         // active|cancelled
  created_at: Date,
  cancelled_at: Date|null,
  guests_count: Number,   // опционально
  total_price: Number,    // опционально

  // embedded snapshot для историчности
  user_snapshot: {
    login: String,
    full_name: String
  },
  hotel_snapshot: Object // опционально
}
```

## Обоснование embedded vs references для MongoDB сущности bookings

- `bookings.user_id` и `bookings.hotel_id` — **references** на PostgreSQL:
  - пользователь и отель живут независимо от бронирования;
  - один пользователь связан со многими бронированиями;
  - один отель связан со многими бронированиями;
  - нет дублирования «живых» данных в Mongo.

- `user_snapshot` и `hotel_snapshot` сделаны как **embedded**, потому что:
  - нужны для хранения исторического состояния на момент бронирования;
  - позволяют читать историю бронирования без join-подобных операций;
  - изменения профиля пользователя/отеля не портят историю.

## Индексы

- `bookings.id` (unique)
- `bookings.user_id + status`
- `bookings.hotel_id + check_in + check_out`

Индексы покрывают ключевые Mongo-операции для бронирований: получение бронирований пользователя и проверки конфликтов дат по отелю.
