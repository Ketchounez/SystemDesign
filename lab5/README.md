# Домашнее задание №5

**Тема:** Оптимизация производительности через кеширование и rate limiting  
**Вариант 13:** Система бронирования отелей  


## Реализовано

1. Кеширование (Cache-Aside) для двух read-endpoint:
   - `GET /api/v1/hotels` (и фильтр по городу), TTL 60 секунд.
   - `GET /api/v1/users/{user_id}/bookings`, TTL 30 секунд.
2. Инвалидация кеша:
   - кеш отелей инвалидируется при `POST /api/v1/hotels`;
   - кеш бронирований пользователя инвалидируется при `POST /api/v1/bookings` и `DELETE /api/v1/bookings/{booking_id}`.
3. Rate limiting для `POST /api/v1/bookings`:
   - алгоритм Fixed Window Counter;
   - лимит 20 запросов в минуту на пользователя;
   - счетчик запросов хранится в Redis;
   - при превышении возвращается `429 Too Many Requests`;
   - добавлены заголовки `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`.

## Файлы для проверки

- `performance_design.md` — анализ hot paths, стратегия кеширования и rate limiting, метрики.
- `src/domain/service.hpp`, `src/domain/service.cpp` — реализация кешей, инвалидации и счетчика лимитов.
- `src/api/handlers/booking_handlers.cpp` — проверка rate limit и возврат HTTP-заголовков.
- `README.md` — описание проекта и оптимизаций.
- `Dockerfile`, `docker-compose.yaml` — запуск приложения и инфраструктуры.

## Запуск

Из каталога `lab5`:

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

## API

Публичный вход через gateway: `http://localhost:8080`.

- `POST /api/v1/auth/register`
- `POST /api/v1/auth/login`
- `GET /api/v1/users/by-login/{login}`
- `GET /api/v1/users/search?name=&surname=`
- `GET|POST /api/v1/hotels`
- `POST /api/v1/bookings`
- `GET /api/v1/users/{user_id}/bookings`
- `DELETE /api/v1/bookings/{booking_id}`

## Быстрая проверка rate limit

После логина используйте токен в `Authorization: Bearer <token>` и отправьте много запросов в `POST /api/v1/bookings`.  
После достижения лимита сервис вернет `429`, а также заголовки:

- `X-RateLimit-Limit`
- `X-RateLimit-Remaining`
- `X-RateLimit-Reset`
