# Домашнее задание №2

**Тема:** разработка REST API. **Вариант 13** — система бронирования отелей.

Сервис реализован на фреймворке Yandex userver.

## Функционал

Сущности:

- Пользователь
- Отель
- Бронирование

Операции:

- Создание пользователя
- Поиск пользователя по логину
- Поиск пользователя по маске имени и фамилии
- Создание отеля
- Получение списка отелей
- Поиск отелей по городу
- Создание бронирования
- Получение бронирований пользователя
- Отмена бронирования

Дополнительно:

- Регистрация и логин: `/api/v1/auth/register`, `/api/v1/auth/login`
- Защищённые методы: `Authorization: Bearer <token>`
- OpenAPI: `docs/openapi.yaml`
- Тесты: `bash tests/run.sh`, `tests/service_test.cpp`

## Структура

```text
lab2/
├── CMakeLists.txt
├── config/
│   └── static_config.yaml    — конфиг userver
├── docker/
│   ├── Dockerfile            — образ API
│   ├── docker-compose.yaml   — API + nginx (Swagger)
│   ├── gateway.Dockerfile
│   └── gateway-nginx.conf
├── docs/
│   └── openapi.yaml
├── src/
│   ├── main.cpp
│   ├── domain/
│   ├── api/
│   └── components/
└── tests/
    ├── run.sh
    └── service_test.cpp
```

## Запуск (Docker)

Из каталога `lab2/docker`:

```bash
cd lab2/docker
docker compose up --build -d
```

Или из корня `lab2`:

```bash
docker compose -f docker/docker-compose.yaml up --build -d
```

Логи API (из корня `lab2`):

```bash
docker compose -f docker/docker-compose.yaml logs -f hotel-booking-api
```

Остановка:

```bash
cd lab2/docker && docker compose down
```

(из корня `lab2`: то же с `-f docker/docker-compose.yaml`.)

Порт **8080**: API по `/api`, Swagger в корне, спецификация — `GET /openapi.yaml`.

## Автотест HTTP

Из корня `lab2`:

```bash
bash tests/run.sh
```

## Примеры API

### 1. Регистрация

```bash
curl -i -X POST http://localhost:8080/api/v1/auth/register \
  -H "Content-Type: application/json" \
  -d '{"login":"demo_user","name":"Ivan","surname":"Petrov","email":"demo@mail.test","password":"secret123"}'
```

### 2. Логин

```bash
curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"demo_user","password":"secret123"}'
```

```bash
TOKEN="<token из ответа>"
```

### 3. Создать отель (auth)

```bash
curl -i -X POST http://localhost:8080/api/v1/hotels \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"name":"Hotel","city":"Moscow","address":"Tverskaya 1"}'
```

### 4. Список отелей

```bash
curl -i http://localhost:8080/api/v1/hotels
```

### 5. Отели по городу

```bash
curl -i "http://localhost:8080/api/v1/hotels?city=Moscow"
```

### 6. Пользователь по логину

```bash
curl -i http://localhost:8080/api/v1/users/by-login/demo_user
```

### 7. Поиск по имени и фамилии

```bash
curl -i "http://localhost:8080/api/v1/users/search?name=Iv&surname=Pet"
```

### 8. Создать бронирование (auth)

```bash
curl -i -X POST http://localhost:8080/api/v1/bookings \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"hotel_id":1,"check_in":"2026-04-10","check_out":"2026-04-15"}'
```

### 9. Бронирования пользователя (auth)

```bash
curl -i http://localhost:8080/api/v1/users/1/bookings \
  -H "Authorization: Bearer $TOKEN"
```

### 10. Отмена бронирования (auth)

```bash
curl -i -X DELETE http://localhost:8080/api/v1/bookings/1 \
  -H "Authorization: Bearer $TOKEN"
```

## Негативные проверки

Без токена (ожидается `401`):

```bash
curl -i -X POST http://localhost:8080/api/v1/hotels \
  -H "Content-Type: application/json" \
  -d '{"name":"x","city":"y","address":"z"}'
```

Пустой поиск (ожидается `400`):

```bash
curl -i "http://localhost:8080/api/v1/users/search"
```

## Локально без Docker

```bash
cd lab2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hotel_booking_lab2 --config ./config/static_config.yaml
```
