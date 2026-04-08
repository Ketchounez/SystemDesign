# Лабораторная работа №3 "Проектирование и оптимизация реляционной бд"

**Вариант 13:** система бронирования отелей.

## Что реализовано

- Спроектирована схема PostgreSQL для сущностей `users`, `hotels`, `bookings`.
- Добавлены ограничения `NOT NULL`, `UNIQUE`, `CHECK`, внешние ключи.
- Добавлены индексы для частых операций API.
- Подготовлены SQL-запросы для всех операций варианта 13.
- API из прошлой работы подключено к PostgreSQL через `userver::components::Postgres`.

## Файлы проекта

- `schema.sql` — SQL-скрипт создания структуры базы данных (таблицы, ограничения, индексы).
- `data.sql` — SQL-скрипт заполнения базы тестовыми данными.
- `queries.sql` — SQL-запросы, соответствующие операциям варианта 13.
- `optimization.md` — документ с анализом и обоснованием выполненных оптимизаций запросов.
- `Dockerfile` — инструкция сборки контейнерного образа API-сервиса.
- `docker-compose.yaml` — конфигурация совместного запуска API-сервиса, PostgreSQL и gateway.

## Запуск

Из каталога `lab3`:

```bash
docker compose up --build -d
```

Проверка:

```bash
curl -i http://localhost:8080/api/v1/hotels
```

Остановка:

```bash
docker compose down -v
```

## Описание схемы

### users

- `id` — PK
- `login` — уникальный логин
- `name`, `surname`, `email`
- `password_hash`

### hotels

- `id` — PK
- `name`, `city`, `address`, `description`
- `created_by_user_id` — FK на `users(id)`

### bookings

- `id` — PK
- `user_id` — FK на `users(id)`
- `hotel_id` — FK на `hotels(id)`
- `check_in`, `check_out`
- `status` (`active`/`cancelled`)

### sessions

Техническая таблица для Bearer-токенов API.
