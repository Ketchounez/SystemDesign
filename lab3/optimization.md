## Оптимизация запросов (вариант 13)

Использованы команды `EXPLAIN (ANALYZE, BUFFERS)` для оценки планов выполнения.

## Подготовка к замерам

```sql
\i schema.sql
\i data.sql
ANALYZE users;
ANALYZE hotels;
ANALYZE bookings;
```

### 1. Поиск отелей по городу

Запрос API:

```sql
SELECT id, name, city, address
FROM hotels
WHERE lower(city) = lower($1)
ORDER BY id;
```

Проверка плана:

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, name, city, address
FROM hotels
WHERE lower(city) = lower('Moscow')
ORDER BY id;
```

### Оптимизация

```sql
CREATE INDEX idx_hotels_city ON hotels(city);
```

Результат: поиск по `city` выполняется по индексу `idx_hotels_city`.

### 2. Получение бронирований пользователя

Запрос API:

```sql
SELECT id, user_id, hotel_id, check_in, check_out, status
FROM bookings
WHERE user_id = $1
ORDER BY id;
```

Проверка плана:

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, user_id, hotel_id, check_in, check_out, status
FROM bookings
WHERE user_id = 1
ORDER BY id;
```

### Оптимизация

```sql
CREATE INDEX idx_bookings_user_id ON bookings(user_id);
```

Результат: выборка по `user_id` использует индекс `idx_bookings_user_id`.

### 3. Проверка пересечения дат для бронирования

Запрос API:

```sql
SELECT id
FROM bookings
WHERE hotel_id = $1
  AND status = 'active'
  AND NOT (check_out <= $2::date OR check_in >= $3::date)
LIMIT 1;
```

Проверка плана:

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id
FROM bookings
WHERE hotel_id = 1
  AND status = 'active'
  AND NOT (check_out <= DATE '2026-04-11' OR check_in >= DATE '2026-04-13')
LIMIT 1;
```

### Оптимизация

```sql
CREATE INDEX idx_bookings_hotel_id_status_dates
  ON bookings(hotel_id, status, check_in, check_out);
```

Результат: проверка пересечения дат использует композитный индекс `idx_bookings_hotel_id_status_dates`.

### 4. Поиск пользователей по маске имени/фамилии

Запрос API:

```sql
SELECT id, login, name, surname, email
FROM users
WHERE ($1 = '' OR lower(name) LIKE '%' || lower($1) || '%')
  AND ($2 = '' OR lower(surname) LIKE '%' || lower($2) || '%')
ORDER BY id;
```

Проверка плана:

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, login, name, surname, email
FROM users
WHERE ('' = '' OR lower(name) LIKE '%' || lower('Iv') || '%')
  AND ('' = '' OR lower(surname) LIKE '%' || lower('Pet') || '%')
ORDER BY id;
```

### Оптимизация

```sql
CREATE INDEX idx_users_name_lower ON users((lower(name)));
CREATE INDEX idx_users_surname_lower ON users((lower(surname)));
```

Результат: индексы применимы для части сценариев поиска по `lower(name)` и `lower(surname)`.

## Сравнение до/после

В таблицу можно подставить фактические значения `cost` и `time` из вашей среды.

| Запрос | До оптимизации | После оптимизации | Изменение |
|---|---|---|---|
| Поиск отелей по городу | `Seq Scan`, cost=... time=... | `Index/Bitmap Scan`, cost=... time=... | Уменьшение чтения страниц |
| Бронирования пользователя | `Seq Scan`, cost=... time=... | `Index/Bitmap Scan`, cost=... time=... | Ускорение выборки |
| Проверка конфликта дат | `Seq Scan`, cost=... time=... | `Index Scan`, cost=... time=... | Снижение стоимости критичного запроса |

## Итог

- Индексы выбраны под реальные операции API варианта 13.
- Оптимизации покрывают поиск отелей, выборку бронирований и проверку пересечений дат.
