# Домашнее задание №4

**Тема:** Проектирование и работа с MongoDB  
**Вариант 13:** Система бронирования отелей  

## Реализовано

1. В сравнении с `lab3` сущность `bookings` переделана под MongoDB.
2. `users`, `hotels`, `sessions` остаются в PostgreSQL.
3. В API (`C++/userver`) подключены одновременно `Postgres` и `Mongo`.
4. Добавлены Docker-сервисы PostgreSQL + MongoDB + API + gateway.
5. Подготовлены Mongo-артефакты ТЗ: `schema_design.md`, `data.js`, `queries.js`, `validation.js`.

Краткое обоснование выбора MongoDB:
- В MongoDB вынесены `bookings`, так как основная нагрузка по ним — выборки по пользователю, статусу и диапазону дат.
- В документе бронирования хранятся snapshot-поля пользователя и отеля, чтобы фиксировать состояние на момент брони.
- `users` и `hotels` оставлены в PostgreSQL.

## Файлы для проверки

- `schema_design.md` — проектирование Mongo-модели и обоснование embedded/references
- `data.js` — скрипт заполнения тестовыми данными
- `queries.js` — CRUD-запросы варианта 13 + aggregation pipeline
- `validation.js` — валидация коллекции `bookings` через `$jsonSchema`
- `schema.sql` / `data.sql` — PostgreSQL структура и тестовые данные для `users`, `hotels`, `sessions`
- `docker/docker-compose.yaml` — запуск API + MongoDB + gateway
- `mongo-init/01-init.js` — создание коллекций и индексов при старте Mongo

## Запуск

Из каталога `lab4`:

```bash
docker-compose up --build -d
```

Проверка контейнеров:

```bash
docker-compose ps
```

Остановка:

```bash
docker-compose down -v
```

## Запуск Mongo-скриптов

После старта `docker-compose`:

```bash
docker-compose exec mongo mongosh "mongodb://localhost:27017/hotel_booking_lab4" /docker-entrypoint-initdb.d/01-init.js
docker-compose exec mongo mongosh "mongodb://localhost:27017/hotel_booking_lab4" /app/data.js
docker-compose exec mongo mongosh "mongodb://localhost:27017/hotel_booking_lab4" /app/queries.js
docker-compose exec mongo mongosh "mongodb://localhost:27017/hotel_booking_lab4" /app/validation.js
```

Если запускаете скрипты с хоста:

```bash
mongosh "mongodb://localhost:27017/hotel_booking_lab4" data.js
mongosh "mongodb://localhost:27017/hotel_booking_lab4" queries.js
mongosh "mongodb://localhost:27017/hotel_booking_lab4" validation.js
```

## API (userver + PostgreSQL + MongoDB)

В сравнении с `lab3` сущность `bookings` переведена на MongoDB:
- операции с `users` и `hotels` выполняются через PostgreSQL;
- операции с `bookings` выполняются через MongoDB.

Публичный вход через gateway: `http://localhost:8080`.

- `POST /api/v1/auth/register`
- `POST /api/v1/auth/login`
- `GET /api/v1/users/by-login/{login}`
- `GET /api/v1/users/search?name=&surname=`
- `GET|POST /api/v1/hotels`
- `POST /api/v1/bookings`
- `GET /api/v1/users/{user_id}/bookings`
- `DELETE /api/v1/bookings/{booking_id}`

## Критерии оценивания

- Корректность документной модели: отражена в `schema_design.md`.
- Обоснование embedded/references: отдельный раздел в `schema_design.md`.
- Качество MongoDB-запросов: `queries.js` использует `$eq`, `$ne`, `$gt`, `$lt`, `$in`, `$and`, `$or`, `$addToSet`, `$pull`.
- Правильность валидации схем: `validation.js` содержит `required`, типы, `minimum`, `enum`.
- Документация приведена в этом файле и в инструкциях по запуску/проверке.
