CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    surname VARCHAR(128) NOT NULL,
    email VARCHAR(256) NOT NULL UNIQUE,
    password_hash VARCHAR(128) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS hotels (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(256) NOT NULL,
    city VARCHAR(128) NOT NULL,
    address VARCHAR(256) NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    created_by_user_id BIGINT NOT NULL REFERENCES users(id),
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS bookings (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id),
    hotel_id BIGINT NOT NULL REFERENCES hotels(id),
    check_in DATE NOT NULL,
    check_out DATE NOT NULL,
    status VARCHAR(16) NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'cancelled')),
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    CHECK (check_in < check_out)
);

CREATE TABLE IF NOT EXISTS sessions (
    token VARCHAR(128) PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id),
    login VARCHAR(64) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hotels_city ON hotels (city);
CREATE INDEX IF NOT EXISTS idx_hotels_created_by_user_id ON hotels (created_by_user_id);

CREATE INDEX IF NOT EXISTS idx_bookings_user_id ON bookings (user_id);
CREATE INDEX IF NOT EXISTS idx_bookings_hotel_id_status_dates
    ON bookings (hotel_id, status, check_in, check_out);
CREATE INDEX IF NOT EXISTS idx_bookings_created_at ON bookings (created_at);

CREATE INDEX IF NOT EXISTS idx_users_name_lower ON users ((lower(name)));
CREATE INDEX IF NOT EXISTS idx_users_surname_lower ON users ((lower(surname)));
