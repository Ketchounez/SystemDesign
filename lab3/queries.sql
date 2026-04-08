INSERT INTO users(login, name, surname, email, password_hash)
VALUES ('new_user', 'Ivan', 'Petrov', 'new_user@mail.test', 'hash');

SELECT id, login, name, surname, email
FROM users
WHERE login = 'new_user';

SELECT id, login, name, surname, email
FROM users
WHERE lower(name) LIKE '%' || lower('iv') || '%'
  AND lower(surname) LIKE '%' || lower('pe') || '%'
ORDER BY id;

INSERT INTO hotels(name, city, address, description, created_by_user_id)
VALUES ('Hotel Demo', 'Moscow', 'Tverskaya 11', 'New hotel', 1);

SELECT id, name, city, address, description, created_by_user_id
FROM hotels
ORDER BY id;

SELECT id, name, city, address, description, created_by_user_id
FROM hotels
WHERE lower(city) = lower('Moscow')
ORDER BY id;

INSERT INTO bookings(user_id, hotel_id, check_in, check_out, status)
VALUES (1, 2, DATE '2026-05-01', DATE '2026-05-05', 'active');

SELECT id, user_id, hotel_id, check_in, check_out, status
FROM bookings
WHERE user_id = 1
ORDER BY id;

UPDATE bookings
SET status = 'cancelled'
WHERE id = 1
  AND user_id = 1
  AND status = 'active';
